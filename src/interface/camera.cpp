#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

mat4 Camera::getView() const {
    return lookAt(translation, translation + orientation, vec3(0.0f, 1.0f, 0.0f));
}

mat4 Camera::getProjection() const {
    return perspective(radians(fov), aspectRatio, nearPlane, farPlane);
}
