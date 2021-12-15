/*!

 Settings.h
 CS123 Support Code

 @author  Evan Wallace (edwallac)
 @date    9/1/2010

 This file contains various settings and enumerations that you will need to
 use in the various assignments. The settings are bound to the GUI via static
 data bindings.

**/

#include "Settings.h"
#include <QFile>
#include <QSettings>

Settings settings;


/**
 * Loads the application settings, or, if no saved settings are available, loads default values for
 * the settings. You can change the defaults here.
 */
void Settings::loadSettingsOrDefaults() {
    // Set the default values below
    QSettings s("CS123", "CS123");

    // Shapes
    shapeType = s.value("shapeType", MB1_SCENE).toInt();
    fractalDepth = s.value("fractalDepth", 15).toInt();
    fractalWidth = s.value("fractalWith", 5).toInt();
    fractalHeight = s.value("fractalHeight", 5).toInt();
    mbDepth = s.value("mbDepth", 8).toInt();
//    useLighting = s.value("useLighting", true).toBool();
//    drawWireframe = s.value("drawWireframe", true).toBool();
//    drawNormals = s.value("drawNormals", false).toBool();

    // Camtrans
    useOrbitCamera = s.value("useOrbitCamera", true).toBool();
    cameraFov = s.value("cameraFov", 55).toDouble();
    cameraNear = s.value("cameraNear", 0.1).toDouble();
    cameraFar = s.value("cameraFar", 50).toDouble();

    // Ray
    useSuperSampling = s.value("useSuperSampling", false).toBool();
    numSuperSamples = s.value("numSuperSamples", 2).toInt();
    useAntiAliasing = s.value("useAntiAliasing", true).toBool();
    useShadows = s.value("useShadows", false).toBool();
    shadowHardness = s.value("shadowHardness", 2).toInt();
    useReflection = s.value("useReflection", false).toBool();
    useRefraction = s.value("useRefraction", false).toBool();
    rayRecursionDepth = s.value("rayRecursionDepth", 1).toInt();
    useMultiThreading = s.value("useMultiThreading", true).toBool();
    usePointLights = s.value("usePointLights", true).toBool();
    useDirectionalLights = s.value("useDirectionalLights", true).toBool();
    useSpotLights = s.value("useSpotLights", true).toBool();

    currentTab = s.value("currentTab", TAB_2D).toBool();

    // These are for computing deltas and the values don't matter, so start all dials in the up
    // position
    cameraPosX = 0;
    cameraPosY = 0;
    cameraPosZ = 0;
    cameraRotU = 0;
    cameraRotV = 0;
    cameraRotN = 0;
}

void Settings::saveSettings() {
    QSettings s("CS123", "CS123");

    // Brush
    s.setValue("brushType", brushType);
    s.setValue("brushRadius", brushRadius);
    s.setValue("brushRed", brushColor.r);
    s.setValue("brushGreen", brushColor.g);
    s.setValue("brushBlue", brushColor.b);
    s.setValue("brushAlpha", brushColor.a);

    // Filter
    s.setValue("filterType", filterType);
    s.setValue("edgeDetectSensitivity", edgeDetectSensitivity);
    s.setValue("blurRadius", blurRadius);
    s.setValue("scaleX", scaleX);
    s.setValue("scaleY", scaleY);
    s.setValue("rotateAngle", rotateAngle);

    // Shapes
    s.setValue("shapeType", shapeType);
    s.setValue("fractalDepth", fractalDepth);
    s.setValue("fractalWidth", fractalWidth);
    s.setValue("fractalHeight", fractalHeight);
    s.setValue("mbDepth", mbDepth);
//    s.setValue("useLighting", useLighting);
//    s.setValue("drawWireframe", drawWireframe);
//    s.setValue("drawNormals", drawNormals);

    // Camtrans
    s.setValue("useOrbitCamera", useOrbitCamera);
    s.setValue("cameraFov", cameraFov);
    s.setValue("cameraNear", cameraNear);
    s.setValue("cameraFar", cameraFar);

    // Ray
    s.setValue("useSuperSampling", useSuperSampling);
    s.setValue("numSuperSamples", numSuperSamples);
    s.setValue("useAntiAliasing", useAntiAliasing);
    s.setValue("useShadows", useShadows);
    s.setValue("shadowHardness", shadowHardness);
    s.setValue("useReflection", useReflection);
    s.setValue("useRefraction", useRefraction);
    s.setValue("rayRecursionDepth", rayRecursionDepth);
    s.setValue("useMultiThreading", useMultiThreading);
    s.setValue("usePointLights", usePointLights);
    s.setValue("useDirectionalLights", useDirectionalLights);
    s.setValue("useSpotLights", useSpotLights);

    s.setValue("currentTab", currentTab);
}

int Settings::getSceneMode() {
    if (this->useSceneviewScene)
        return SCENEMODE_SCENEVIEW;
    else
        return SCENEMODE_SHAPES;
}

int Settings::getCameraMode() {
    if (this->useOrbitCamera)
        return CAMERAMODE_ORBIT;
    else
        return CAMERAMODE_CAMTRANS;
}
