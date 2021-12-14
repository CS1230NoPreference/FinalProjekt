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
#include <thread>
#include <unistd.h>
#include "Canvas2D.h"
#include "Settings.h"
#include "RayScene.h"

#include <QCoreApplication>
#include <QPainter>
#include <QtConcurrent/qtconcurrentrun.h>

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

    settings.fractal_depth= settings.shapeParameter1;
    settings.rendernumber= settings.shapeType;
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


    if( settings.renderSphere==settings.rendernumber){
        std::cout<<"renderSphere"<<std::endl;
    }
    if( settings.rendertree==settings.rendernumber){
          std::cout<<"rendertree"<< std::endl;

    }

    if( settings.rendermandelbulb==settings.rendernumber){
           std::cout<<"rendermandelbulb"<< std::endl;

    }

    if( settings.renderepicscene1==settings.rendernumber){
           std::cout<<"renderepicscene1"<< std::endl;
    }

    if( settings.renderepicscene2==settings.rendernumber){
           std::cout<<"renderepicscene2"<< std::endl;
    }

    auto Supersampling = settings.useSuperSampling ? settings.numSuperSamples : 1;

    auto iTime = 4200;
    auto rayOrigin = 1.45f * glm::vec4{ 6.0 * std::sin(iTime * .3), 4.8, 6.0 * std::cos(iTime * .3), 1 };
//    auto rayOrigin = glm::vec4{0, 0, -1, 0};
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

    Ray::RelativeStepSizeForOcclusionEstimation = 0.5;
    Ray::RelativeStepSizeForIntersection = 0.5;

    Lights.resize(2);
    Lights[0].type = LightType::LIGHT_DIRECTIONAL;
    Lights[0].color = glm::vec4{ 1., 1., 1., 1. };
    Lights[0].dir = glm::vec4{ -glm::normalize(glm::vec3{ 1.0, 0.6, 0.5 }), 0 };

    Lights[1].type = LightType::LIGHT_DIRECTIONAL;
    Lights[1].color = glm::vec4{ 0.25, 0.25, 0.25, 1. };
    Lights[1].dir = -look;

