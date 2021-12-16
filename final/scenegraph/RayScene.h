#ifndef RAYSCENE_H
#define RAYSCENE_H

#include "Scene.h"

#include <vector>


/**
 * @class RayScene
 *
 *  Students will implement this class as necessary in the Ray project.
 */
class RayScene : public Scene {
public:
    RayScene(Scene &scene);
    virtual ~RayScene();

};

#endif // RAYSCENE_H
