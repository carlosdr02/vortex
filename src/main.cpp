#include <stdio.h>

#include <graphics.h>
#include <camera.h>

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    const char* applicationName = "Achantcraft";

    VkInstance instance = createInstance(applicationName, VK_MAKE_VERSION(1, 0, 0));

#ifdef _DEBUG
    VkDebugUtilsMessengerEXT debugMessenger = createDebugMessenger(instance);
#endif // _DEBUG

    int width = 1600, height = 900;
    Window window(instance, width, height, applicationName);

    Device device(instance, window.surface);

    VkSurfaceCapabilitiesKHR surfaceCapabilities = device.getSurfaceCapabilities(window);
    VkSurfaceFormatKHR surfaceFormat = device.getSurfaceFormat(window.surface);
    VkPresentModeKHR presentMode = device.getSurfacePresentMode(window.surface);
    VkFormat depthFormat = device.getDepthStencilFormat();

    VkRenderPass renderPass = createRenderPass(device.logical, surfaceFormat.format, depthFormat);

    VkQueue graphicsQueue = device.getQueue(0);
    VkQueue presentQueue = device.getQueue(1);

    RendererCreateInfo rendererCreateInfo = {
        .surface             = window.surface,
        .surfaceCapabilities = &surfaceCapabilities,
        .surfaceFormat       = surfaceFormat,
        .presentMode         = presentMode,
        .depthFormat         = depthFormat,
        .renderPass          = renderPass,
        .framesInFlight      = 3,
        .graphicsQueue       = graphicsQueue,
        .presentQueue        = presentQueue
    };

    Renderer renderer(device, rendererCreateInfo);

    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                           = nullptr,
        .flags                           = 0,
        .vertexBindingDescriptionCount   = 0,
        .pVertexBindingDescriptions      = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions    = nullptr
    };

    VkPipelineLayout pipelineLayout = createPipelineLayout(device.logical, 1, &renderer.descriptorSetLayout);

    GraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
        .vertexShaderPath           = "../src/engine/shaders/vert.spv",
        .fragmentShaderPath         = "../src/engine/shaders/frag.spv",
        .vertexInputStateCreateInfo = &pipelineVertexInputStateCreateInfo,
        .polygonMode                = VK_POLYGON_MODE_FILL,
        .pipelineLayout             = pipelineLayout,
        .renderPass                 = renderPass
    };

    VkPipeline graphicsPipeline = createGraphicsPipeline(device.logical, graphicsPipelineCreateInfo);

    renderer.recordCommandBuffers(device.logical, renderPass, surfaceCapabilities.currentExtent, graphicsPipeline, pipelineLayout);

    Camera camera = {
        .translation = glm::vec3(0.0f, 0.0f, 1.0f),
        .orientation = glm::vec3(0.0f, 0.0f, -1.0f),
        .speed       = 1.0f,
        .sensitivity = 0.1f,
        .fov         = 65.0f,
        .aspectRatio = width / (float)height,
        .nearPlane   = 0.1f,
        .farPlane    = 100.0f
    };

    glfwSetWindowUserPointer(window, &camera);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        camera.update(window, deltaTime);

        glm::mat4 cameraData[] = {
            camera.getView(),
            camera.getProjection()
        };

        if (!renderer.draw(device.logical, cameraData)) {
            do {
                glfwGetFramebufferSize(window, &width, &height);
                glfwWaitEvents();
            } while(width == 0 || height == 0);

            surfaceCapabilities = device.getSurfaceCapabilities(window);

            renderer.waitIdle(device.logical);
            renderer.recreate(device, rendererCreateInfo);
            renderer.recordCommandBuffers(device.logical, renderPass, surfaceCapabilities.currentExtent, graphicsPipeline, pipelineLayout);

            camera.aspectRatio = width / (float)height;
        }

        printf("Frame time: %f\n", deltaTime);
    }

    renderer.waitIdle(device.logical);

    vkDestroyPipeline(device.logical, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device.logical, pipelineLayout, nullptr);

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
