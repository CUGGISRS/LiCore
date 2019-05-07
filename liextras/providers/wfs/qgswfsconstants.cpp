/***************************************************************************
    qgswfsconstants.cpp
    ---------------------
    begin                : February 2016
    copyright            : (C) 2016 by Even Rouault
    email                : even.rouault at spatialys.com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgswfsconstants.h"

const QString QgsWFSConstants::GML_NAMESPACE( QStringLiteral( "http://www.opengis.net/gml" ) );
const QString QgsWFSConstants::OGC_NAMESPACE( QStringLiteral( "http://www.opengis.net/ogc" ) );
const QString QgsWFSConstants::OWS_NAMESPACE( QStringLiteral( "http://www.opengis.net/ows" ) );
const QString QgsWFSConstants::WFS_NAMESPACE( QStringLiteral( "http://www.opengis.net/wfs" ) );
const QString QgsWFSConstants::XMLSCHEMA_NAMESPACE( QStringLiteral( "http://www.w3.org/2001/XMLSchema" ) );

const QString QgsWFSConstants::URI_PARAM_URL( QStringLiteral( "url" ) );
const QString QgsWFSConstants::URI_PARAM_USERNAME( QStringLiteral( "username" ) );
const QString QgsWFSConstants::URI_PARAM_USER( QStringLiteral( "user" ) );
const QString QgsWFSConstants::URI_PARAM_PASSWORD( QStringLiteral( "password" ) );
const QString QgsWFSConstants::URI_PARAM_AUTHCFG( QStringLiteral( "authcfg" ) );
const QString QgsWFSConstants::URI_PARAM_VERSION( QStringLiteral( "version" ) );
const QString QgsWFSConstants::URI_PARAM_TYPENAME( QStringLiteral( "typename" ) );
const QString QgsWFSConstants::URI_PARAM_SRSNAME( QStringLiteral( "srsname" ) );
const QString QgsWFSConstants::URI_PARAM_BBOX( QStringLiteral( "bbox" ) );
const QString QgsWFSConstants::URI_PARAM_FILTER( QStringLiteral( "filter" ) );
const QString QgsWFSConstants::URI_PARAM_OUTPUTFORMAT( QStringLiteral( "outputformat" ) );
const QString QgsWFSConstants::URI_PARAM_RESTRICT_TO_REQUEST_BBOX( QStringLiteral( "restrictToRequestBBOX" ) );
const QString QgsWFSConstants::URI_PARAM_MAXNUMFEATURES( QStringLiteral( "maxNumFeatures" ) );
const QString QgsWFSConstants::URI_PARAM_IGNOREAXISORIENTATION( QStringLiteral( "IgnoreAxisOrientation" ) );
const QString QgsWFSConstants::URI_PARAM_INVERTAXISORIENTATION( QStringLiteral( "InvertAxisOrientation" ) );
const QString QgsWFSConstants::URI_PARAM_VALIDATESQLFUNCTIONS( QStringLiteral( "validateSQLFunctions" ) );
const QString QgsWFSConstants::URI_PARAM_HIDEDOWNLOADPROGRESSDIALOG( QStringLiteral( "hideDownloadProgressDialog" ) );

const QString QgsWFSConstants::VERSION_AUTO( QStringLiteral( "auto" ) );

const QString QgsWFSConstants::CONNECTIONS_WFS( QStringLiteral( "qgis/connections-wfs/" ) );
const QString QgsWFSConstants::SETTINGS_VERSION( QStringLiteral( "version" ) );
const QString QgsWFSConstants::SETTINGS_MAXNUMFEATURES( QStringLiteral( "maxnumfeatures" ) );

const QString QgsWFSConstants::FIELD_GEN_COUNTER( QStringLiteral( "__qgis_gen_counter" ) );
const QString QgsWFSConstants::FIELD_GMLID( QStringLiteral( "__qgis_gmlid" ) );
const QString QgsWFSConstants::FIELD_HEXWKB_GEOM( QStringLiteral( "__qgis_hexwkb_geom" ) );
const QString QgsWFSConstants::FIELD_MD5( QStringLiteral( "__qgis_md5" ) );
