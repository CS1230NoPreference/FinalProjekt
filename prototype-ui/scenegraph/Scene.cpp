#include "Scene.h"
#include "Camera.h"
#include "CS123ISceneParser.h"

#include "glm/gtx/transform.hpp"


Scene::Scene()
{
}

Scene::Scene(Scene &scene)
{
    this->GlobalData = scene.GlobalData;
    this->Lights = scene.Lights;
    this->Objects = scene.Objects;

}

Scene::~Scene()
{
    // Do not delete m_camera, it is owned by SupportCanvas3D
}

auto DFSTraverse(auto SceneToFill, auto&& ParentTransformation, auto Node)->void {
    auto FusedTransformation = ParentTransformation;
    for (auto x : Node->transformations)
        FusedTransformation *= [&] {
            if (x->type == TransformationType::TRANSFORMATION_TRANSLATE)
                return glm::translate(x->translate);
            else if (x->type == TransformationType::TRANSFORMATION_SCALE)
                return glm::scale(x->scale);
            else if (x->type == TransformationType::TRANSFORMATION_ROTATE)
                return glm::rotate(x->angle, x->rotate);
            else
                return x->matrix;
        }();
    for (auto x : Node->primitives)
        SceneToFill->Objects.push_back({ *x, FusedTransformation });
    for (auto x : Node->children)
        DFSTraverse(SceneToFill, FusedTransformation, x);
}

void Scene::parse(Scene *sceneToFill, CS123ISceneParser *parser) {
    auto LightCount = parser->getNumLights();
    sceneToFill->Lights.resize(LightCount);
    parser->getGlobalData(sceneToFill->GlobalData);
    for (auto x = 0; x < LightCount; ++x)
        parser->getLightData(x, sceneToFill->Lights[x]);
    if (auto Root = parser->getRootNode(); Root != nullptr)
        DFSTraverse(sceneToFill, glm::mat4{}, Root);
}

void Scene::addPrimitive(const CS123ScenePrimitive &scenePrimitive, const glm::mat4x4 &matrix) {
}

void Scene::addLight(const CS123SceneLightData &sceneLight) {
}

void Scene::setGlobal(const CS123SceneGlobalData &global) {
}

