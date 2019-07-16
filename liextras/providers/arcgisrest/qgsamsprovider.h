/***************************************************************************
    qgsamsprovider.h - ArcGIS MapServer Raster Provider
     --------------------------------------------------
    Date                 : Nov 24, 2015
    Copyright            : (C) 2015 by Sandro Mani
    email                : manisandro@gmail.com

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSMAPSERVERPROVIDER_H
#define QGSMAPSERVERPROVIDER_H

#include "qgsrasterdataprovider.h"
#include "qgscoordinatereferencesystem.h"
#include "tilingscheme.h"
#include "liproviderinterface.h"
#include "qgsmapprojection.h"

class QgsArcGisAsyncQuery;
class QgsAmsProvider;

class QgsAmsLegendFetcher : public QgsImageFetcher
{
    Q_OBJECT
public:
    QgsAmsLegendFetcher( QgsAmsProvider *provider );
    void start() override;
    bool haveImage() const { return mLegendImage.isNull(); }
    QImage getImage() const { return mLegendImage; }
    const QString &errorTitle() const { return mErrorTitle; }
    const QString &errorMessage() const { return mError; }

private slots:
    void handleFinished();
    void handleError( const QString &errorTitle, const QString &errorMsg );

private:
    QgsAmsProvider *mProvider = nullptr;
    QgsArcGisAsyncQuery *mQuery = nullptr;
    QByteArray mQueryReply;
    QImage mLegendImage;
    QString mErrorTitle;
    QString mError;
};

class QgsAmsProvider :
        public QgsRasterDataProvider,
        public TilingScheme,
        public LiProviderInterface
{
    Q_OBJECT

public:
    explicit QgsAmsProvider( const QString &uri, const QgsDataProvider::ProviderOptions &options = QgsDataProvider::ProviderOptions() );

    /* Inherited from QgsDataProvider */
    bool isValid() const override { return mValid; }
    QString name() const override { return QStringLiteral( "mapserver" ); }
    QString description() const override { return QStringLiteral( "ArcGIS MapServer data provider" ); }
    QgsCoordinateReferenceSystem crs() const override { return mCrs; }
    uint subLayerCount() const override { return mSubLayers.size(); }
    QStringList subLayers() const override { return mSubLayers; }
    QStringList subLayerStyles() const override;
    void setLayerOrder( const QStringList &layers ) override;
    void setSubLayerVisibility( const QString &name, bool vis ) override;
    void reloadData() override;

    /* Inherited from QgsRasterInterface */
    int bandCount() const override { return 1; }
    int capabilities() const override { return Identify | IdentifyText | IdentifyFeature; }

    /* Inherited from QgsRasterDataProvider */
    QgsRectangle extent() const override { return mExtent; }
    QString lastErrorTitle() override { return mErrorTitle; }
    QString lastError() override { return mError; }
    Qgis::DataType dataType( int /*bandNo*/ ) const override { return Qgis::ARGB32; }
    Qgis::DataType sourceDataType( int /*bandNo*/ ) const override { return Qgis::ARGB32; }
    QgsRasterInterface *clone() const override;
    QString htmlMetadata() override;
    bool supportsLegendGraphic() const override { return true; }
    QImage getLegendGraphic( double scale = 0, bool forceRefresh = false, const QgsRectangle *visibleExtent = nullptr ) override;
    QgsImageFetcher *getLegendGraphicFetcher( const QgsMapSettings *mapSettings ) override;
    QgsRasterIdentifyResult identify( const QgsPointXY &point, QgsRaster::IdentifyFormat format, const QgsRectangle &extent = QgsRectangle(), int width = 0, int height = 0, int dpi = 96 ) override;

    // tiling scheme
    bool isGeographic() const { return !mMapProjection->useWebMercator(); }
    TilingScheme *tilingScheme() { return mTilingScheme; }
    int getNumberOfXTilesAtLevel(int level) const;
    int getNumberOfYTilesAtLevel(int level) const;
    LiRectangle rectangle() const { return mExtent84; }
    MapProjection *projection() const { return mMapProjection.data(); }
    LiRectangle tileXYToRectangle(int x, int y, int level) const;
    LiRectangle tileXYToNativeRectangle(int x, int y, int level) const;
    LiRectangle rectangleToNativeRectangle(const LiRectangle &rectangle) const;
    Cartesian2 positionToTileXY(const Cartographic &position, int level) const;

    // provider interface
    LiProviderInterface *providerInterface() { return this; }
    bool isTiled() const;
    int tileWidth() const;
    int tileHeight() const;
    int minimumLevel() const;
    int maximumLevel() const;
    int computeLevel(const LiRectangle &rectangle) const;
    QUrl getTileUrl(int x, int y, int level);

protected:
//    void readBlock( int bandNo, const QgsRectangle &viewExtent, int width, int height, void *data, QgsRasterBlockFeedback *feedback = nullptr ) override;

    void draw( const QgsRectangle &viewExtent, int pixelWidth, int pixelHeight );

private:
    double getResolution(int level) const;

    struct LodEntry
    {
        int level;
        double resolution;
    };

    struct TileInfo
    {
        double x;
        double y;
        int tileWidth;
        int tileHeight;
        QList<LodEntry> lods;
    };

    struct ServiceInfo
    {
        QString url;
        QByteArray format;
        TileInfo tileInfo;
        bool singleFusedMapCache = false;
    };

    ServiceInfo mInfo;
    bool mValid = false;
    TilingScheme *mTilingScheme;
    QgsAmsLegendFetcher *mLegendFetcher = nullptr;
    QVariantMap mServiceInfo;
    QVariantMap mLayerInfo;
    QgsCoordinateReferenceSystem mCrs;
    QgsCoordinateTransform mTransform;
    QgsRectangle mExtent;
    LiRectangle mExtent84;
    QStringList mSubLayers;
    QList<bool> mSubLayerVisibilities;
    QString mErrorTitle;
    QString mError;
    QImage mCachedImage;
    QgsRectangle mCachedImageExtent;
    QSharedPointer<QgsMapProjection> mMapProjection;
};

#endif // QGSMAPSERVERPROVIDER_H
