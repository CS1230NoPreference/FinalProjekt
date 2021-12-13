/**
 * @file Canvas2D.cpp
 *
 * CS123 2-dimensional canvas. Contains support code necessary for Brush, Filter, Intersect, and
 * Ray.
 *
 * YOU WILL NEED TO FILL THIS IN!
 *
 */

// For your convenience, a few headers are included for you.
#include <assert.h>
#include <math.h>
#include <iostream>
#include <memory>
#include <unistd.h>
#include "Canvas2D.h"
#include "Settings.h"
#include "RayScene.h"

#include <QCoreApplication>
#include <QPainter>
#include <QtConcurrent/qtconcurrentrun.h>
#include <ctime>
#include <chrono>
#include <thread>

#include "../UniversalContext.hxx"
#include "../RayMarching.hxx"
#include "../Filter.hxx"

Canvas2D::Canvas2D() {
    this->Width = this->m_image->width();
    this->Height = this->m_image->height();
}

Canvas2D::~Canvas2D()
{
}

// This is called when the canvas size is changed. You can change the canvas size by calling
// resize(...). You probably won't need to fill this in, but you can if you want to.
void Canvas2D::notifySizeChanged(int w, int h) {
    this->Width = this->m_image->width();
    this->Height = this->m_image->height();
}

void Canvas2D::paintEvent(QPaintEvent *e) {
    // You probably won't need to fill this in, but you can if you want to override any painting
    // events for the 2D canvas. For now, we simply call the superclass.
    SupportCanvas2D::paintEvent(e);

}

void Canvas2D::settingsChanged() {
    // TODO: Process changes to the application settings.
    std::cout << "Canvas2d::settingsChanged() called. Settings have changed" << std::endl;
}

// ********************************************************************************************
// ** BRUSH
// ********************************************************************************************


void Canvas2D::mouseDown(int x, int y) {
    UniversalContext::BrushMask.Radius = settings.brushRadius;
    UniversalContext::CachedImagePatch.Radius = settings.brushRadius;

    if (settings.brushType == BrushType::BRUSH_CONSTANT)
        UniversalContext::BrushMask.Update(Brush::IntensityFunctions::Constant);
    else if (settings.brushType == BrushType::BRUSH_LINEAR)
        UniversalContext::BrushMask.Update(Brush::IntensityFunctions::Linear);
    else
        UniversalContext::BrushMask.Update(Brush::IntensityFunctions::Quadratic);

    if (settings.brushType == BrushType::BRUSH_SMUDGE)
        UniversalContext::CachedImagePatch.Update(*this, y, x);

    //    bool fixAlphaBlending = settings.fixAlphaBlending; // for extra/half credit
}

void Canvas2D::mouseDragged(int x, int y) {
    if (settings.brushType == BrushType::BRUSH_SMUDGE)
        Brush::Overlay(*this, UniversalContext::BrushMask, UniversalContext::CachedImagePatch, y, x);
    else
        Brush::Overlay(*this, UniversalContext::BrushMask, settings.brushColor, y, x);
}

void Canvas2D::mouseUp(int x, int y) {
    // TODO: [BRUSH] Mouse interaction for Brush.
    std::cout << "Canvas2d::mouseUp() called" << std::endl;
}



// ********************************************************************************************
// ** FILTER
// ********************************************************************************************

void Canvas2D::filterImage() {
    if (auto InputFrame = Filter::DisplayPort::ConstructFrameFrom(*this); settings.filterType == FilterType::FILTER_EDGE_DETECT)
        Filter::DisplayPort::Transfer(*this, Filter::Sobel(InputFrame, settings.edgeDetectSensitivity));
    else if (settings.filterType == FilterType::FILTER_BLUR)
        Filter::DisplayPort::Transfer(*this, Filter::GaussKernel(settings.blurRadius) * InputFrame);
    else if (settings.filterType == FilterType::FILTER_SCALE) {
        this->resize(Width * settings.scaleX + 0.5, Height * settings.scaleY + 0.5);
        Filter::DisplayPort::Transfer(*this, Filter::Transpose(Filter::HorizontalScale(Filter::Transpose(Filter::HorizontalScale(InputFrame, settings.scaleX)), settings.scaleY)));
    }
    else if (settings.filterType == FilterType::FILTER_SPECIAL_1)
        Filter::DisplayPort::Transfer(*this, Filter::MedianKernel * InputFrame);

    // Leave this code here! This code ensures that the Canvas2D will be completely wiped before
    // drawing the new image.
    repaint(rect());
    QCoreApplication::processEvents();
}

