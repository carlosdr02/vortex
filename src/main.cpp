#include <graphics.h>

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    const char* applicationName = "Achantcraft";

    VkInstance instance = createInstance(applicationName, VK_MAKE_VERSION(1, 0, 0));

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT debugMessenger = createDebugMessenger(instance);
#endif // _DEBUG

    Window window(instance, 1600, 900, applicationName);

    Device device(instance, window);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    window.destroy();

#ifdef _DEBUG
    destroyDebugMessenger(instance, debugMessenger);
#endif // _DEBUG

    vkDestroyInstance(instance, nullptr);

    glfwTerminate();
}
