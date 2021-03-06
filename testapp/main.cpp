﻿#include <QApplication>
#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QtQml>
#include <QMainWindow>

#include <liviewer.h>
#include <liwidget.h>
#include <liengine.h>
#include <liscene.h>
#include <globe.h>
#include <terrainprovider.h>
#include <terrainsurfacelayer.h>
#include <imagerylayer.h>
#include <asyncfuture.h>
#include <litimesystem.h>

#include <lientity.h>
#include <licomponent.h>
#include <libehavior.h>
#include <litransform.h>
#include <ligeometryrenderer.h>
#include <licamera.h>
#include <lilight.h>
#include <lisun.h>

#include <libuffer.h>
#include <ligeometry.h>
#include <ligeometryattribute.h>
#include <liboxgeometry.h>

#include <liparameter.h>
#include <limaterial.h>
#include <lishader.h>
#include <lishaderprogram.h>
#include <lirenderstate.h>

#include <litexture.h>
#include <litextureimage.h>
#include <lirendertarget.h>
#include <lirendertargetoutput.h>
#include <lipostprocess.h>
#include <lifullscreenquad.h>
#include <liraycasthit.h>
#include <liframeaction.h>
#include <liambientocclusion.h>
#include <litonemapping.h>
#include <litextureprojection.h>
#include <liviewshed.h>
#include <licomputerenderer.h>

#include <lisceneiofactory.h>
#include <lisceneloader.h>
#include <li3dtileset.h>
#include <liforest.h>
#include <lifog.h>

#include <liinputsystem.h>
#include <liscriptengine.h>
#include <liglobeterrainprovider.h>

// LiExtras
#include <qgsfeature.h>
#include <gisimageryprovider.h>
#include <wmsimageryprovider.h>
#include <transformhelper.h>

#include "sample.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qRegisterMetaType<QgsFeature>("QgsFeature");
    QgsApplication::init();
    QgsApplication::initQgis();

    const char *uri = "LiQuick.Core";
    qmlRegisterUncreatableType<LiNode>(uri, 1, 0, "Node", QLatin1String("Node is a base class"));
    qmlRegisterUncreatableType<LiComponent>(uri, 1, 0, "Component3D", QLatin1String("Component3D is a base class"));
    qmlRegisterUncreatableType<LiAmbientOcclusion>(uri, 1, 0, "AmbientOcclusion", QLatin1String("AmbientOcclusion is a base class"));
    qmlRegisterUncreatableType<LiToneMapping>(uri, 1, 0, "HDRToneMap", QLatin1String("HDRToneMap is a base class"));
    qmlRegisterType<LiEntity>(uri, 1, 0, "Entity");
    qmlRegisterType<LiBehavior>(uri, 1, 0, "Behavior3D");
    qmlRegisterType<LiFrameAction>(uri, 1, 0, "FrameAction");
    qmlRegisterType<LiTransform>(uri, 1, 0, "Transform3D");
    qmlRegisterType<LiCamera>(uri, 1, 0, "Camera");
    qmlRegisterType<LiLight>(uri, 1, 0, "Light");
    qmlRegisterType<LiBuffer>(uri, 1, 0, "Buffer");
    qmlRegisterType<LiGeometry>(uri, 1, 0, "Geometry");
    qmlRegisterType<LiBoxGeometry>(uri, 1, 0, "BoxGeometry");
    qmlRegisterType<LiGeometryAttribute>(uri, 1, 0, "GeometryAttribute");
    qmlRegisterType<LiGeometryRenderer>(uri, 1, 0, "GeometryRenderer");
    qmlRegisterType<LiParameter>(uri, 1, 0, "Parameter");
    qmlRegisterType<LiMaterial>(uri, 1, 0, "Material");
    qmlRegisterType<LiShader>(uri, 1, 0, "Shader");
    qmlRegisterType<LiShaderProgram>(uri, 1, 0, "ShaderProgram");
    qmlRegisterType<LiRenderState>(uri, 1, 0, "RenderState");
    qmlRegisterType<LiTexture>(uri, 1, 0, "Texture");
    qmlRegisterType<LiTexture1D>(uri, 1, 0, "Texture1D");
    qmlRegisterType<LiTexture2D>(uri, 1, 0, "Texture2D");
    qmlRegisterType<LiTextureCubeMap>(uri, 1, 0, "TextureCubeMap");
    qmlRegisterType<LiTextureImage>(uri, 1, 0, "TextureImage");
    qmlRegisterType<LiRenderTarget>(uri, 1, 0, "RenderTarget");
    qmlRegisterType<LiRenderTargetOutput>(uri, 1, 0, "RenderTargetOutput");
    qmlRegisterType<LiPostProcess>(uri, 1, 0, "PostProcess");
    qmlRegisterType<LiFullscreenQuad>(uri, 1, 0, "FullscreenQuad");
    qmlRegisterType<LiRaycastHit>(uri, 1, 0, "RaycastHit");
    qmlRegisterType<LiTextureProjection>(uri, 1, 0, "TextureProjection");
    qmlRegisterType<LiViewshed>(uri, 1, 0, "Viewshed");
    qmlRegisterType<LiComputeRenderer>(uri, 1, 0, "ComputeRenderer");

    LiViewer viewer;
    LiScene *scene = viewer.scene();

    LiWidget *widget = viewer.widget();
    QObject::connect(widget, &LiWidget::dropEvent, widget, [scene](QDropEvent *event) {
        qDebug() << event->mimeData()->text();
        Ray ray = scene->mainCamera()->screenPointToRay(event->pos());
        LiRaycastHit hit;
        if (scene->raycast(ray, &hit)) {
            loadModel(scene, event->mimeData()->text(), Cartographic::fromCartesian(hit.point()));
        }
    });

