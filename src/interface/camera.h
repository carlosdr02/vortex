#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

#include <GLFW/glfw3.h>

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

    void update(GLFWwindow* window, float deltaTime);

    glm::mat4 getView() const;
    glm::mat4 getProjection() const;

private:
    glm::vec3 getRight() const;
    glm::vec3 getFront() const;
};

void cursorPosCallback(GLFWwindow* window, double xPos, double yPos);

#endif // !CAMERA_H
