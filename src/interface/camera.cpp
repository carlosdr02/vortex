#include "camera.h"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

void Camera::update(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_W)) translation += getForwardVector() * speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_A)) translation -= getRightVector() * speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_S)) translation -= getForwardVector() * speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_D)) translation += getRightVector() * speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_SPACE)) translation += vec3(0.0f, 1.0f, 0.0f) * speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) translation -= vec3(0.0f, 1.0f, 0.0f) * speed * deltaTime;
}

mat4 Camera::getInverseViewMatrix() const {
    return inverse(lookAt(translation, translation + orientation, vec3(0.0f, 1.0f, 0.0f)));
}

mat4 Camera::getInverseProjectionMatrix(float aspectRatio) const {
    return inverse(perspective(radians(fov), aspectRatio, nearPlane, farPlane));
}

vec3 Camera::getRightVector() const {
    return normalize(cross(orientation, vec3(0.0f, 1.0f, 0.0f)));
}

vec3 Camera::getForwardVector() const {
    return normalize(cross(vec3(0.0f, 1.0f, 0.0f), getRightVector()));
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

    Camera* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));

    xOffset *= camera->sensitivity;
    yOffset *= camera->sensitivity;

    yaw += xOffset;
    pitch += yOffset;

    pitch = clamp(pitch, -89.9f, 89.9f);

    vec3 orientation = {
        cos(radians(yaw)) * cos(radians(pitch)),
        sin(radians(pitch)),
        sin(radians(yaw)) * cos(radians(pitch))
    };

    camera->orientation = normalize(orientation);
}
