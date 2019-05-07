/***************************************************************************
      qgsoracleprovider.h  -  Data provider for oracle layers
                             -------------------
    begin                : August 2012
    copyright            : (C) 2012 by Juergen E. Fischer
    email                : jef at norbit dot de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef QGSORACLEPROVIDER_H
#define QGSORACLEPROVIDER_H

#include "qgsvectordataprovider.h"
#include "qgsrectangle.h"
#include "qgsvectorlayerexporter.h"
#include "qgsoracletablemodel.h"
#include "qgsdatasourceuri.h"
#include "qgsfields.h"
#include "qgsproviderregistry.h"

#include <QVector>
#include <QQueue>
#include <QSqlQuery>
#include <QSqlError>
#include <QMutex>

class QgsFeature;
class QgsField;
class QgsGeometry;
class QgsOracleFeatureIterator;
class QgsOracleSharedData;

enum QgsOraclePrimaryKeyType
{
  PktUnknown,
  PktInt,
  PktRowId,
  PktFidMap
};

/**
  \class QgsOracleProvider
  \brief Data provider for oracle layers.

  This provider implements the
  interface defined in the QgsDataProvider class to provide access to spatial
  data residing in a oracle enabled database.
  */
class QgsOracleProvider : public QgsVectorDataProvider
{
    Q_OBJECT
    Q_PROPERTY( QString workspace READ getWorkspace WRITE setWorkspace )

  public:

    //! Import a vector layer into the database
    static QgsVectorLayerExporter::ExportError createEmptyLayer(
      const QString &uri,
      const QgsFields &fields,
      QgsWkbTypes::Type wkbType,
      const QgsCoordinateReferenceSystem *srs,
      bool overwrite,
      QMap<int, int> *oldToNewAttrIdxMap,
      QString *errorMessage = nullptr,
      const QMap<QString, QVariant> *options = nullptr
    );

    /**
     * Constructor for the provider. The uri must be in the following format:
     * host=localhost user=gsherman dbname=test password=xxx table=test.alaska (the_geom)
     * \param uri String containing the required parameters to connect to the database
     * and query the table.
     * \param options generic data provider options
     */
    explicit QgsOracleProvider( QString const &uri, const QgsDataProvider::ProviderOptions &options );

    //! Destructor
    ~QgsOracleProvider() override;

    QgsAbstractFeatureSource *featureSource() const override;
    QString storageType() const override;
    QgsCoordinateReferenceSystem crs() const override;
    QgsWkbTypes::Type wkbType() const override;

    /**
     * Returns the number of layers for the current data source
     * \note Should this be subLayerCount() instead?
     */
    size_t layerCount() const;

    long featureCount() const override;

    /**
     * Gets the number of fields in the layer
     */
    uint fieldCount() const;

    /**
     * Returns a string representation of the endian-ness for the layer
     */
    QString endianString();

    /**
     * Changes the stored extent for this layer to the supplied extent.
     * For example, this is called when the extent worker thread has a result.
     */
    void setExtent( QgsRectangle &newExtent );

    QgsRectangle extent() const override;
    void updateExtents() override;

    /**
     * Determine the fields making up the primary key
     */
    bool determinePrimaryKey();

    QgsFields fields() const override;
    QString dataComment() const override;

    /**
     * Reset the layer
     */
    void rewind();

    QVariant minimumValue( int index ) const override;
    QVariant maximumValue( int index ) const override;
    QSet<QVariant> uniqueValues( int index, int limit = -1 ) const override;
    bool isValid() const override;
    QgsAttributeList pkAttributeIndexes() const override { return mPrimaryKeyAttrs; }
    QVariant defaultValue( QString fieldName, QString tableName = QString(), QString schemaName = QString() );
    QVariant defaultValue( int fieldId ) const override;
    bool addFeatures( QgsFeatureList &flist, QgsFeatureSink::Flags flags = nullptr ) override;
    bool deleteFeatures( const QgsFeatureIds &id ) override;
    bool addAttributes( const QList<QgsField> &attributes ) override;
    bool deleteAttributes( const QgsAttributeIds &ids ) override;
    bool renameAttributes( const QgsFieldNameMap &renamedAttributes ) override;
    bool changeAttributeValues( const QgsChangedAttributesMap &attr_map ) override;
    bool changeGeometryValues( const QgsGeometryMap &geometry_map ) override;
    bool createSpatialIndex() override;

    //! Gets the table name associated with this provider instance
    QString getTableName();

    QString subsetString() const override;
    bool setSubsetString( const QString &theSQL, bool updateFeatureCount = true ) override;
    bool supportsSubsetString() const override { return true; }
    QgsVectorDataProvider::Capabilities capabilities() const override;
    QString name() const override;
    QString description() const override;
    QgsFeatureIterator getFeatures( const QgsFeatureRequest &request = QgsFeatureRequest() ) const override;

    static bool exec( QSqlQuery &qry, QString sql, const QVariantList &args );

    bool isSaveAndLoadStyleToDatabaseSupported() const override { return true; }

    /**
     * Switch to oracle workspace
     */
    void setWorkspace( const QString &workspace );

    /**
     * Retrieve oracle workspace name
     */
    QString getWorkspace() const;

  private:
    QString whereClause( QgsFeatureId featureId, QVariantList &args ) const;
    QString pkParamWhereClause() const;
    QString paramValue( QString fieldvalue, const QString &defaultValue ) const;
    void appendGeomParam( const QgsGeometry &geom, QSqlQuery &qry ) const;
    void appendPkParams( QgsFeatureId fid, QSqlQuery &qry ) const;

