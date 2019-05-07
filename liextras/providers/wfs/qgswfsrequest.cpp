/***************************************************************************
    qgswfsrequest.cpp
    ---------------------
    begin                : February 2016
    copyright            : (C) 2011 by Martin Dobias
                           (C) 2016 by Even Rouault
    email                : wonder dot sk at gmail dot com
                           even.rouault at spatialys.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgswfsrequest.h"
#include "qgslogger.h"
#include "qgsmessagelog.h"
#include "qgsnetworkaccessmanager.h"
#include "qgsapplication.h"

#include <QEventLoop>
#include <QNetworkCacheMetaData>
#include <QCryptographicHash> // just for testin file:// fake_qgis_http_endpoint hack
#include <QFuture>
#include <QtConcurrent>

const qint64 READ_BUFFER_SIZE_HINT = 1024 * 1024;

QgsWfsRequest::QgsWfsRequest( const QgsWFSDataSourceURI &uri )
  : mUri( uri )
  , mErrorCode( QgsWfsRequest::NoError )
  , mIsAborted( false )
  , mForceRefresh( false )
  , mTimedout( false )
  , mGotNonEmptyResponse( false )
{
  QgsDebugMsg( "theUri = " + uri.uri( ) );
  connect( QgsNetworkAccessManager::instance(), &QgsNetworkAccessManager::requestTimedOut, this, &QgsWfsRequest::requestTimedOut );
}

QgsWfsRequest::~QgsWfsRequest()
{
  abort();
}

void QgsWfsRequest::requestTimedOut( QNetworkReply *reply )
{
  if ( reply == mReply )
    mTimedout = true;
}

QUrl QgsWfsRequest::requestUrl( const QString &request ) const
{
  return mUri.requestUrl( request );
}

bool QgsWfsRequest::sendGET( const QUrl &url, bool synchronous, bool forceRefresh, bool cache )
{
  abort(); // cancel previous
  mIsAborted = false;
  mTimedout = false;
  mGotNonEmptyResponse = false;

  mErrorMessage.clear();
  mErrorCode = QgsWfsRequest::NoError;
  mForceRefresh = forceRefresh;
  mResponse.clear();

  QUrl modifiedUrl( url );

  // Specific code for testing
  if ( modifiedUrl.toString().contains( QLatin1String( "fake_qgis_http_endpoint" ) ) )
  {
    // Just for testing with local files instead of http:// resources
    QString modifiedUrlString = modifiedUrl.toString();
    // Qt5 does URL encoding from some reason (of the FILTER parameter for example)
    modifiedUrlString = QUrl::fromPercentEncoding( modifiedUrlString.toUtf8() );
    QgsDebugMsg( QString( "Get %1" ).arg( modifiedUrlString ) );
    modifiedUrlString = modifiedUrlString.mid( QStringLiteral( "http://" ).size() );
    QString args = modifiedUrlString.mid( modifiedUrlString.indexOf( '?' ) );
    if ( modifiedUrlString.size() > 256 )
    {
      args = QCryptographicHash::hash( args.toUtf8(), QCryptographicHash::Md5 ).toHex();
    }
    else
    {
      args.replace( QLatin1String( "?" ), QLatin1String( "_" ) );
      args.replace( QLatin1String( "&" ), QLatin1String( "_" ) );
      args.replace( QLatin1String( "<" ), QLatin1String( "_" ) );
      args.replace( QLatin1String( ">" ), QLatin1String( "_" ) );
      args.replace( QLatin1String( "'" ), QLatin1String( "_" ) );
      args.replace( QLatin1String( "\"" ), QLatin1String( "_" ) );
      args.replace( QLatin1String( " " ), QLatin1String( "_" ) );
      args.replace( QLatin1String( ":" ), QLatin1String( "_" ) );
      args.replace( QLatin1String( "/" ), QLatin1String( "_" ) );
      args.replace( QLatin1String( "\n" ), QLatin1String( "_" ) );
    }
#ifdef Q_OS_WIN
    // Passing "urls" like "http://c:/path" to QUrl 'eats' the : after c,
    // so we must restore it
    if ( modifiedUrlString[1] == '/' )
    {
      modifiedUrlString = modifiedUrlString[0] + ":/" + modifiedUrlString.mid( 2 );
    }
#endif
    modifiedUrlString = modifiedUrlString.mid( 0, modifiedUrlString.indexOf( '?' ) ) + args;
    QgsDebugMsg( QStringLiteral( "Get %1 (after laundering)" ).arg( modifiedUrlString ) );
    modifiedUrl = QUrl::fromLocalFile( modifiedUrlString );
  }

  QgsDebugMsgLevel( QStringLiteral( "Calling: %1" ).arg( modifiedUrl.toDisplayString( ) ), 4 );

  QNetworkRequest request( modifiedUrl );
  if ( !mUri.auth().setAuthorization( request ) )
  {
    mErrorCode = QgsWfsRequest::NetworkError;
    mErrorMessage = errorMessageFailedAuth();
    QgsMessageLog::logMessage( mErrorMessage, tr( "WFS" ) );
    return false;
  }

  if ( cache )
  {
    request.setAttribute( QNetworkRequest::CacheLoadControlAttribute, forceRefresh ? QNetworkRequest::AlwaysNetwork : QNetworkRequest::PreferCache );
    request.setAttribute( QNetworkRequest::CacheSaveControlAttribute, true );
  }

  QWaitCondition waitCondition;
  QMutex waitConditionMutex;

  std::function<bool()> downloaderFunction = [ this, request, synchronous, &waitConditionMutex, &waitCondition ]()
  {
    if ( QThread::currentThread() != QgsApplication::instance()->thread() )
      QgsNetworkAccessManager::instance( Qt::DirectConnection );

    bool success = true;
    mReply = QgsNetworkAccessManager::instance()->get( request );

    mReply->setReadBufferSize( READ_BUFFER_SIZE_HINT );
    if ( !mUri.auth().setAuthorizationReply( mReply ) )
    {
      mErrorCode = QgsWfsRequest::NetworkError;
      mErrorMessage = errorMessageFailedAuth();
      QgsMessageLog::logMessage( mErrorMessage, tr( "WFS" ) );
      waitCondition.wakeAll();
      success = false;
    }
    else
    {
      // We are able to use direct connection here, because we
      // * either run on the thread mReply lives in, so DirectConnection is standard and safe anyway
      // * or the owner thread of mReply is currently not doing anything because it's blocked in future.waitForFinished() (if it is the main thread)
      connect( mReply, &QNetworkReply::finished, this, &QgsWfsRequest::replyFinished, Qt::DirectConnection );
      connect( mReply, &QNetworkReply::downloadProgress, this, &QgsWfsRequest::replyProgress, Qt::DirectConnection );

      if ( synchronous )
      {
        auto resumeMainThread = [&waitConditionMutex, &waitCondition]()
        {
          waitConditionMutex.lock();
          waitCondition.wakeAll();
          waitConditionMutex.unlock();

          waitConditionMutex.lock();
          waitCondition.wait( &waitConditionMutex );
          waitConditionMutex.unlock();
        };

        connect( QgsNetworkAccessManager::instance(), &QgsNetworkAccessManager::authenticationRequired, this, resumeMainThread, Qt::DirectConnection );
        connect( QgsNetworkAccessManager::instance(), &QgsNetworkAccessManager::proxyAuthenticationRequired, this, resumeMainThread, Qt::DirectConnection );

#ifndef QT_NO_SSL
        connect( QgsNetworkAccessManager::instance(), &QgsNetworkAccessManager::sslErrors, this, resumeMainThread, Qt::DirectConnection );
#endif
        QEventLoop loop;
        connect( this, &QgsWfsRequest::downloadFinished, &loop, &QEventLoop::quit, Qt::DirectConnection );
        loop.exec();
      }
    }
    waitCondition.wakeAll();
    return success;
  };

  bool success;

  if ( synchronous && QThread::currentThread() == QApplication::instance()->thread() )
  {
    std::unique_ptr<DownloaderThread> downloaderThread = qgis::make_unique<DownloaderThread>( downloaderFunction );
    downloaderThread->start();

    while ( !downloaderThread->isFinished() )
    {
      waitConditionMutex.lock();
      waitCondition.wait( &waitConditionMutex );
      waitConditionMutex.unlock();

      // If the downloader thread wakes us (the main thread) up and is not yet finished
      // he needs the authentication to run.
      // The processEvents() call gives the auth manager the chance to show a dialog and
      // once done with that, we can wake the downloaderThread again and continue the download.
      if ( !downloaderThread->isFinished() )
      {
        QgsApplication::instance()->processEvents();
        waitConditionMutex.lock();
        waitCondition.wakeAll();
        waitConditionMutex.unlock();
      }
    }

    success = downloaderThread->success();
  }
  else
  {
    success = downloaderFunction();
  }
  return success && mErrorMessage.isEmpty();
}

bool QgsWfsRequest::sendPOST( const QUrl &url, const QString &contentTypeHeader, const QByteArray &data )
{
  abort(); // cancel previous
  mIsAborted = false;
  mTimedout = false;
  mGotNonEmptyResponse = false;

  mErrorMessage.clear();
  mErrorCode = QgsWfsRequest::NoError;
  mForceRefresh = true;
  mResponse.clear();

  if ( url.toEncoded().contains( "fake_qgis_http_endpoint" ) )
  {
    // Hack for testing purposes
    QUrl modifiedUrl( url );
    modifiedUrl.addQueryItem( QStringLiteral( "POSTDATA" ), QString::fromUtf8( data ) );
    return sendGET( modifiedUrl, true, true, false );
  }

  QNetworkRequest request( url );
  if ( !mUri.auth().setAuthorization( request ) )
  {
    mErrorCode = QgsWfsRequest::NetworkError;
    mErrorMessage = errorMessageFailedAuth();
    QgsMessageLog::logMessage( mErrorMessage, tr( "WFS" ) );
    return false;
  }
  request.setHeader( QNetworkRequest::ContentTypeHeader, contentTypeHeader );

  mReply = QgsNetworkAccessManager::instance()->post( request, data );
  mReply->setReadBufferSize( READ_BUFFER_SIZE_HINT );
  if ( !mUri.auth().setAuthorizationReply( mReply ) )
  {
    mErrorCode = QgsWfsRequest::NetworkError;
    mErrorMessage = errorMessageFailedAuth();
    QgsMessageLog::logMessage( mErrorMessage, tr( "WFS" ) );
    return false;
  }
  connect( mReply, &QNetworkReply::finished, this, &QgsWfsRequest::replyFinished );
  connect( mReply, &QNetworkReply::downloadProgress, this, &QgsWfsRequest::replyProgress );

  QEventLoop loop;
  connect( this, &QgsWfsRequest::downloadFinished, &loop, &QEventLoop::quit );
  loop.exec( QEventLoop::ExcludeUserInputEvents );

  return mErrorMessage.isEmpty();
}

void QgsWfsRequest::abort()
{
  mIsAborted = true;
  if ( mReply )
  {
    mReply->deleteLater();
    mReply = nullptr;
  }
}

void QgsWfsRequest::replyProgress( qint64 bytesReceived, qint64 bytesTotal )
{
  QgsDebugMsg( QStringLiteral( "%1 of %2 bytes downloaded." ).arg( bytesReceived ).arg( bytesTotal < 0 ? QString( "unknown number of" ) : QString::number( bytesTotal ) ) );

  if ( bytesReceived != 0 )
    mGotNonEmptyResponse = true;

  if ( !mIsAborted && mReply )
  {
    if ( mReply->error() == QNetworkReply::NoError )
    {
      QVariant redirect = mReply->attribute( QNetworkRequest::RedirectionTargetAttribute );
      if ( !redirect.isNull() )
      {
        // We don't want to emit downloadProgress() for a redirect
        return;
      }
    }
  }

  emit downloadProgress( bytesReceived, bytesTotal );
}

void QgsWfsRequest::replyFinished()
{
  if ( !mIsAborted && mReply )
  {
    if ( mReply->error() == QNetworkReply::NoError )
    {
      QgsDebugMsg( "reply OK" );
      QVariant redirect = mReply->attribute( QNetworkRequest::RedirectionTargetAttribute );
      if ( !redirect.isNull() )
      {
        QgsDebugMsg( "Request redirected." );

        const QUrl &toUrl = redirect.toUrl();
        mReply->request();
        if ( toUrl == mReply->url() )
        {
          mErrorMessage = tr( "Redirect loop detected: %1" ).arg( toUrl.toString() );
          QgsMessageLog::logMessage( mErrorMessage, tr( "WFS" ) );
          mResponse.clear();
        }
        else
        {
          QNetworkRequest request( toUrl );
          if ( !mUri.auth().setAuthorization( request ) )
          {
            mResponse.clear();
            mErrorMessage = errorMessageFailedAuth();
            mErrorCode = QgsWfsRequest::NetworkError;
            QgsMessageLog::logMessage( mErrorMessage, tr( "WFS" ) );
            emit downloadFinished();
            return;
          }
          request.setAttribute( QNetworkRequest::CacheLoadControlAttribute, mForceRefresh ? QNetworkRequest::AlwaysNetwork : QNetworkRequest::PreferCache );
          request.setAttribute( QNetworkRequest::CacheSaveControlAttribute, true );

          mReply->deleteLater();
          mReply = nullptr;

          QgsDebugMsg( QString( "redirected: %1 forceRefresh=%2" ).arg( redirect.toString() ).arg( mForceRefresh ) );
          mReply = QgsNetworkAccessManager::instance()->get( request );
          mReply->setReadBufferSize( READ_BUFFER_SIZE_HINT );
          if ( !mUri.auth().setAuthorizationReply( mReply ) )
          {
            mResponse.clear();
            mErrorMessage = errorMessageFailedAuth();
            mErrorCode = QgsWfsRequest::NetworkError;
            QgsMessageLog::logMessage( mErrorMessage, tr( "WFS" ) );
            emit downloadFinished();
            return;
          }
          connect( mReply, &QNetworkReply::finished, this, &QgsWfsRequest::replyFinished );
          connect( mReply, &QNetworkReply::downloadProgress, this, &QgsWfsRequest::replyProgress );
          return;
        }
      }
      else
      {
        const QgsNetworkAccessManager *nam = QgsNetworkAccessManager::instance();

        if ( nam->cache() )
        {
          QNetworkCacheMetaData cmd = nam->cache()->metaData( mReply->request().url() );

          QNetworkCacheMetaData::RawHeaderList hl;
          Q_FOREACH ( const QNetworkCacheMetaData::RawHeader &h, cmd.rawHeaders() )
          {
            if ( h.first != "Cache-Control" )
              hl.append( h );
          }
          cmd.setRawHeaders( hl );

          QgsDebugMsg( QString( "expirationDate:%1" ).arg( cmd.expirationDate().toString() ) );
          if ( cmd.expirationDate().isNull() )
          {
            cmd.setExpirationDate( QDateTime::currentDateTime().addSecs( defaultExpirationInSec() ) );
          }

          nam->cache()->updateMetaData( cmd );
        }
        else
        {
          QgsDebugMsg( "No cache!" );
        }

#ifdef QGISDEBUG
        bool fromCache = mReply->attribute( QNetworkRequest::SourceIsFromCacheAttribute ).toBool();
        QgsDebugMsg( QString( "Reply was cached: %1" ).arg( fromCache ) );
#endif

        mResponse = mReply->readAll();

        if ( mResponse.isEmpty() && !mGotNonEmptyResponse )
        {
          mErrorMessage = tr( "empty response: %1" ).arg( mReply->errorString() );
          mErrorCode = QgsWfsRequest::ServerExceptionError;
          QgsMessageLog::logMessage( mErrorMessage, tr( "WFS" ) );
        }
      }
    }
    else
    {
      mErrorMessage = errorMessageWithReason( mReply->errorString() );
      mErrorCode = QgsWfsRequest::ServerExceptionError;
      QgsMessageLog::logMessage( mErrorMessage, tr( "WFS" ) );
      mResponse.clear();
    }
  }
  if ( mTimedout )
    mErrorCode = QgsWfsRequest::TimeoutError;

  if ( mReply )
  {
    mReply->deleteLater();
    mReply = nullptr;
  }

  emit downloadFinished();
}

QString QgsWfsRequest::errorMessageFailedAuth()
{
  return errorMessageWithReason( tr( "network request update failed for authentication config" ) );
}
