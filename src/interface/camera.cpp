#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

void Camera::update(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_W)) translation += getFront() * speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_A)) translation -= getRight() * speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_S)) translation -= getFront() * speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_D)) translation += getRight() * speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_SPACE)) translation += vec3(0.0f, 1.0f, 0.0f) * speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL)) translation -= vec3(0.0f, 1.0f, 0.0f) * speed * deltaTime;
}

mat4 Camera::getViewProjection() const {
    mat4 view = lookAt(translation, translation + orientation, vec3(0.0f, 1.0f, 0.0f));
    mat4 projection = perspective(radians(fov), aspectRatio, nearPlane, farPlane);

    return projection * view;
}

vec3 Camera::getRight() const {
    return normalize(cross(orientation, vec3(0.0f, 1.0f, 0.0f)));
}

vec3 Camera::getFront() const {
    return normalize(cross(vec3(0.0f, 1.0f, 0.0f), getRight()));
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

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Camera* camera = (Camera*)glfwGetWindowUserPointer(window);

    switch (key) {
        case GLFW_KEY_LEFT_SHIFT: {
            const float speedFactor = 1.75f;

            switch (action) {
                case GLFW_PRESS: camera->speed *= speedFactor; break;
                case GLFW_RELEASE: camera->speed /= speedFactor; break;
            }

            break;
        }
    }
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

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    Camera* camera = (Camera*)glfwGetWindowUserPointer(window);

    switch (button) {
        case GLFW_MOUSE_BUTTON_4: {
            const float zoomFactor = 0.1f;

            switch (action) {
                case GLFW_PRESS:
                    camera->fov *= zoomFactor;
                    camera->sensitivity *= zoomFactor;

                    break;
                case GLFW_RELEASE:
                    camera->fov /= zoomFactor;
                    camera->sensitivity /= zoomFactor;

                    break;
            }

            break;
        }
    }
}
