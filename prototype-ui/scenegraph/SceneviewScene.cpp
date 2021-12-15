#include "SceneviewScene.h"
#include "GL/glew.h"
#include <QGLWidget>
#include "Camera.h"
#include <iostream>
#include "Settings.h"
#include "SupportCanvas3D.h"
#include "ResourceLoader.h"
#include "gl/shaders/CS123Shader.h"


using namespace CS123::GL;


SceneviewScene::SceneviewScene()
{
    // TODO: [SCENEVIEW] Set up anything you need for your Sceneview scene here...
    loadPhongShader();
    loadWireframeShader();
    loadNormalsShader();
    loadNormalsArrowShader();
}

SceneviewScene::~SceneviewScene()
{
}

void SceneviewScene::loadPhongShader() {
    std::string vertexSource = ResourceLoader::loadResourceFileToString(":/shaders/default.vert");
    std::string fragmentSource = ResourceLoader::loadResourceFileToString(":/shaders/default.frag");
    m_phongShader = std::make_unique<CS123Shader>(vertexSource, fragmentSource);
}

void SceneviewScene::loadWireframeShader() {
    std::string vertexSource = ResourceLoader::loadResourceFileToString(":/shaders/wireframe.vert");
    std::string fragmentSource = ResourceLoader::loadResourceFileToString(":/shaders/wireframe.frag");
    m_wireframeShader = std::make_unique<Shader>(vertexSource, fragmentSource);
}

void SceneviewScene::loadNormalsShader() {
    std::string vertexSource = ResourceLoader::loadResourceFileToString(":/shaders/normals.vert");
    std::string geometrySource = ResourceLoader::loadResourceFileToString(":/shaders/normals.gsh");
    std::string fragmentSource = ResourceLoader::loadResourceFileToString(":/shaders/normals.frag");
    m_normalsShader = std::make_unique<Shader>(vertexSource, geometrySource, fragmentSource);
}

void SceneviewScene::loadNormalsArrowShader() {
    std::string vertexSource = ResourceLoader::loadResourceFileToString(":/shaders/normalsArrow.vert");
    std::string geometrySource = ResourceLoader::loadResourceFileToString(":/shaders/normalsArrow.gsh");
    std::string fragmentSource = ResourceLoader::loadResourceFileToString(":/shaders/normalsArrow.frag");
    m_normalsArrowShader = std::make_unique<Shader>(vertexSource, geometrySource, fragmentSource);
}

void SceneviewScene::render(SupportCanvas3D *context) {
    setClearColor();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_phongShader->bind();
    setGlobalData();
    setSceneUniforms(context);
    setLights();
    renderGeometry();
    glBindTexture(GL_TEXTURE_2D, 0);
    m_phongShader->unbind();

}

void SceneviewScene::setGlobalData() {
    m_phongShader->setUniform("ka", GlobalData.ka);
    m_phongShader->setUniform("kd", GlobalData.kd);
    m_phongShader->setUniform("ks", GlobalData.ks);
}

void SceneviewScene::setSceneUniforms(SupportCanvas3D *context) {
    Camera *camera = context->getCamera();
//    m_phongShader->setUniform("useLighting", settings.useLighting);
    m_phongShader->setUniform("useArrowOffsets", false);
    m_phongShader->setUniform("isShapeScene", false);
    m_phongShader->setUniform("p" , camera->getProjectionMatrix2());
    m_phongShader->setUniform("v", camera->getViewMatrix());
    m_phongShader->setUniform("m", glm::mat4{});
}

void SceneviewScene::setMatrixUniforms(Shader *shader, SupportCanvas3D *context) {
    shader->setUniform("p", context->getCamera()->getProjectionMatrix2());
    shader->setUniform("v", context->getCamera()->getViewMatrix());
    shader->setUniform("m", glm::mat4{});
}

void SceneviewScene::setLights() {
    for (auto&& x : Lights)
        m_phongShader->setLight(x);
}

void SceneviewScene::renderGeometry() {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    for (auto DisplayPort = OpenGLShape{}; auto&& [PointerToMaterial, x] : TessellatedObjects) {
        m_phongShader->applyMaterial(*PointerToMaterial);
        DisplayPort.m_vertexData = Shape::Export(x);
        DisplayPort.initializeOpenGLShapeProperties();
        DisplayPort.draw();
    }
}

void SceneviewScene::settingsChanged() {
    if (settings.fractalDepth == ShapeParameter1 && settings.fractalWidth == ShapeParameter2)
        return;

    ShapeParameter1 = settings.fractalDepth;
    ShapeParameter2 = settings.fractalWidth;
    TessellatedObjects.resize(Objects.size());

//    for (auto x : Range{ Objects.size() })
//        if (auto& [Primitive, ObjectTransformation] = Objects[x]; Primitive.type == PrimitiveType::PRIMITIVE_CUBE)
//            TessellatedObjects[x] = std::tuple{ &Primitive.material, ObjectTransformation * Shape::Spatial::DrawCube(settings.fractalDepth) };
//        else if (Primitive.type == PrimitiveType::PRIMITIVE_CYLINDER)
//            TessellatedObjects[x] = std::tuple{ &Primitive.material, ObjectTransformation * Shape::Spatial::DrawCylinder(settings.fractalDepth, std::max(settings.fractalWidth, 3)) };
//        else if (Primitive.type == PrimitiveType::PRIMITIVE_CONE)
//            TessellatedObjects[x] = std::tuple{ &Primitive.material, ObjectTransformation * Shape::Spatial::DrawCone(settings.fractalDepth, std::max(settings.fractalWidth, 3)) };
//        else if (Primitive.type == PrimitiveType::PRIMITIVE_SPHERE)
//            TessellatedObjects[x] = std::tuple{ &Primitive.material, ObjectTransformation * Shape::Spatial::DrawSphere(std::max(settings.fractalDepth, 2), std::max(settings.fractalWidth, 3)) };
//        else
//            throw std::runtime_error{ "Unrecognized primitive type detected!" };
}
