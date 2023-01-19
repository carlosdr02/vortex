#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class Camera {
public:
    glm::vec3 translation = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 orientation = glm::vec3(0.0f, 0.0f, -1.0f);
    float fov = 90.0f;
    float sensitivity = 0.1f;
    float speed = 1.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

    void update(GLFWwindow* window, float deltaTime);

    glm::mat4 getInverseViewMatrix() const;
    glm::mat4 getInverseProjectionMatrix(float aspectRatio) const;

private:
    glm::vec3 getRightVector() const;
    glm::vec3 getForwardVector() const;
};

void cursorPosCallback(GLFWwindow* window, double xPos, double yPos);
