#include <glitter/vulkan/vulkan-helpers.h>

#include <vulkan/vulkan.h>

#include <array>
#include <cassert>
#include <memory>
#include <utility>
#include <cstdint>

namespace {
bool ValidationLayerFound(std::uint32_t const checkCount,
                          char const* const* const check_names,
                          std::uint32_t const layer_count,
                          VkLayerProperties* layers) {

    for (uint32_t i = 0; i < checkCount; i++) {
        bool found = false;

        for (uint32_t j = 0; j < layer_count; j++) {
            if (!strcmp(check_names[i], layers[j].layerName)) {
                found = true;
                break;
            }
        }
        if (!found) { return false; }
    }

    return true;
}

char const* const instanceValidationLayersAlt1[] = {"VK_LAYER_LUNARG_standard_validation"};
const int layersAlt1Count = sizeof(instanceValidationLayersAlt1) / sizeof(instanceValidationLayersAlt1[0]);

char const* const instanceValidationLayersAlt2[] = {"VK_LAYER_GOOGLE_threading",
                                                    "VK_LAYER_LUNARG_parameter_validation",
                                                    "VK_LAYER_LUNARG_object_tracker",
                                                    "VK_LAYER_LUNARG_core_validation",
                                                    "VK_LAYER_GOOGLE_unique_objects"};
const int layersAlt2Count = sizeof(instanceValidationLayersAlt2) / sizeof(instanceValidationLayersAlt2[0]);

std::tuple<std::uint32_t, char const* const*, std::uint32_t>
EnableDebugLayer(int const instanceLayerCount, VkLayerProperties* vkLayerProps) {

    std::uint32_t validationLayerCount          = 0;
    char const* const* instanceValidationlayers = nullptr;
    std::uint32_t enabledLayerCount             = 0;

    instanceValidationlayers = instanceValidationLayersAlt1;

    bool validationFound = false;

    if (instanceLayerCount > 0) {

        validationFound =
            ValidationLayerFound(layersAlt1Count, instanceValidationlayers, instanceLayerCount, vkLayerProps);

        if (validationFound) {
            enabledLayerCount = layersAlt1Count;
            // enabled_layers[0]      = "VK_LAYER_LUNARG_standard_validation";
            validationLayerCount = layersAlt1Count;

        } else {
            instanceValidationlayers = instanceValidationLayersAlt2;
            enabledLayerCount        = layersAlt2Count;

            validationFound = validationFound = ValidationLayerFound(
                layersAlt1Count, instanceValidationlayers, instanceLayerCount, vkLayerProps);

            validationLayerCount = layersAlt2Count;

            // for (uint32_t i = 0; i < validationLayerCount; i++) {
            //    enabled_layers[i] = instanceValidationlayers[i];
            //}
        }
    }

    return {validationLayerCount, instanceValidationlayers, enabledLayerCount};
}

bool HasExtensionName(VkExtensionProperties* extensions,
                      std::uint32_t instanceExtensionCount,
                      char const* extension_names) {
    for (uint32_t i = 0; i < instanceExtensionCount; i++) {
        if (!strcmp(extension_names, extensions[i].extensionName)) { return true; }
    }

    return false;
}

VkApplicationInfo CreateApplicationInfo() {
    VkApplicationInfo appInfo  = {};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName        = "No Engine";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    return appInfo;
}

VkInstanceCreateInfo CreateInstanceInfo(VkApplicationInfo const& appInfo,
                                        std::uint32_t const extensionCount,
                                        char const* const* enabledExtensionNames,
                                        std::uint32_t const enabledLayersCount,
                                        char const* const* enabledLayerNames) {

    VkInstanceCreateInfo createInfo    = {};
    createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo        = &appInfo;
    createInfo.enabledExtensionCount   = extensionCount;
    createInfo.ppEnabledExtensionNames = enabledExtensionNames;
    createInfo.enabledLayerCount       = enabledLayersCount;
    createInfo.ppEnabledLayerNames     = enabledLayerNames;

    return createInfo;
}

} // namespace

namespace glitt {
gget::Error Vulkan::Init(bool const enableDebugLayer) {

    auto const appInfo = CreateApplicationInfo();

    std::uint32_t instanceLayerCount = 0;
    {
        auto const result = vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
        if (result != VK_SUCCESS) {
            return {"vkEnumerateInstanceLayerProperties failed to populate layer count"};
        }
    }

    auto const instanceLayers = std::make_unique<VkLayerProperties[]>(instanceLayerCount);
    {
        auto const result = vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.get());

        if (result != VK_SUCCESS)
            return {"vkEnumerateInstanceLayerProperties failed to populate instance layers"};
    }

    auto const [validationLayerCount, instanceValidationlayers, enabledLayerCount] =
        EnableDebugLayer(instanceLayerCount, instanceLayers.get());
    if (enableDebugLayer) {

        if (validationLayerCount == 0) return {"Failed to initialise debug validation layer"};
    }

    std::uint32_t instanceExtensionCount = 0;
    {
        auto const result = vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);

        if (result != VK_SUCCESS)
            return {"vkEnumerateInstanceExtensionProperties failed to populate instance extension layers"};
    }

    auto const instanceExtensions = std::make_unique<VkExtensionProperties[]>(instanceExtensionCount);
    {
        auto const result = vkEnumerateInstanceExtensionProperties(
            nullptr, &instanceExtensionCount, instanceExtensions.get());
        if (result != VK_SUCCESS)
            return {"vkEnumerateInstanceExtensionProperties failed to populate instance extension layers"};
    }

    bool const surfaceExtFound =
        HasExtensionName(instanceExtensions.get(), instanceExtensionCount, VK_KHR_SURFACE_EXTENSION_NAME);

    if (!surfaceExtFound) return {"Failed to find the surface extension. Possible missing Vulkan driver"};

    bool const platformSurfaceExtFound = HasExtensionName(
        instanceExtensions.get(), instanceExtensionCount, VK_KHR_WIN32_SURFACE_EXTENSION_NAME);

    if (!platformSurfaceExtFound)
        return {"Failed to find the platform surface extension. Possible missing Vulkan driver"};

    char const* extensionNames[64] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};

    auto const instanceInfo =
        CreateInstanceInfo(appInfo, 2, extensionNames, enabledLayerCount, instanceValidationlayers);

    VkInstance instance;
    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
        return {"Failed to create Vulkan Instance"};

    return gget::Error::NoError;
}

} // namespace glitt