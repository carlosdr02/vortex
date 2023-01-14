#include "camera.h"

using namespace glm;

void Camera::orientate(float xPos, float yPos) {
    static float xLast = xPos;
    static float yLast = yPos;

    static float yaw = 0.0f;
    static float pitch = 0.0f;

    float xOffset = xPos - xLast;
    float yOffset = yPos - yLast;

    xLast = xPos;
    yLast = yPos;

    xOffset *= sensitivity;
    yOffset *= sensitivity;

    yaw += xOffset;
    pitch += yOffset;

    pitch = clamp(pitch, -90.0f, 90.0f);

    vec3 orientation = {
        cos(radians(yaw)) * cos(radians(pitch)),
        sin(radians(pitch)),
        sin(radians(yaw)) * cos(radians(pitch))
    };

    this->orientation = orientation;
}

vec3 Camera::getRightVector() {
    return normalize(cross(orientation, vec3(0.0f, 1.0f, 0.0f)));
}

vec3 Camera::getForwardVector() {
    return normalize(cross(vec3(0.0f, 1.0f, 0.0f), getRightVector()));
}
