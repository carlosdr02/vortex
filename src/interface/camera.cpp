#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

mat4 Camera::getView() const {
    return lookAt(translation, translation + orientation, vec3(0.0f, 1.0f, 0.0f));
}

mat4 Camera::getProjection() const {
    return perspective(radians(fov), aspectRatio, nearPlane, farPlane);
}

static float clamp(float val, float min, float max) {
    if (val < min) {
        return min;
    }

    if (val > max) {
        return max;
    }

    return val;
}

void cursorPosCallback(GLFWwindow* window, double xPos, double yPos) {
    static float xLast = xPos;
    static float yLast = yPos;

    static float yaw = -90.0f;
    static float pitch = 0.0f;

    float xOffset = xPos - xLast;
    float yOffset = yLast - yPos;

    xLast = xPos;
    yLast = yPos;

    Camera* camera = (Camera*)glfwGetWindowUserPointer(window);

    xOffset *= camera->sensitivity;
    yOffset *= camera->sensitivity;

    yaw += xOffset;
    pitch += yOffset;

    pitch = clamp(pitch, -89.9f, 89.9f);

    float rYaw = radians(yaw);
    float rPitch = radians(pitch);

    float cPitch = cos(rPitch);

    vec3 orientation = {
        cos(rYaw) * cPitch,
        sin(rPitch),
        sin(rYaw) * cPitch
    };

    camera->orientation = normalize(orientation);
}
