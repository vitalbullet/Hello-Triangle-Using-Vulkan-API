#include "TriangleApp.h"

void TriangleApp::initWindow()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(WIDTH, HEIGHT, "TriangleApp", nullptr, nullptr);
}

void TriangleApp::initVulkan()
{
	createInstance();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createCommandPool();
	createCommandBuffers();
	createSemaphores();
}

void TriangleApp::createInstance()
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "TriangleApp";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
	appInfo.apiVersion = VK_API_VERSION_1_2;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	uint32_t glfwExtensionCount{ 0 };
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	//debug(glfwExtensionCount)	for (uint32_t i = 0; i < glfwExtensionCount; ++i)	debug(glfwExtensions[i]);

	uint32_t supportedExtensionsCount{ 0 };
	vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionsCount, nullptr);

	std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionsCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionsCount, supportedExtensions.data());

	//debug(supportedExtensionsCount)	for (auto& extension : supportedExtensions) debug(extension.extensionName);

	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;

	createInfo.enabledLayerCount = 0;
	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create an instance!");
	}
}
void TriangleApp::createSurface()
{
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create window sukrface!");
	}
}

void TriangleApp::pickPhysicalDevice()
{
	physicalDevice = VK_NULL_HANDLE;

	uint32_t deviceCount{ 0 };
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if(!deviceCount){
		throw std::runtime_error("Failed to find any device with Vulkan Support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	//debug(deviceCount);	for (auto& it : devices)	deviceDetails(surface,it);

	for (auto& device : devices) {
		if (isDeviceSuitable(device, surface)) {
			//Select the first device which is suitable 
			physicalDevice = device;
			getQueueFamilyIndices(selectedDevicesQueueFamilyIndices, device, surface);
			break;
		}
	}


	if (physicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("Failed to find suitable GPU!");
	}
}

void TriangleApp::createLogicalDevice()
{
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

	//Graphics QueueCreateInfo - for rendering andd stuff
	VkDeviceQueueCreateInfo QcreateInfo{};
	QcreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

	//Probably add a check whether QfamilyIndices.graphics[0] exist or not
	if (selectedDevicesQueueFamilyIndices.graphics.size() == 0) {
		throw std::runtime_error("no Qfamily with GRAPHICS_BIT(for rendering) on");
	}
	QcreateInfo.queueFamilyIndex = selectedDevicesQueueFamilyIndices.graphics[0];
	QcreateInfo.queueCount = 1;

	float queuePriority{ 1.0f };
	QcreateInfo.pQueuePriorities = &queuePriority;

	queueCreateInfos.emplace_back(QcreateInfo);
	
	//Presentation QeueuCreateInfo - for presenting the rendered image onto the device
	if (selectedDevicesQueueFamilyIndices.presentation.size() == 0) {
		throw std::runtime_error("no QFamily to Present to Surface");
	}
	QcreateInfo.queueFamilyIndex = selectedDevicesQueueFamilyIndices.presentation[0];
	
	queueCreateInfos.emplace_back(QcreateInfo);

	/*for (const auto createInfo : queueCreateInfos) {
		debug(createInfo.sType);
		debug(createInfo.queueFamilyIndex);
		debug(createInfo.queueCount);
		debug(*(createInfo.pQueuePriorities))
	}*/

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

	//not needed for the current porject
	//vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device");
	}

	//0-> index of the Q in familiy Queue as familyQueue can consit of many queues
	vkGetDeviceQueue(device, selectedDevicesQueueFamilyIndices.graphics[0], 0, &graphicsQueue);
	vkGetDeviceQueue(device, selectedDevicesQueueFamilyIndices.presentation[0], 0, &presentationQueue);

}

void TriangleApp::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport; 
	getSwapChainSupportDetails(swapChainSupport, physicalDevice, surface);

	VkSurfaceFormatKHR chosenFormat;
	getChosenFormat(chosenFormat, swapChainSupport.formats);

	VkPresentModeKHR chosenPresentationMode;
	getChosenPresentationMode(chosenPresentationMode, swapChainSupport.presentModes);

	VkExtent2D chosenExtent;
	getChosenExtent(chosenExtent, swapChainSupport.capabilities);

	uint32_t imageCount{ swapChainSupport.capabilities.minImageCount+1};
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = chosenFormat.format;
	createInfo.imageColorSpace = chosenFormat.colorSpace;
	createInfo.imageExtent = chosenExtent;
	
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	if (selectedDevicesQueueFamilyIndices.graphics[0] != selectedDevicesQueueFamilyIndices.presentation[0]) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		uint32_t indicesArray[] = { selectedDevicesQueueFamilyIndices.graphics[0],	selectedDevicesQueueFamilyIndices.presentation[0] };
		createInfo.pQueueFamilyIndices = indicesArray;
	}else{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = chosenPresentationMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;
	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	//Retrieving imageCount from the swapChain created
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = chosenFormat.format;
	swapChainExtent = chosenExtent;
}