//    auto MandelbulbLights = Lights;
//    Lights[1].type = LightType::LIGHT_DIRECTIONAL;
//    Lights[1].color = glm::vec4{ 0.6, 0.6, 0.6, 1. };
//    Lights[1].dir = -look;

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
    auto hash = [](auto x, auto y, auto z) {
        return -1.f + 2.f * glm::fract(glm::sin(x * 311.7f + y * 127.1f + z * 241.3f) * 4378.5453123f);
    };

    ObjectRecords[0].DistanceFunction = [](auto&& p) {
        return static_cast<double>(p.y + 0.2138*std::cos(p.x + p.z)
                                   + 0.3902*std::sin(p.y)
                                   - 0.4520*std::sin(p.x));
//                                   + 0.1242*std::sin(p.x + p.z));
//        return static_cast<double>(p.y) + std::sin(p.x + p.z);
    };
    ObjectRecords[0].Material.cDiffuse = glm::vec4{ 0.85, 0.9, 0.5, 1 };
    ObjectRecords[0].Material.cAmbient = glm::vec4{ 0.1, 0.1, 0.1, 1 };
    ObjectRecords[0].Material.cSpecular = glm::vec4{ 0.25, 0.25, 0.25, 1 };
    ObjectRecords[0].Material.shininess = 32;
    ObjectRecords[0].IlluminationModel = GlobalIlluminationModel;

    ObjectRecords[1].DistanceFunction = CreateSphere(glm::vec4{ -1, 2.25, 0, 1 }, 1.5);
    ObjectRecords[1].Material.cDiffuse = glm::vec4{ 0, 0, 0, 1 };
    ObjectRecords[1].Material.cAmbient = glm::vec4{ 0, 0, 0, 1 };
    ObjectRecords[1].Material.cSpecular = glm::vec4{ 1, 1, 1, 1 };
    ObjectRecords[1].Material.cReflective = glm::vec4{ 1, 1, 1, 1 };
    ObjectRecords[1].Material.cTransparent = glm::vec4{ 1, 1, 1, 1 };
    ObjectRecords[1].Material.IsReflective = true;
    ObjectRecords[1].Material.IsTransparent = true;
    ObjectRecords[1].Material.shininess = 32;
    ObjectRecords[1].Material.ior = 2;
    ObjectRecords[1].IlluminationModel = GlobalIlluminationModel;

    ObjectRecords[2].DistanceFunction = CreateSphere(glm::vec4{ 2, 0.25, 1.5, 1 }, 1.);
    ObjectRecords[2].Material.cDiffuse = glm::vec4{ 1, 0, 0, 1 };
    ObjectRecords[2].Material.cAmbient = glm::vec4{ 0, 0, 0, 1 };
    ObjectRecords[2].Material.cSpecular = glm::vec4{ 1, 1, 1, 1 };
    ObjectRecords[2].Material.cReflective = glm::vec4{ 0.5, 0.5, 0.5, 1 };
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
    ObjectRecords[3].IlluminationModel = GlobalIlluminationModel;


    auto InterruptHandler = [&](auto&& SurfacePosition, auto&& SurfaceNormal, auto&& ObjectRecord) {
        if (auto& [_, ObjectMaterial, __] = ObjectRecord; &ObjectRecord == &ObjectRecords[ObjectRecords.size() - 1])
            ObjectMaterial.cDiffuse = SurfaceNormal;
    };

    // Helper function to create a thread
    auto CreateThread = [=](auto&& SupersampledRender, auto&& SupersampledRayCaster, auto start, auto end, auto height) {
        auto ORCopy = ObjectRecords;
        auto DFCopy = DistanceField::Synthesize(ORCopy);
        auto GIMCopy = Illuminations::ConfigureIlluminationModel(Lights, Ka, Kd, Ks, DFCopy, Hardness);
        for (auto i : Range{ ORCopy.size() })
            ORCopy[i].IlluminationModel = GIMCopy;
//        ORCopy[ORCopy.size() - 1].IlluminationModel = Illuminations::ConfigureIlluminationModel(MandelbulbLights, Ka, Kd, Ks, DFCopy, Hardness);
        // Custom Interrupt handler
        auto InterruptHandler = [&ORCopy](auto&& SurfacePosition, auto&& SurfaceNormal, auto&& ObjectRecord) {
            if (auto& [_, ObjectMaterial, __] = ObjectRecord; &ObjectRecord == &ORCopy[ORCopy.size() - 1])
                ObjectMaterial.cDiffuse = SurfaceNormal;
        };
        // Render each pixel
        for (auto y : Range{ height })
            for (auto x = start; x < end; x++) {
                auto AccumulatedIntensity = Ray::March(rayOrigin, SupersampledRayCaster(y, x), Ks, Kt, DFCopy, InterruptHandler, 1);
                SupersampledRender[0][y][x] = AccumulatedIntensity.x;
                SupersampledRender[1][y][x] = AccumulatedIntensity.y;
                SupersampledRender[2][y][x] = AccumulatedIntensity.z;
            }
    };

    // Maximize thread usage
    QThreadPool::globalInstance()->setMaxThreadCount(1.5 * std::thread::hardware_concurrency());
    //// Performance metrics logging; should disable later
    std::cout << "Number of threads available: " << std::thread::hardware_concurrency() << std::endl;
    int MaxThreads = QThreadPool::globalInstance()->maxThreadCount();
    std::cout << "Number of threads being used: " << (settings.useMultiThreading ? MaxThreads : 1) << std::endl;
    auto start = std::chrono::steady_clock::now();
    std::string ThreadType = settings.useMultiThreading ? "Multithreaded: " : "Singlethreaded: ";


    auto SupersampledRender = Filter::Frame{ height * Supersampling, width * Supersampling, 3 };
    auto RayCaster = ViewPlane::ConfigureRayCaster(look, up, focalLength, height * Supersampling, width * Supersampling);

    if (settings.useMultiThreading) {
        int ThreadWidth = width * Supersampling / MaxThreads;
        for (int i = 0; i < MaxThreads; i++) {
            QtConcurrent::run([=, &SupersampledRender]() {
                auto start = i * ThreadWidth, end = (i + 1) * ThreadWidth;
                if (i == MaxThreads - 1) end += (width * Supersampling) % MaxThreads;
                CreateThread(SupersampledRender, RayCaster, start, end, height * Supersampling);
            });
        }
        // wait for threads to finish
        QThreadPool::globalInstance()->waitForDone();
    } else {
        for (auto y : Range{ height * Supersampling })
            for (auto x : Range{ width * Supersampling }) {
                auto AccumulatedIntensity = Ray::March(rayOrigin, RayCaster(y, x), Ks, Kt, DistanceField, InterruptHandler, 1);
                SupersampledRender[0][y][x] = AccumulatedIntensity.x;
                SupersampledRender[1][y][x] = AccumulatedIntensity.y;
                SupersampledRender[2][y][x] = AccumulatedIntensity.z;
            }
    }

    auto ResampledRender = Filter::Transpose(Filter::HorizontalScale(Filter::Transpose(Filter::HorizontalScale(SupersampledRender.Finalize(), 1. / Supersampling)), 1. / Supersampling));
    Filter::DisplayPort::Transfer(*this, ResampledRender);

    std::cout << "Done rendering." << std::endl;

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << ThreadType << elapsed_seconds.count() << "s" << std::endl;

    this->update();
}

