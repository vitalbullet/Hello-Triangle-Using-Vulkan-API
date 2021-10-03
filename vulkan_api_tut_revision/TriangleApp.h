#pragma once
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
#include<stdexcept>
#include<iostream>
#include<vector>
#include<cstring>
#include<algorithm>
#include<fstream>

#define	debug(x)		std::cerr<<(#x)<<" : "<<(x)<<std::endl;
#define debug2(x,y)		std::cerr<<(#x)<<" : "<<(x)<<", "<<(#y)<<" : "<<(y)<<std::endl;
#define debug3(x,y,z)	std::cerr<<(#x)<<" : "<<(x)<<", "<<(#y)<<" : "<<(y)<<", "<<(#z)<<" : "<<(z)<<std::endl;

typedef struct QueueFamilyIndices {
	std::vector<uint32_t> graphics, compute, transfer, sparseBinding, presentation;
	bool isComplete() {
		return graphics.size() && presentation.size();
	}
} QueueFamilyIndices;

typedef struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
} SwapChainSupportDetails;

const uint32_t WIDTH = 800, HEIGHT = 600;
const std::vector<const char*> deviceExtensions{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

class TriangleApp
{
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanUp();
	}
private:
	GLFWwindow* window;
	
	VkInstance instance;
	
	VkSurfaceKHR surface;

	VkPhysicalDevice physicalDevice;
	VkPhysicalDeviceFeatures deviceFeatures;
	QueueFamilyIndices selectedDevicesQueueFamilyIndices;
	
	VkDevice device;
	VkQueue graphicsQueue,presentationQueue;

	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	
	std::vector<VkImageView> swapChainImageViews;

	VkRenderPass renderPass;

	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkCommandPool commandPool;

	std::vector<VkCommandBuffer> commandBuffers;

	VkSemaphore imageAvailableSemaphore, renderFinishedSemaphore;

	void initWindow();

	void initVulkan();
	void createInstance();
	void createSurface();
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSwapChain();
	void createImageViews();
	void createRenderPass();
	void createGraphicsPipeline();
	void createFramebuffers();
	void createCommandPool();
	void createCommandBuffers();
	void createSemaphores();

	void mainLoop();

	void drawFrame();

	void cleanUp();
};

static std::vector<char> readFile(const char*);

void deviceDetails(VkSurfaceKHR ,VkPhysicalDevice&);
bool isDeviceSuitable(VkPhysicalDevice&, VkSurfaceKHR&);
bool checkDeviceExtensionSupport(VkPhysicalDevice&);
void getQueueFamilyIndices(QueueFamilyIndices&, VkPhysicalDevice&, VkSurfaceKHR&);
void getSwapChainSupportDetails(SwapChainSupportDetails&, VkPhysicalDevice&, VkSurfaceKHR&);

void getChosenFormat(VkSurfaceFormatKHR&, const std::vector<VkSurfaceFormatKHR>&);
void getChosenPresentationMode(VkPresentModeKHR&, const std::vector<VkPresentModeKHR>&);
void getChosenExtent(VkExtent2D&, const VkSurfaceCapabilitiesKHR&);

VkShaderModule createShaderModule(const VkDevice&, const std::vector<char>&);