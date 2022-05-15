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

    glm::mat4 getViewProjection() const;

private:
    glm::vec3 getRight() const;
    glm::vec3 getFront() const;
};

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
void cursorPosCallback(GLFWwindow* window, double xPos, double yPos);
void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

#endif // !CAMERA_H