void Canvas2D::cancelRender() {
    // TODO: cancel the raytracer (optional)
}

/* Fractal Tree
    int leaf=0;
    ObjectRecords[2].DistanceFunction = [&](auto&& p) {
            auto ln =[](auto&& p, auto&& a, auto&& b, auto&& R) {
                double r = glm::dot(p-a,b-a)/glm::dot(b-a,b-a);
                r = std::clamp(r,0.,1.);
               // p.x+= 0.2*sqrt(R)*smoothstep(1.,0.,abs(r*2.-1.));
                return glm::length(p-a-(float)r*(b-a))-R*(1.5-0.4*r);
            };
            auto ro =[](auto&& a) {
                float s = sin(a), c = cos(a);
                return glm::mat2(c,-s,s,c);
            };

            double l = glm::length(p);
            double iTime= 113.14;
            auto pos= p;

            pos.y += 0.4;
            pos.z += 0.2;
            pos.x += 0.0;
            float pi =3.14159;
            //pos.xz *= 1.;
            glm::vec2 rl = glm::vec2(0.02,.25+ 0.01*sin(pi*4.*iTime));
            leaf=0;
            for (int i = 1; i <16; i++) {
                l = std::min(l,ln(pos,glm::vec4(0),glm::vec4(0,rl.y,0,0),rl.x));
                pos.y -= rl.y;
                //p.xy *= ro(0.2*sin(3.1*iTime+float(i))+sin(0.222*iTime)*(-0.1*sin(0.4*pi*iTime)+sin(0.543*iTime)/max(float(i),2.)));
                pos.x = abs(pos.x);
                //p.zy *= ro(0.5);
                auto tmp = glm::vec2{pos.x,pos.y} * ro(0.6+0.4*sin(iTime)*sin(0.871*iTime)+0.05*float(i)*sin(2.*iTime));
                pos.x = tmp.x;
                pos.y = tmp.y;
                tmp = glm::vec2{pos.z,pos.x}*ro(0.5*pi+0.2*sin(0.5278*iTime)+0.8*float(i)*(sin(0.1*iTime)*(sin(0.1*pi*iTime)+sin(0.333*iTime)+0.2*sin(1.292*iTime))));
                pos.z =tmp.x;
                pos.x =tmp.y;

                rl *= (.7+0.015*float(i)*(sin(iTime)+0.1*sin(4.*pi*iTime)));

                if( glm::length(pos)-0.15*sqrt(rl.x) <l && i>10){
                    leaf=1;
                }

                l=std::min(l,glm::length(pos)-0.15*sqrt(rl.x));
            }
            return l;
        };
    ObjectRecords[0].Material.cDiffuse = glm::vec4{ 1.0, 0.2, 0.6, 1 };
    ObjectRecords[0].Material.cAmbient = glm::vec4{ 0.1, 0.1, 0.1, 1 };
    ObjectRecords[0].Material.cSpecular = glm::vec4{ 0., 0., 0., 1 };
    ObjectRecords[0].Material.shininess = 32;
    ObjectRecords[0].IlluminationModel = GlobalIlluminationModel;

    auto InterruptHandler = [&](auto&& SurfacePosition, auto&& SurfaceNormal, auto&& ObjectRecord) {
        if (leaf == 1)
            ORCopy[2].Material.cDiffuse = glm::vec4{ 0.0, 1.0, 0.0, 1 };
        else
            ORCopy[2].Material.cDiffuse = glm::vec4{ 165.0/255.0, 42.0/255.0, 42.0/255.0, 1 };

        if (auto& [_, ObjectMaterial, __] = ObjectRecord; &ObjectRecord == &ORCopy[ORCopy.size() - 1])
            ObjectMaterial.cDiffuse = SurfaceNormal;
    };


 */