//    QObject::connect(viewer.engine()->inputSystem(), &LiInputSystem::leftButtonDown, []{
//        QPoint p = GlobalViewer()->engine()->inputSystem()->mousePosition();
//        Ray ray = GlobalViewer()->scene()->mainCamera()->screenPointToRay(p);

//        LiRaycastHit hit;
//        if (GlobalViewer()->scene()->raycast(ray, &hit)) {
//            qDebug() << "hit:" << hit.entity();
//        }
//    });

//    loadDOM(viewer.scene());
//    loadTrees(viewer.scene());
//    load3DTileset(viewer.scene());
//    loadCarAnimation(viewer.scene());
//    loadQuadtreeTileset();
//    raycast(viewer.scene());
//    pickFeature(viewer.scene());
//    createTextureProjection();
//    createViewshed(viewer.scene());
//    createPlane(viewer.scene());
//    createFire(viewer.scene());
//    createHeatmap(viewer.scene());
//    createGisImageryProvider(viewer.scene());
//    createLights(viewer.scene());
//    createClipPlane(viewer.scene());
//    createClipVolume(viewer.scene());
//    createWaterMaterial(viewer.scene());
    controlTime();
//    loadWMS();
//    loadPMTS();
//    flattenTerrain();

//    auto *terrainProvider = qobject_cast<LiGlobeTerrainProvider*>(viewer.scene()->globe()->terrainProvider());
//    if (terrainProvider)
//    {
//        auto promise = terrainProvider->readyPromise();
//        observe(promise).subscribe([]{
//            qDebug() << "terrain loaded.";
//        }, [] {
//            qDebug() << "failed to load terrain.";
//        });
//    }


//    viewer.scene()->fog()->setEnabled(false);

    LiInputSystem *input = viewer.engine()->inputSystem();
    QObject::connect(input, &LiInputSystem::keyDown, [&](int key) {
//        if (key == Qt::Key_F7) {
//            loadDOM(viewer.scene());
//        }
//        if (key == Qt::Key_F3) {
//            bool b = viewer.scene()->dayNightLighting();
//            viewer.scene()->setDayNightLighting(!b);
//        }
//        if (key == Qt::Key_F4) {
//            bool b = viewer.scene()->sun()->castShadow();
//            viewer.scene()->sun()->setCastShadow(!b);
//        }
        if (key == Qt::Key_F) {
            viewer.scene()->mainCamera()->flyTo(Cartesian3::fromDegrees(114.054494, 22.540745, 1000));
        }
//        if (key == Qt::Key_G) {
//            bool show = viewer.scene()->globe()->show();
//            viewer.scene()->globe()->setShow(!show);
//        }
    });
//    viewer.scene()->mainCamera()->flyTo(Cartesian3::fromDegrees(114.054494, 22.540745, 1000));

    viewer.show();

    return a.exec();
}
