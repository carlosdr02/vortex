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

    Device device(instance, window.surface);

    VkSurfaceCapabilitiesKHR surfaceCapabilities = device.getSurfaceCapabilities(window);
    VkSurfaceFormatKHR surfaceFormat = device.getSurfaceFormat(window.surface);
    VkPresentModeKHR presentMode = device.getSurfacePresentMode(window.surface);
    VkFormat depthFormat = device.getDepthStencilFormat();

    VkRenderPass renderPass = createRenderPass(device.logical, surfaceFormat.format, depthFormat);

    RendererCreateInfo rendererCreateInfo = {
        .surface             = window.surface,
        .surfaceCapabilities = &surfaceCapabilities,
        .surfaceFormat       = surfaceFormat,
        .presentMode         = presentMode,
        .depthFormat         = depthFormat,
        .renderPass          = renderPass,
        .framesInFlight      = 3
    };

    Renderer renderer(device, rendererCreateInfo);
    renderer.recordCommandBuffers(device.logical, renderPass, surfaceCapabilities.currentExtent);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        if (!renderer.draw(device.logical)) {
            int width, height;

            do {
                glfwGetFramebufferSize(window, &width, &height);
                glfwWaitEvents();
            } while(width == 0 || height == 0);

            surfaceCapabilities = device.getSurfaceCapabilities(window);
            renderer.recreate(device, rendererCreateInfo);
            renderer.recordCommandBuffers(device.logical, renderPass, surfaceCapabilities.currentExtent);
        }
    }

    renderer.destroy(device.logical);

    vkDestroyRenderPass(device.logical, renderPass, nullptr);

    device.destroy();
    window.destroy(instance);

#ifdef _DEBUG
    destroyDebugMessenger(instance, debugMessenger);
#endif // _DEBUG

    vkDestroyInstance(instance, nullptr);

    glfwTerminate();
}