// ********************************************************************************************
// ** RAY
// ********************************************************************************************

void Canvas2D::setScene(RayScene *scene) {
    m_rayScene.reset(scene);
}

/* Ray tracing rendering down below
void Canvas2D::renderImage(CS123SceneCameraData* camera, int width, int height) {
    if (m_rayScene) {
        this->resize(width, height);
        camera->focalLength = 0.1;
        camera->aspectRatio = static_cast<double>(width) / height;

        using ImplicitFunctionType = std::function<auto(const glm::vec4&, const glm::vec4&)->std::tuple<double, double, double, glm::vec3>>;
        using IlluminationModelType = std::function<auto(const glm::vec4&, const glm::vec4&, const glm::vec4&, double, double, const CS123SceneMaterial&)->glm::vec4>;
        using ObjectRecordType = std::tuple<ImplicitFunctionType, glm::mat4x4, CS123SceneMaterial, IlluminationModelType>;

        auto ProjectIntoWorldSpace = ViewPlane::ConfigureProjectorFromScreenSpaceIntoWorldSpace(*camera, height, width);
        auto ObjectRecords = std::vector<ObjectRecordType>{};
        auto FloatingPointToUInt8 = [](auto x) { return std::clamp(static_cast<int>(255 * x), 0, 255); };

        if (settings.useReflection == false)
            Ray::RecursiveTracingDepth = 1;
        else
            Ray::RecursiveTracingDepth = Ray::DefaultRecursiveTracingDepth;

        for (auto&& [Primitive, ObjectTransformation] : m_rayScene->Objects) {
            auto ImplicitFunction = [&] {
                if (Primitive.type == PrimitiveType::PRIMITIVE_CUBE)
                    return ImplicitFunctionType{ ImplicitFunctions::Cube };
                else if (Primitive.type == PrimitiveType::PRIMITIVE_SPHERE)
                    return ImplicitFunctionType{ ImplicitFunctions::Sphere };
                else if (Primitive.type == PrimitiveType::PRIMITIVE_CYLINDER)
                    return ImplicitFunctionType{ ImplicitFunctions::Cylinder };
                else if (Primitive.type == PrimitiveType::PRIMITIVE_CONE)
                    return ImplicitFunctionType{ ImplicitFunctions::Cone };
                else
                    throw std::runtime_error{ "Unrecognized primitive type detected!" };
            }();
            ObjectRecords.push_back({ ImplicitFunction , ObjectTransformation, Primitive.material, Illuminations::ConfigureIlluminationModel(m_rayScene->Lights, m_rayScene->GlobalData.ka, m_rayScene->GlobalData.kd, m_rayScene->GlobalData.ks, Texture::Image{ Primitive.material.textureMap.filename.data() }, ObjectRecords, settings.usePointLights, settings.useDirectionalLights, settings.useShadows, settings.useTextureMapping) });
        }

        for (auto& Self = *this; auto y : Range{ height })
            for (auto x : Range{ width }) {
                auto AccumulatedIntensity = Ray::Trace(camera->pos, glm::normalize(ProjectIntoWorldSpace(y, x) - camera->pos), m_rayScene->GlobalData.ks, ObjectRecords, 1);
                Self[y][x] = RGBA{ FloatingPointToUInt8(Self[y][x].r / 255. + AccumulatedIntensity.x), FloatingPointToUInt8(Self[y][x].g / 255. + AccumulatedIntensity.y), FloatingPointToUInt8(Self[y][x].b / 255. + AccumulatedIntensity.z), FloatingPointToUInt8(Self[y][x].a / 255. + AccumulatedIntensity.w) };
            }

        this->update();
    }
}
*/

