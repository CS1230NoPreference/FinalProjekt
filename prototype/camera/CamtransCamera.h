#ifndef CAMTRANSCAMERA_H
#define CAMTRANSCAMERA_H

#include "Camera.h"
#include "../glm/gtc/matrix_transform.hpp"

/**
 * @class CamtransCamera
 *.2
 * The perspective camera to be implemented in the Camtrans lab.
 */
class CamtransCamera : public Camera {
public:
    float m_aspectRatio;
    float m_near, m_far;
    glm::mat4 m_translationMatrix, m_perspectiveTransformation;
    glm::mat4 m_scaleMatrix, m_rotationMatrix;
    float m_thetaH, m_thetaW;
    glm::vec4 m_eye, m_up;
    glm::vec4 m_u, m_v, m_w;
    glm::mat4 m_perspectiveTransformation2;



    auto updatePerspectiveMatrix() {

        auto c = -m_near / m_far;
        auto p = glm::mat4{
        1,0,0,0,
        0,1,0,0,
        0,0,1/(1+c),-c/(1+c),
        0,0,-1,0
        };

        auto p2 = glm::mat4{
            1,0,0,0,
            0,1,0,0,
            0,0,-1 / (1 + c),c / (1 + c),
            0,0,-1,0
        };

        this->m_perspectiveTransformation = glm::transpose(p);
        this->m_perspectiveTransformation2 = glm::transpose(p2);
        
    }
    auto updateScaleMatrix() {
        auto scale = glm::mat4{
        1 / (m_far * std::tan(m_thetaW / 2)),0,0,0,
        0,1 / (m_far * std::tan(m_thetaH / 2)),0,0,
        0,0,1 / m_far,0,
        0,0,0,1
        };

        this->m_scaleMatrix = glm::transpose(scale);
    }

    auto updateProjectionMatrix() {
        updateScaleMatrix();
        updatePerspectiveMatrix();
    }


    auto updateRotationMatrix() {
        auto rot = glm::mat4{
        m_u.x,m_u.y,m_u.z,0,
        m_v.x,m_v.y,m_v.z,0,
        m_w.x,m_w.y,m_w.z,0,
        0,0,0,1
        };
        this->m_rotationMatrix = glm::transpose(rot);
    }
    void updateTranslationMatrix() {
        auto trans = glm::mat4{
            1,0,0,-m_eye.x,
            0,1,0,-m_eye.y,
            0,0,1,-m_eye.z,
            0,0,0,1
        };
        this->m_translationMatrix = glm::transpose(trans);
    }

    auto updateViewMatrix() {
        updateTranslationMatrix();
        updateRotationMatrix();
    }


    auto getU() const {
        return this->m_u;
    }
    auto getV() const {
        return this->m_v;
    }
    auto getW() const {
        return this->m_w;
    }
    // Initialize your camera.
    CamtransCamera();

    // Sets the aspect ratio of this camera. Automatically called by the GUI when the window is
    // resized.
    virtual void setAspectRatio(float aspectRatio);

    // Returns the projection matrix given the current camera settings.
    virtual glm::mat4x4 getProjectionMatrix() const;

    // Returns the view matrix given the current camera settings.
    virtual glm::mat4x4 getViewMatrix() const;

    // Returns the matrix that scales down the perspective view volume into the canonical
    // perspective view volume, given the current camera settings.
    virtual glm::mat4x4 getScaleMatrix() const;

    // Returns the matrix the unhinges the perspective view volume, given the current camera
    // settings.
    virtual glm::mat4x4 getPerspectiveMatrix() const;

    virtual CS123SceneCameraData getCameraData() const;

    // Returns the current position of the camera.
    glm::vec4 getPosition() const;

    // Returns the current 'look' vector for this camera.
    glm::vec4 getLook() const;

    // Returns the current 'up' vector for this camera (the 'V' vector).
    glm::vec4 getUp() const;

    // Returns the currently set aspect ratio.
    float getAspectRatio() const;

    // Returns the currently set height angle.
    float getHeightAngle() const;

    // Move this camera to a new eye position, and orient the camera's axes given look and up
    // vectors.
    void orientLook(const glm::vec4 &eye, const glm::vec4 &look, const glm::vec4 &up);

    // Sets the height angle of this camera.
    void setHeightAngle(float h);

    // Translates the camera along a given vector.
    void translate(const glm::vec4 &v);

    // Rotates the camera about the U axis by a specified number of degrees.
    void rotateU(float degrees);

    // Rotates the camera about the V axis by a specified number of degrees.
    void rotateV(float degrees);

    // Rotates the camera about the W axis by a specified number of degrees.
    void rotateW(float degrees);

    // Sets the near and far clip planes for this camera.
    void setClip(float nearPlane, float farPlane);



    auto getProjectionMatrix2() const -> glm::mat4x4 {
        return this->m_perspectiveTransformation2 * this->getScaleMatrix();
    }

};

#endif // CAMTRANSCAMERA_H
