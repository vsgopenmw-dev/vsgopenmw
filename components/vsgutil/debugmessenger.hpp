#ifndef VSGOPENMW_VSGUTIL_DEBUGMESSENGER_H
#define VSGOPENMW_VSGUTIL_DEBUGMESSENGER_H

#include <iostream>

#include <vsg/vk/Instance.h>

namespace vsgUtil
{
    /*
     * DebugMessenger is an extension of vsg::Instance used to catch message output of validation layers.
     * By default, logs errors and warnings to std::cerr, then calls the error() or warning() method which can be used as a breakpoint.
     * Users must enable the instance extension VK_EXT_debug_utils.
     */
    class DebugMessenger
    {
        VkDebugUtilsMessengerEXT _messenger = VK_NULL_HANDLE;
        vsg::ref_ptr<vsg::Instance> _instance;
    public:
        DebugMessenger(vsg::ref_ptr<vsg::Instance> instance,
                VkDebugUtilsMessageSeverityFlagsEXT severityFlags = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT,
                VkDebugUtilsMessageTypeFlagsEXT typeFlags = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
        {
            _instance = instance;

            if (auto pfnCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_instance->vk(), "vkCreateDebugUtilsMessengerEXT"))
            {
                VkDebugUtilsMessengerCreateInfoEXT createInfo = {
                    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                    .pNext = nullptr,
                    .flags = 0,
                    .messageSeverity = severityFlags,
                    .messageType= typeFlags,
                    .pfnUserCallback = DebugMessenger::callback,
                    .pUserData = this
                };
                auto res = pfnCreateDebugUtilsMessengerEXT(_instance->vk(), &createInfo, nullptr, &_messenger);
                if (res != VK_SUCCESS)
                {
                    std::cerr << "vkCreateDebugUtilsMessengerEXT failed with VkResult = " << res << std::endl;
                }
                else
                    std::cout << "VkDebugUtilsMessengerEXT has been set up successfully. Validation errors will pass through vsgUtil::DebugMessenger::error()." << std::endl;
            }
            else
                std::cerr << "DebugMessenger::DebugMessenger(..) failed, VK_EXT_debug_utils is not enabled." << std::endl;
        }

        ~DebugMessenger()
        {
            if (_messenger != VK_NULL_HANDLE)
            {
                if (auto pfnDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(_instance->vk(), "vkDestroyDebugUtilsMessengerEXT"))
                    pfnDestroyDebugUtilsMessengerEXT(_instance->vk(), _messenger, nullptr);
            }
        }

        static VkBool32 callback(VkDebugUtilsMessageSeverityFlagBitsEXT severityFlags, VkDebugUtilsMessageTypeFlagsEXT typeFlags, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user)
        {
            static_cast<DebugMessenger*>(user)->message(severityFlags, typeFlags, data);
        }

        virtual VkBool32 message(VkDebugUtilsMessageSeverityFlagBitsEXT severityFlags, VkDebugUtilsMessageTypeFlagsEXT typeFlags, const VkDebugUtilsMessengerCallbackDataEXT* data)
        {
            std::cerr << data->pMessage << std::endl;
            if (severityFlags & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
                error();
            if (severityFlags & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
                warning();
            return true;
        }
        virtual void error() {}
        virtual void warning() {}
    };
}

#endif
