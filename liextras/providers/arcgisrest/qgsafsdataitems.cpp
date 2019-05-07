/***************************************************************************
      qgsafsdataitems.cpp
      -------------------
    begin                : Jun 03, 2015
    copyright            : (C) 2015 by Sandro Mani
    email                : smani@sourcepole.ch
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgslogger.h"
#include "qgsowsconnection.h"
#include "qgsafsdataitems.h"
#include "qgsafsprovider.h"
#include "qgsarcgisrestutils.h"

#ifdef HAVE_GUI
#include "qgsnewhttpconnection.h"
#include "qgsafssourceselect.h"
#endif

#include <QCoreApplication>
#include <QSettings>
#include <QUrl>


QgsAfsRootItem::QgsAfsRootItem( QgsDataItem *parent, const QString &name, const QString &path )
  : QgsDataCollectionItem( parent, name, path )
{
  mCapabilities |= Fast;
  mIconName = QStringLiteral( "mIconAfs.svg" );
  populate();
}

QVector<QgsDataItem *> QgsAfsRootItem::createChildren()
{
  QVector<QgsDataItem *> connections;

  for ( const QString &connName : QgsOwsConnection::connectionList( "arcgisfeatureserver" ) )
  {
    QgsOwsConnection connection( QStringLiteral( "arcgisfeatureserver" ), connName );
    QString path = "afs:/" + connName;
    connections.append( new QgsAfsConnectionItem( this, connName, path, connection.uri().param( QStringLiteral( "url" ) ) ) );
  }
  return connections;
}

#ifdef HAVE_GUI
QList<QAction *> QgsAfsRootItem::actions( QWidget *parent )
{
  QAction *actionNew = new QAction( tr( "New Connection" ), parent );
  connect( actionNew, &QAction::triggered, this, &QgsAfsRootItem::newConnection );
  return QList<QAction *>() << actionNew;
}

QWidget *QgsAfsRootItem::paramWidget()
{
  QgsAfsSourceSelect *select = new QgsAfsSourceSelect( nullptr, nullptr, QgsProviderRegistry::WidgetMode::Manager );
  connect( select, &QgsArcGisServiceSourceSelect::connectionsChanged, this, &QgsAfsRootItem::onConnectionsChanged );
  return select;
}

void QgsAfsRootItem::onConnectionsChanged()
{
  refresh();
}

void QgsAfsRootItem::newConnection()
{
  QgsNewHttpConnection nc( nullptr, QgsNewHttpConnection::ConnectionOther, QStringLiteral( "qgis/connections-arcgisfeatureserver/" ) );
  nc.setWindowTitle( tr( "Create a New ArcGIS Feature Server Connection" ) );

  if ( nc.exec() )
  {
    refresh();
  }
}
#endif

///////////////////////////////////////////////////////////////////////////////

QgsAfsConnectionItem::QgsAfsConnectionItem( QgsDataItem *parent, const QString &name, const QString &path, const QString &url )
  : QgsDataCollectionItem( parent, name, path )
  , mUrl( url )
{
  mIconName = QStringLiteral( "mIconConnect.svg" );
  mCapabilities |= Collapse;
}

QVector<QgsDataItem *> QgsAfsConnectionItem::createChildren()
{
  QVector<QgsDataItem *> layers;
  QString errorTitle, errorMessage;
  const QVariantMap serviceData = QgsArcGisRestUtils::getServiceInfo( mUrl, errorTitle, errorMessage );
  if ( serviceData.isEmpty() )
  {
    return layers;
  }
  const QString authid = QgsArcGisRestUtils::parseSpatialReference( serviceData.value( QStringLiteral( "spatialReference" ) ).toMap() ).authid();

  const QVariantList layerInfoList = serviceData[QStringLiteral( "layers" )].toList();
  for ( const QVariant &layerInfo : layerInfoList )
  {
    const QVariantMap layerInfoMap = layerInfo.toMap();
    if ( !layerInfoMap.value( QStringLiteral( "subLayerIds" ) ).toList().empty() )
    {
      // group layer - do not show as it is not possible to load
      // TODO - show nested groups
      continue;
    }
    const QString id = layerInfoMap.value( QStringLiteral( "id" ) ).toString();
    QgsAfsLayerItem *layer = new QgsAfsLayerItem( this, mName, mUrl + "/" + id, layerInfoMap.value( QStringLiteral( "name" ) ).toString(), authid );
    layers.append( layer );
  }

  return layers;
}

bool QgsAfsConnectionItem::equal( const QgsDataItem *other )
{
  const QgsAfsConnectionItem *o = dynamic_cast<const QgsAfsConnectionItem *>( other );
  return ( type() == other->type() && o && mPath == o->mPath && mName == o->mName );
}

#ifdef HAVE_GUI
QList<QAction *> QgsAfsConnectionItem::actions( QWidget *parent )
{
  QList<QAction *> lst;

  QAction *actionEdit = new QAction( tr( "Edit" ), parent );
  connect( actionEdit, &QAction::triggered, this, &QgsAfsConnectionItem::editConnection );
  lst.append( actionEdit );

  QAction *actionDelete = new QAction( tr( "Delete" ), parent );
  connect( actionDelete, &QAction::triggered, this, &QgsAfsConnectionItem::deleteConnection );
  lst.append( actionDelete );

  return lst;
}

void QgsAfsConnectionItem::editConnection()
{
  QgsNewHttpConnection nc( nullptr, QgsNewHttpConnection::ConnectionOther, QStringLiteral( "qgis/connections-arcgisfeatureserver/" ), mName );
  nc.setWindowTitle( tr( "Modify ArcGIS Feature Server Connection" ) );

  if ( nc.exec() )
  {
    mParent->refresh();
  }
}

void QgsAfsConnectionItem::deleteConnection()
{
  QgsOwsConnection::deleteConnection( QStringLiteral( "arcgisfeatureserver" ), mName );
  mParent->refresh();
}
#endif

///////////////////////////////////////////////////////////////////////////////

QgsAfsLayerItem::QgsAfsLayerItem( QgsDataItem *parent, const QString &name, const QString &url, const QString &title, const QString &authid )
  : QgsLayerItem( parent, title, parent->path() + "/" + name, QString(), QgsLayerItem::Vector, QStringLiteral( "arcgisfeatureserver" ) )
{
  mUri = QStringLiteral( "crs='%1' url='%2'" ).arg( authid, url );
  setState( Populated );
  mIconName = QStringLiteral( "mIconAfs.svg" );
}