void TriangleApp::createImageViews()
{
	swapChainImageViews.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];

		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image view!");
		}
	}
}

void TriangleApp::createRenderPass() 
{
	//Single Color Attachment represented by 1 image from the swap chain
	VkAttachmentDescription colorAttachment{};
	//format from swpChain
	colorAttachment.format = swapChainImageFormat;
	//no multisampling yet
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;

	//what to do with attachment data b4 rendering => clear operation to clear framebuffer to black b4 rendering
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	//what to do with attachment data after rendering => Rendered contents will be stored in memory and can be read later
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

	//what to do with stencil data, since we are not using any stencil data therefore
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//Texture and Framrbuffers in Vulkan are represented by VkImage objects with a certain pixel format, however layout of pixel in memmory changes
	//can change bsed on what we are trying to do with an image

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	//A single render pass can consist of multiple subpasses. Subpasses are subsequent rendering operations that depend on the contents of the 
	//framebuffers in previous passes. if we group these subpasses into a single renderpass, vulkan can reorder subpasses to optimize the code
	//and give some better performance

	//Every subpass references one or more of the attachments ^^
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");
}

void TriangleApp::createGraphicsPipeline()
{
	auto vertShaderCode = readFile("../shaders/vert.spv");
	auto fragShaderCode = readFile("../shaders/frag.spv");

	//debug2(vertShaderCode.size(),fragShaderCode.size())

	VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
	VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{}, fragShaderStageInfo{};
	vertShaderStageInfo.sType = fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	vertShaderStageInfo.module = vertShaderModule;	fragShaderStageInfo.module = fragShaderModule;
	vertShaderStageInfo.pName = fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[]{ vertShaderStageInfo,fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 0;
	vertexInputInfo.pVertexBindingDescriptions = nullptr;
	vertexInputInfo.vertexAttributeDescriptionCount = 0;
	vertexInputInfo.pVertexAttributeDescriptions = nullptr;

	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;
	
	//Viewport and Scissors

	VkViewport viewport{};
	viewport.x = viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChainExtent.width);
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.offset = { 0,0 };
	scissor.extent = swapChainExtent;

	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;		//pointer to array of VkViewport(s)
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;			//pointer to array of VkRect2D or scissors

	//Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	//BackFace Culled and Front face = Clockwise
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;

	//Multisampling is disbled now
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;

	//Color Blending is disabled => colors from fragment shaders are unmodified and written to fragmentbuffer
	//First way -> mix the old and new to produce a final color

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	//Per FrameBuffer struct, since we are creating single buffer that's y only 1 object of this struct
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	/*
	* equal to
	* if(blenEnable){
		finlColor.rgb = newColor.rgb*SrcColorBlendFactor <VK_BLEND_OP> oldColor.rgb*dstColorBlendFactor
		finlColor.alpha = newColor.alpha*SrcColorBlendFactor <VK_BLEND_OP> oldColor.alpha*dstColorBlendFactor
	  }else{
		finlColor = newColor
	  }
	  finlColor &= colorWriteMask 
	*/

	//how to implement alpha blending , we want to blend new color to old color based on the opacity
	/*
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	*/

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;					//using the first way
	colorBlending.logicOp = VK_LOGIC_OP_COPY;				//For second way which binary OP to use
	colorBlending.attachmentCount = 1;						//colorBlendAttachment count typically =  to #of framebuffers to render to
	colorBlending.pAttachments = &colorBlendAttachment;		//pointer to array of BlendAttachments for each framebuffer to render to
	colorBlending.blendConstants[0] = 0.0f;					//constants/factors in colorBlendAttachment for each color RGB+Alpha
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;

	VkDynamicState dynamicStates[] {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pSetLayouts = nullptr;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline Layout!");

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;

	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = nullptr;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;

	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;

	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
		throw std::runtime_error("failed to create graphics Pipelines!");

	vkDestroyShaderModule(device, fragShaderModule, nullptr);
	vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void TriangleApp::createFramebuffers()
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
		VkImageView attachments[]{
			swapChainImageViews[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffers!");
	}
}

void TriangleApp::createCommandPool()
{
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = selectedDevicesQueueFamilyIndices.graphics[0];
	poolInfo.flags = 0;

	if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create command pool!");
}

void TriangleApp::createCommandBuffers()
{
	commandBuffers.resize(swapChainFramebuffers.size());
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
		throw std::runtime_error("failed to allocate command buffers");

	for (size_t i = 0; i < commandBuffers.size(); ++i) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
			throw std::runtime_error("failed to begin recording command buffer!");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[i];

		renderPassInfo.renderArea.offset = { 0,0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		VkClearValue clearColor{ 0.0f,0.0f,0.0f,0.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffers[i],&renderPassInfo,VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

		vkCmdEndRenderPass(commandBuffers[i]);

		if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to record command buffer!");
	}

}

void TriangleApp::createSemaphores()
{
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS)
		throw std::runtime_error("failed to create image available semaphore!");
	if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS)
		throw std::runtime_error("failed to create render finished semaphore!");
}

void TriangleApp::mainLoop()
{
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		drawFrame();
	}
}