    bool hasSufficientPermsAndCapabilities();

    QgsField field( int index ) const;

    /**
     * Load the field list
     */
    bool loadFields();

    //! Convert a QgsField to work with Oracle
    static bool convertField( QgsField &field );

    QgsFields mAttributeFields;  //! List of fields
    QVariantList mDefaultValues; //! List of default values
    QString mDataComment;

    //! Data source URI struct for this layer
    QgsDataSourceUri mUri;

    /**
     * Flag indicating if the layer data source is a valid oracle layer
     */
    bool mValid;

    /**
     * provider references query (instead of a table)
     */
    bool mIsQuery;

    /**
     * Name of the table with no schema
     */
    QString mTableName;

    /**
     * Name of the table or subquery
     */
    QString mQuery;

    /**
     * Owner of the table
     */
    QString mOwnerName;

    /**
     * SQL statement used to limit the features retrieved
     */
    QString mSqlWhereClause;

    /**
     * Data type for the primary key
     */
    QgsOraclePrimaryKeyType mPrimaryKeyType;

    /**
     * List of primary key attributes for fetching features.
     */
    QList<int> mPrimaryKeyAttrs;
    QString mPrimaryKeyDefault;

    QString mGeometryColumn;           //! name of the geometry column
    mutable QgsRectangle mLayerExtent; //! Rectangle that contains the extent (bounding box) of the layer
    mutable long mFeaturesCounted;     //! Number of features in the layer
    int mSrid;                         //! srid of column
    QgsVectorDataProvider::Capabilities mEnabledCapabilities;          //! capabilities of layer

    QgsWkbTypes::Type mDetectedGeomType;   //! geometry type detected in the database
    QgsWkbTypes::Type mRequestedGeomType;  //! geometry type requested in the uri

    bool getGeometryDetails();

    /* Use estimated metadata. Uses fast table counts, geometry type and extent determination */
    bool mUseEstimatedMetadata;

    /* Include additional geo attributes */
    bool mIncludeGeoAttributes;

    struct OracleFieldNotFound {}; //! Exception to throw

    struct OracleException
    {
        OracleException( QString msg, const QSqlQuery &q )
          : mWhat( tr( "Oracle error: %1\nSQL: %2\nError: %3" )
                   .arg( msg )
                   .arg( q.lastError().text() )
                   .arg( q.lastQuery() )
                 )
        {}

        OracleException( QString msg, const QSqlDatabase &q )
          : mWhat( tr( "Oracle error: %1\nError: %2" )
                   .arg( msg )
                   .arg( q.lastError().text() )
                 )
        {}

        OracleException( const OracleException &e )
          : mWhat( e.errorMessage() )
        {}

        ~OracleException()
          = default;

        QString errorMessage() const
        {
          return mWhat;
        }

      private:
        QString mWhat;
    };

    // A function that determines if the given schema.table.column
    // contains unqiue entries
    bool uniqueData( QString query, QString colName );

    void disconnectDb();

    static QString quotedIdentifier( QString ident ) { return QgsOracleConn::quotedIdentifier( ident ); }
    static QString quotedValue( const QVariant &value, QVariant::Type type = QVariant::Invalid ) { return QgsOracleConn::quotedValue( value, type ); }

    QMap<QVariant, QgsFeatureId> mKeyToFid;  //! map key values to feature id
    QMap<QgsFeatureId, QVariant> mFidToKey;  //! map feature back to fea
    QgsOracleConn *mConnection = nullptr;

    bool mHasSpatialIndex;                   //! Geometry column is indexed
    QString mSpatialIndexName;               //! name of spatial index of geometry column

    std::shared_ptr<QgsOracleSharedData> mShared;

    friend class QgsOracleFeatureIterator;
    friend class QgsOracleFeatureSource;
};


//! Assorted Oracle utility functions
class QgsOracleUtils
{
  public:
    static QString whereClause( QgsFeatureId featureId,
                                const QgsFields &fields,
                                QgsOraclePrimaryKeyType primaryKeyType,
                                const QList<int> &primaryKeyAttrs,
                                std::shared_ptr<QgsOracleSharedData> sharedData,
                                QVariantList &params );

    static QString whereClause( QgsFeatureIds featureIds,
                                const QgsFields &fields,
                                QgsOraclePrimaryKeyType primaryKeyType,
                                const QList<int> &primaryKeyAttrs,
                                std::shared_ptr<QgsOracleSharedData> sharedData,
                                QVariantList &params );

    static QString andWhereClauses( const QString &c1, const QString &c2 );
};


/**
 * Data shared between provider class and its feature sources. Ideally there should
 *  be as few members as possible because there could be simultaneous reads/writes
 *  from different threads and therefore locking has to be involved. */
class QgsOracleSharedData
{
  public:
    QgsOracleSharedData() = default;

    // FID lookups
    QgsFeatureId lookupFid( const QVariantList &v ); // lookup existing mapping or add a new one
    QVariant removeFid( QgsFeatureId fid );
    void insertFid( QgsFeatureId fid, const QVariantList &k );
    QVariantList lookupKey( QgsFeatureId featureId );

  protected:
    QMutex mMutex; //!< Access to all data members is guarded by the mutex

    QgsFeatureId mFidCounter = 0;                    // next feature id if map is used
    QMap<QVariantList, QgsFeatureId> mKeyToFid;      // map key values to feature id
    QMap<QgsFeatureId, QVariantList> mFidToKey;      // map feature back to fea
};


#endif