void Canvas2D::renderImage(CS123SceneCameraData*, int width, int height) {
    this->resize(width, height);

    auto iTime = 4200;
    auto rayOrigin = glm::vec4{ 6.0 * std::sin(iTime * .3), 4.8, 6.0 * std::cos(iTime * .3), 1 };
    auto focalLength = 2.;

    auto target = glm::vec4{ 0, 0, 0, 1 };
    auto look = glm::normalize(rayOrigin - target);
    auto up = glm::vec4{ 0, 1, 0, 0 };

    auto Ka = 1.f;
    auto Kd = 1.f;
    auto Ks = 1.f;
    auto Kt = 1.f;
    auto Hardness = 2.;
    auto Lights = std::vector<CS123SceneLightData>{};

    Lights.resize(1);
    Lights[0].type = LightType::LIGHT_DIRECTIONAL;
    Lights[0].color = glm::vec4{ 1., 1., 1., 1. };
    Lights[0].dir = glm::vec4{ -glm::normalize(glm::vec3{ 1.0, 0.6, 0.5 }), 0 };

    auto MandelbulbLights = Lights;
    MandelbulbLights.resize(2);
    MandelbulbLights[1].type = LightType::LIGHT_DIRECTIONAL;
    MandelbulbLights[1].color = glm::vec4{ 0.25, 0.25, 0.25, 1. };
    MandelbulbLights[1].dir = -look;

    using DistanceFunctionType = std::function<auto(const glm::vec4&)->double>;
    using IlluminationModelType = std::function<auto(const glm::vec4&, const glm::vec4&, const glm::vec4&, const CS123SceneMaterial&)->glm::vec4>;
    using ObjectRecordType = struct { DistanceFunctionType DistanceFunction; CS123SceneMaterial Material; IlluminationModelType IlluminationModel; };

    auto ObjectRecords = std::vector<ObjectRecordType>{};
    auto DistanceField = DistanceField::Synthesize(ObjectRecords);
    auto GlobalIlluminationModel = Illuminations::ConfigureIlluminationModel(Lights, Ka, Kd, Ks, DistanceField, Hardness);

    ObjectRecords.resize(4);

    auto CreateSphere = [](auto&& Center, auto Radius) {
        return [=, Center = Forward(Center)](auto&& Position) { return glm::length(Position - Center) - Radius; };
    };

    ObjectRecords[0].DistanceFunction = [](auto&& p) { return static_cast<double>(p.y); };
    ObjectRecords[0].Material.cDiffuse = glm::vec4{ 0.2, 0.2, 0.6, 1 };
    ObjectRecords[0].Material.cAmbient = glm::vec4{ 0.1, 0.1, 0.1, 1 };
    ObjectRecords[0].Material.cSpecular = glm::vec4{ 0.25, 0.25, 0.25, 1 };
    ObjectRecords[0].Material.shininess = 32;
    ObjectRecords[0].IlluminationModel = GlobalIlluminationModel;

    ObjectRecords[1].DistanceFunction = CreateSphere(glm::vec4{ -1, 2.25, 0, 1 }, 1.5);
    ObjectRecords[1].Material.cDiffuse = glm::vec4{ 0, 0, 0, 1 };
    ObjectRecords[1].Material.cAmbient = glm::vec4{ 0, 0, 0, 1 };
    ObjectRecords[1].Material.cSpecular = glm::vec4{ 1, 1, 1, 1 };
    ObjectRecords[1].Material.cReflective = glm::vec4{ 0.25, 0.25, 0.25, 1 };
    ObjectRecords[1].Material.cTransparent = glm::vec4{ 1, 1, 1, 1 };
    ObjectRecords[1].Material.IsReflective = true;
    ObjectRecords[1].Material.IsTransparent = true;
    ObjectRecords[1].Material.shininess = 32;
    ObjectRecords[1].Material.ior = 3;
    ObjectRecords[1].IlluminationModel = GlobalIlluminationModel;

    ObjectRecords[2].DistanceFunction = CreateSphere(glm::vec4{ 2, 0.25, 1.5, 1 }, 1.);
    ObjectRecords[2].Material.cDiffuse = glm::vec4{ 1, 0, 0, 1 };
    ObjectRecords[2].Material.cAmbient = glm::vec4{ 0, 0, 0, 1 };
    ObjectRecords[2].Material.cSpecular = glm::vec4{ 1, 1, 1, 1 };
    ObjectRecords[2].Material.cReflective = glm::vec4{ 0.25, 0.25, 0.25, 1 };
    ObjectRecords[2].Material.IsReflective = true;
    ObjectRecords[2].Material.shininess = 8;
    ObjectRecords[2].IlluminationModel = GlobalIlluminationModel;

    ObjectRecords[3].DistanceFunction = [](auto&& p) {
        auto sdMandelbulb = [](auto&& pos) {
            auto z = glm::vec3{ pos };
            auto dr = 1.0;
            auto r = 0.0;
            for (auto _ : Range{ 5 }) {
                r = glm::length(z);
                if (r > 4.) break;
                // convert to polar coordinates
                auto theta = std::acos(z.z / r);
                auto phi = std::atan2(z.y, z.x);
                dr = std::pow(r, 8. - 1) * 8. * dr + 1;

                // scale and rotate the point
                auto zr = static_cast<float>(std::pow(r, 8.));
                theta = theta * 8.;
                phi = phi * 8.;

                // convert back to cartesian coordinates
                z = zr * glm::vec3{
                    std::sin(theta) * std::cos(phi),
                    std::sin(phi) * std::sin(theta),
                    std::cos(theta) };
                z += glm::vec3{ pos };
            }
            return 0.5 * std::log(r) * r / dr;
        };
        return sdMandelbulb(p - glm::vec4{ -1,2,-3,0 });
    };
    ObjectRecords[3].Material.cDiffuse = glm::vec4{ 1, 1, 1, 1 };
    ObjectRecords[3].Material.cAmbient = glm::vec4{ 0, 0, 0, 1 };
    ObjectRecords[3].Material.cSpecular = glm::vec4{ 0, 0, 0, 1 };
    ObjectRecords[3].Material.shininess = 1;
    ObjectRecords[3].IlluminationModel = Illuminations::ConfigureIlluminationModel(MandelbulbLights, Ka, Kd, Ks, DistanceField, Hardness);

    auto RayCaster = ViewPlane::ConfigureRayCaster(look, up, focalLength, height, width);
    auto FloatingPointToUInt8 = [](auto x) { return std::clamp(static_cast<int>(255 * x), 0, 255); };

    std::cout << "Number of threads available: " << std::thread::hardware_concurrency() << std::endl;
    std::cout << "Number of threads being used: " << QThreadPool::globalInstance()->maxThreadCount() << std::endl;
    auto start = std::chrono::steady_clock::now();
    std::string ThreadType;
    QFuture<void> wait;
    for (auto& Self = *this; auto y : Range{ height }) {
      for (auto x : Range{ width }) {
        if (settings.useMultiThreading) {
            ThreadType = "Multithreaded: ";
            QtConcurrent::run([=, &Self]() {
                auto ORCopy = ObjectRecords;
                auto DistanceFieldCopy = DistanceField::Synthesize(ORCopy);
                auto GIMCopy = Illuminations::ConfigureIlluminationModel(Lights, Ka, Kd, Ks, DistanceFieldCopy, Hardness);
                for (auto i : Range{ ORCopy.size() })
                ORCopy[i].IlluminationModel = GIMCopy;
                ORCopy[ORCopy.size() - 1].IlluminationModel = Illuminations::ConfigureIlluminationModel(MandelbulbLights, Ka, Kd, Ks, DistanceFieldCopy, Hardness);
                auto Interrupt = [&ORCopy](auto&& SurfacePosition, auto&& SurfaceNormal, auto&& ObjectRecord) {
                if (auto& [_, ObjectMaterial, __] = ObjectRecord; &ObjectRecord == &ORCopy[ORCopy.size() - 1])
                    ObjectMaterial.cDiffuse = SurfaceNormal;
                };
                auto AccumulatedIntensity = Ray::March(rayOrigin, RayCaster(y, x), Ks, Kt, DistanceFieldCopy, Interrupt, 1);
                Self[y][x] = RGBA{ FloatingPointToUInt8(Self[y][x].r / 255. + AccumulatedIntensity.x), FloatingPointToUInt8(Self[y][x].g / 255. + AccumulatedIntensity.y), FloatingPointToUInt8(Self[y][x].b / 255. + AccumulatedIntensity.z), FloatingPointToUInt8(Self[y][x].a / 255. + AccumulatedIntensity.w) };
            });
        } else {
                ThreadType = "Singlethreaded: ";
                auto Interrupt = [&](auto&& SurfacePosition, auto&& SurfaceNormal, auto&& ObjectRecord) {
                if (auto& [_, ObjectMaterial, __] = ObjectRecord; &ObjectRecord == &ObjectRecords[3])
                    ObjectMaterial.cDiffuse = SurfaceNormal;
                };
                auto AccumulatedIntensity = Ray::March(rayOrigin, RayCaster(y, x), Ks, Kt, DistanceField, Interrupt, 1);
                Self[y][x] = RGBA{ FloatingPointToUInt8(Self[y][x].r / 255. + AccumulatedIntensity.x), FloatingPointToUInt8(Self[y][x].g / 255. + AccumulatedIntensity.y), FloatingPointToUInt8(Self[y][x].b / 255. + AccumulatedIntensity.z), FloatingPointToUInt8(Self[y][x].a / 255. + AccumulatedIntensity.w) };
        }
    }
    std::cout << "Active threads: " << QThreadPool::globalInstance()->activeThreadCount() << std::endl;
  }

//    if (settings.useMultiThreading)
//            wait.waitForFinished();

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << ThreadType << elapsed_seconds.count() << "s" << std::endl;


    this->update();
}

void Canvas2D::cancelRender() {
    // TODO: cancel the raytracer (optional)
}