void TriangleApp::drawFrame() 
{
	uint32_t imageIndex;
	vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	
	VkSemaphore waitSemaphores[]{ imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[]{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask =  waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[]{ renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}
}

void TriangleApp::cleanUp()
{
	vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
	vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);	

	vkDestroyCommandPool(device, commandPool, nullptr);
	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	vkDestroyPipeline(device, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	
	vkDestroyRenderPass(device, renderPass, nullptr);

	for (auto ImageView : swapChainImageViews) {
		vkDestroyImageView(device,ImageView,nullptr);
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);
	vkDestroyDevice(device, nullptr);

	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);
	glfwTerminate();
}

static std::vector<char> readFile(const char* fileName) {
	std::ifstream file(fileName, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open the file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

void deviceDetails(VkSurfaceKHR surface, VkPhysicalDevice& device) {
	debug("Device Properties");

	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	debug(deviceProperties.deviceID);
	debug(deviceProperties.deviceName);
	debug(deviceProperties.deviceType);
	debug(deviceProperties.apiVersion);	debug(deviceProperties.driverVersion);

	/*debug("Device Features");
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);

	debug(features.robustBufferAccess);
	debug(features.fullDrawIndexUint32);
	debug(features.imageCubeArray);
	debug(features.independentBlend);
	debug(features.geometryShader);
	debug(features.tessellationShader);
	debug(features.sampleRateShading);
	debug(features.dualSrcBlend);
	debug(features.logicOp);
	debug(features.multiDrawIndirect);
	debug(features.drawIndirectFirstInstance);
	debug(features.depthClamp);
	debug(features.depthBiasClamp);
	debug(features.fillModeNonSolid);
	debug(features.depthBounds);
	debug(features.wideLines);
	debug(features.largePoints);
	debug(features.alphaToOne);
	debug(features.multiViewport);
	debug(features.samplerAnisotropy);
	debug(features.textureCompressionETC2);
	debug(features.textureCompressionASTC_LDR);
	debug(features.textureCompressionBC);
	debug(features.occlusionQueryPrecise);
	debug(features.pipelineStatisticsQuery);
	debug(features.vertexPipelineStoresAndAtomics);
	debug(features.fragmentStoresAndAtomics);
	debug(features.shaderTessellationAndGeometryPointSize);
	debug(features.shaderImageGatherExtended);
	debug(features.shaderStorageImageExtendedFormats);
	debug(features.shaderStorageImageMultisample);
	debug(features.shaderStorageImageReadWithoutFormat);
	debug(features.shaderStorageImageWriteWithoutFormat);
	debug(features.shaderUniformBufferArrayDynamicIndexing);
	debug(features.shaderSampledImageArrayDynamicIndexing);
	debug(features.shaderStorageBufferArrayDynamicIndexing);
	debug(features.shaderStorageImageArrayDynamicIndexing);
	debug(features.shaderClipDistance);
	debug(features.shaderCullDistance);
	debug(features.shaderFloat64);
	debug(features.shaderInt64);
	debug(features.shaderInt16);
	debug(features.shaderResourceResidency);
	debug(features.shaderResourceMinLod);
	debug(features.sparseBinding);
	debug(features.sparseResidencyBuffer);
	debug(features.sparseResidencyImage2D);
	debug(features.sparseResidencyImage3D);
	debug(features.sparseResidency2Samples);
	debug(features.sparseResidency4Samples);
	debug(features.sparseResidency8Samples);
	debug(features.sparseResidency16Samples);
	debug(features.sparseResidencyAliased);
	debug(features.variableMultisampleRate);
	debug(features.inheritedQueries);*/

	uint32_t queueFamilyCount{ 0 };
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> qfamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, qfamilies.data());

	debug(queueFamilyCount) for (const auto& family : qfamilies) {
		debug(family.queueCount);
		debug(family.queueFlags);
	}

	for (uint32_t i = 0; i < queueFamilyCount; ++i) {
		VkBool32 presentSupport{ false };
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
		if(presentSupport)
			debug(i);
	}
}

bool isDeviceSuitable(VkPhysicalDevice& device, VkSurfaceKHR & surface)
{
	QueueFamilyIndices qfamilyIndices;
	SwapChainSupportDetails swapChainSupportDetails;

	getQueueFamilyIndices(qfamilyIndices, device, surface);
	bool extensionSupported{ checkDeviceExtensionSupport(device) };

	getSwapChainSupportDetails(swapChainSupportDetails, device, surface);
	bool swapChainSupported{ swapChainSupportDetails.formats.size() && swapChainSupportDetails.presentModes.size() };

	return qfamilyIndices.isComplete() && extensionSupported && swapChainSupported;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice& device)
{
	uint32_t extensionCount{ 0 };
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
	
	std::vector<const char*> availableExtensionsNames(extensionCount);
	for (uint32_t i = 0; i < extensionCount; ++i) {
		availableExtensionsNames[i] = availableExtensions[i].extensionName;
	}

	auto cstringComparator = [](const char* a, const char* b) -> bool {
		if (strcmp(a, b) < 0)	return true;
		return false;
	};

	std::sort(availableExtensionsNames.begin(), availableExtensionsNames.end(), cstringComparator);
	
	uint32_t required{ 0 };
	for (; required < deviceExtensions.size(); required++) {
		auto it = std::lower_bound(availableExtensionsNames.begin(), availableExtensionsNames.end(), deviceExtensions[required], cstringComparator);
		if (it == availableExtensionsNames.end() || strcmp(*it, deviceExtensions[required]))
			break;
	}
	//debug(required)
	return required == deviceExtensions.size();
}

void getQueueFamilyIndices(QueueFamilyIndices& result, VkPhysicalDevice& device, VkSurfaceKHR &surface) 
{
	//QueueFamilyIndices indices;
	uint32_t qFamilyCount{ 0 };
	vkGetPhysicalDeviceQueueFamilyProperties(device, &qFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> qFamilies(qFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &qFamilyCount, qFamilies.data());

	for (uint32_t i = 0; i < qFamilyCount; ++i) {
		if (qFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			//indices.graphics.emplace_back(i);
			result.graphics.emplace_back(i);
		if (qFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
			//indices.compute.emplace_back(i);
			result.compute.emplace_back(i);
		if (qFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
			//indices.transfer.emplace_back(i);
			result.transfer.emplace_back(i);
		if (qFamilies[i].queueFlags * VK_QUEUE_SPARSE_BINDING_BIT)
			//indices.sparseBinding.emplace_back(i);
			result.sparseBinding.emplace_back(i);

		VkBool32 presentationSupported{ false };
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupported);
		if (presentationSupported)
			//indices.presentation.emplace_back(i);
			result.presentation.emplace_back(i);
	}
	
	//result = indices;
}

void getSwapChainSupportDetails(SwapChainSupportDetails& result, VkPhysicalDevice& device, VkSurfaceKHR& surface)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	uint32_t formatCount{ 0 };
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	//debug(formatCount)
	if (formatCount) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
	}

	uint32_t presentModesCount{ 0 };
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModesCount, nullptr);
	
	//debug(presentModesCount)
	if (presentModesCount) {
		details.presentModes.resize(presentModesCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModesCount, details.presentModes.data());
	}

	result = details;
}

void getChosenFormat(VkSurfaceFormatKHR& result, const std::vector<VkSurfaceFormatKHR>& formats) 
{
	for (const auto& availableFormat : formats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			result = availableFormat;
			return;
		}
	}
	result = formats[0];
}

void getChosenPresentationMode(VkPresentModeKHR& result , const std::vector<VkPresentModeKHR>& presentModes) 
{
	for (const auto& presentMode : presentModes) {
		if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			result = presentMode;
			return;
		}
	}
	result = VK_PRESENT_MODE_FIFO_KHR;
}

void getChosenExtent(VkExtent2D& result, const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		result = capabilities.currentExtent;
		return;
	}
	VkExtent2D actualExtent = { WIDTH,HEIGHT };
	actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
	actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
	result = actualExtent;
}

VkShaderModule createShaderModule(const VkDevice& device, const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();	
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}
	return shaderModule;
}