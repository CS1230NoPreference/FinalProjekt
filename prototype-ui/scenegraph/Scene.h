#ifndef SCENE_H
#define SCENE_H

#include "CS123SceneData.h"
#include <tuple>

class Camera;
class CS123ISceneParser;


/**
 * @class Scene
 *
 * @brief This is the base class for all scenes. Modify this class if you want to provide
 * common functionality to all your scenes.
 */
class Scene {
public:
    using ObjectType = std::tuple<CS123ScenePrimitive, glm::mat4x4>;
    CS123SceneGlobalData GlobalData = { 1, 1, 1, 1 };
    std::vector<CS123SceneLightData> Lights = {};
    std::vector<ObjectType> Objects = {};

public:
    Scene();
    Scene(Scene &scene);
    virtual ~Scene();

    virtual void settingsChanged() {}

    static void parse(Scene *sceneToFill, CS123ISceneParser *parser);

protected:

    // Adds a primitive to the scene.
    virtual void addPrimitive(const CS123ScenePrimitive &scenePrimitive, const glm::mat4x4 &matrix);

    // Adds a light to the scene.
    virtual void addLight(const CS123SceneLightData &sceneLight);

    // Sets the global data for the scene.
    virtual void setGlobal(const CS123SceneGlobalData &global);

};

#endif // SCENE_H
