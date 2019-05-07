/***************************************************************************
      qgsgpxprovider.h  -  Data provider for GPS eXchange files
                             -------------------
    begin                : 2004-04-14
    copyright            : (C) 2004 by Lars Luthman
    email                : larsl@users.sourceforge.net

    Partly based on qgsdelimitedtextprovider.h, (C) 2004 Gary E. Sherman
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSGPXPROVIDER_H
#define QGSGPXPROVIDER_H

#include "qgsvectordataprovider.h"
#include "gpsdata.h"
#include "qgsfields.h"

class QgsFeature;
class QgsField;
class QFile;
class QDomDocument;
class QgsGpsData;

class QgsGPXFeatureIterator;

/**
\class QgsGPXProvider
\brief Data provider for GPX (GPS eXchange) files
* This provider adds the ability to load GPX files as vector layers.
*
*/
class QgsGPXProvider : public QgsVectorDataProvider
{
    Q_OBJECT

  public:
    explicit QgsGPXProvider( const QString &uri, const QgsDataProvider::ProviderOptions &options );
    ~QgsGPXProvider() override;

    /* Functions inherited from QgsVectorDataProvider */

    QgsAbstractFeatureSource *featureSource() const override;
    QString storageType() const override;
    QgsFeatureIterator getFeatures( const QgsFeatureRequest &request ) const override;
    QgsWkbTypes::Type wkbType() const override;
    long featureCount() const override;
    QgsFields fields() const override;
    bool addFeatures( QgsFeatureList &flist, QgsFeatureSink::Flags flags = nullptr ) override;
    bool deleteFeatures( const QgsFeatureIds &id ) override;
    bool changeAttributeValues( const QgsChangedAttributesMap &attr_map ) override;
    QgsVectorDataProvider::Capabilities capabilities() const override;
    QVariant defaultValue( int fieldId ) const override;


    /* Functions inherited from QgsDataProvider */

    QgsRectangle extent() const override;
    bool isValid() const override;
    QString name() const override;
    QString description() const override;
    QgsCoordinateReferenceSystem crs() const override;


    /* new functions */

    void changeAttributeValues( QgsGpsObject &obj,
                                const QgsAttributeMap &attrs );

    bool addFeature( QgsFeature &f, QgsFeatureSink::Flags flags = nullptr ) override;


    enum DataType
    {
      WaypointType = 1,
      RouteType = 2,
      TrackType = 4,

      TrkRteType = RouteType | TrackType,
      AllType = WaypointType | RouteType | TrackType

    };

    enum Attribute { NameAttr = 0, EleAttr, SymAttr, NumAttr,
                     CmtAttr, DscAttr, SrcAttr, URLAttr, URLNameAttr
                   };

  private:

    QgsGpsData *data = nullptr;

    //! Fields
    QgsFields attributeFields;
    //! map from field index to attribute
    QVector<int> indexToAttr;

    QString mFileName;

    DataType mFeatureType = WaypointType;

    static const char *ATTR[];
    static QVariant::Type attrType[];
    static DataType attrUsed[];
    static const int ATTR_COUNT;

    bool mValid = false;

    friend class QgsGPXFeatureSource;
};

#endif
