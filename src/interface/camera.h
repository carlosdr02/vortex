#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

class Camera {
public:
    glm::vec3 translation;
    glm::vec3 orientation;
    float speed;
    float sensitivity;
    float fov;
    float aspectRatio;
    float nearPlane;
    float farPlane;

    glm::mat4 getView() const;
    glm::mat4 getProjection() const;
};

#endif // !CAMERA_H
