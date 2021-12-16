/**
 * @file   CamtransCamera.cpp
 *
 * This is the perspective camera class you will need to fill in for the Camtrans lab.  See the
 * lab handout for more details.
 */

#include "CamtransCamera.h"
#include <Settings.h>

CamtransCamera::CamtransCamera()
{
    this->m_near = 1;
    this->m_far = 30;
    this->m_thetaH = glm::radians(60.);
    this->m_aspectRatio = 1;
    this->m_thetaW = 2 * std::atan(this->m_aspectRatio * std::tan(this->m_thetaH / 2));
    this->m_eye = glm::vec4{ 2,2,2,1 };
    this->m_up = glm::vec4{ 0,1,0,0 };

    auto w = glm::normalize(glm::vec3{ 2,2,2 });
    auto v = glm::normalize(glm::vec3{ this->m_up } - glm::dot(glm::vec3{ this->m_up }, w) * w);
    auto u = glm::cross(v, w);

    this->m_w = glm::vec4{ w, 0 };
    this->m_v = glm::vec4{ v, 0 };
    this->m_u = glm::vec4{ u, 0 };

    updateViewMatrix();
    updateProjectionMatrix();
}

void CamtransCamera::setAspectRatio(float a)
{
    this->m_aspectRatio = a;
    this->m_thetaW = 2 * std::atan(a * std::tan(this->m_thetaH / 2));

    this->updateProjectionMatrix();
}

CS123SceneCameraData CamtransCamera::getCameraData() const {
    // @TODO: [CAMTRANS] Fill this in...
    throw 0;
}

glm::mat4x4 CamtransCamera::getProjectionMatrix() const {
    return this->getPerspectiveMatrix() * this->getScaleMatrix();
}

glm::mat4x4 CamtransCamera::getViewMatrix() const {
    return this->m_rotationMatrix * this->m_translationMatrix;
}

glm::mat4x4 CamtransCamera::getScaleMatrix() const {
    return this->m_scaleMatrix;
}

glm::mat4x4 CamtransCamera::getPerspectiveMatrix() const {
    return this->m_perspectiveTransformation;
}

glm::vec4 CamtransCamera::getPosition() const {
    return this->m_eye;
}

glm::vec4 CamtransCamera::getLook() const {
    return -this->m_w;
}

glm::vec4 CamtransCamera::getUp() const {
    // @TODO: [CAMTRANS] Fill this in...
    return glm::vec4();
}

float CamtransCamera::getAspectRatio() const {
    return this->m_aspectRatio;
}

float CamtransCamera::getHeightAngle() const {
    return glm::degrees(this->m_thetaH);
}

void CamtransCamera::orientLook(const glm::vec4 &eye, const glm::vec4 &look, const glm::vec4 &up) {
    this->m_eye = eye;
    this->m_up = glm::normalize(up);


    auto w = -glm::normalize(glm::vec3{ look });
    auto v = glm::normalize(glm::vec3{ this->m_up } - glm::dot(glm::vec3{ this->m_up }, w) * w);
    auto u = glm::cross(v, w);

    this->m_w = glm::vec4{ w, 0 };
    this->m_v = glm::vec4{ v, 0 };
    this->m_u = glm::vec4{ u, 0 };

    updateViewMatrix();
    updateProjectionMatrix();
}

void CamtransCamera::setHeightAngle(float h) {
    this->m_thetaH = glm::radians(h);
    this->m_thetaW = 2 * std::atan(this->m_aspectRatio * std::tan(this->m_thetaH / 2));
    updateProjectionMatrix();
}

void CamtransCamera::translate(const glm::vec4 &v) {
    this->m_eye.x += v.x;
    this->m_eye.y += v.y;
    this->m_eye.z += v.z;
    updateViewMatrix();
}

void CamtransCamera::rotateU(float degrees) {
    auto rad = glm::radians(degrees);
    auto rot = glm::transpose(glm::mat4{
    1,0,0,0,
    0,std::cos(rad),-std::sin(rad),0,
    0,std::sin(rad),std::cos(rad),0,
    0,0,0,1
    });

    auto rot_w_cam_space = rot * glm::vec4{ 0,0,1,0 };
    auto rot_v_cam_space = rot * glm::vec4{ 0,1,0,0 };

    this->m_w = glm::inverse(getViewMatrix()) * rot_w_cam_space;
    this->m_v = glm::inverse(getViewMatrix()) * rot_v_cam_space;



    updateViewMatrix();
}

void CamtransCamera::rotateV(float degrees) {
    auto rad = glm::radians(degrees);
    auto rot = glm::transpose(glm::mat4{
        std::cos(rad),0,std::sin(rad),0,
        0,1,0,0,
        -std::sin(rad),0,std::cos(rad),0,
        0,0,0,1
    });

    auto rot_w_cam_space = rot * glm::vec4{ 0,0,1,0 };
    auto rot_u_cam_space = rot * glm::vec4{ 1,0,0,0 };

    this->m_w = glm::inverse(getViewMatrix()) * rot_w_cam_space;
    this->m_u = glm::inverse(getViewMatrix()) * rot_u_cam_space;



    updateViewMatrix();

}

void CamtransCamera::rotateW(float degrees) {
    auto rad = glm::radians(degrees);
    auto rot = glm::transpose(glm::mat4{
        std::cos(rad),-std::sin(rad),0,0,
        std::sin(rad),std::cos(rad),0,0,
        0,0,1,0,
        0,0,0,1
    });

    auto rot_v_cam_space = rot * glm::vec4{ 0,1,0,0 };
    auto rot_u_cam_space = rot * glm::vec4{ 1,0,0,0 };


    this->m_v = glm::inverse(getViewMatrix()) * rot_v_cam_space;
    this->m_u = glm::inverse(getViewMatrix()) * rot_u_cam_space;


    updateViewMatrix();


}

void CamtransCamera::setClip(float nearPlane, float farPlane) {
    this->m_near = nearPlane;
    this->m_far = farPlane;
    updateProjectionMatrix();
}

