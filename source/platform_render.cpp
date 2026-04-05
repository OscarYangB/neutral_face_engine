#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 0

#include "SDL3/SDL_video.h"
#include "SDL3/SDL_init.h"
#include "platform_render.h"
#include "SDL3/SDL_vulkan.h"
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include "vulkan/vulkan_raii.hpp"
#include "definitions.h"

SDL_Window* window{};

constexpr int MAX_FRAMES_IN_FLIGHT = 2;
u32 frame_index = 0;
vk::raii::Context* vulkan_context;
vk::raii::Instance vulkan_instance = nullptr;
vk::raii::PhysicalDevice vulkan_physical_device = nullptr;
vk::raii::Device vulkan_device = nullptr;
vk::PhysicalDeviceFeatures vulkan_device_features;
vk::raii::Queue vulkan_queue = nullptr;
vk::raii::SurfaceKHR vulkan_surface = nullptr;
vk::Extent2D extent;
vk::raii::SwapchainKHR* swapchain;
std::vector<vk::Image> swapchain_images{};
std::vector<vk::raii::ImageView> swapchain_image_views{};
vk::raii::PipelineLayout pipeline_layout = nullptr;
vk::raii::Pipeline vulkan_pipeline = nullptr;
vk::raii::CommandPool command_pool = nullptr;
std::vector<vk::raii::CommandBuffer> command_buffers{};
std::vector<vk::raii::Semaphore> present_semaphores{};
std::vector<vk::raii::Semaphore> render_semaphores{};
std::vector<vk::raii::Fence> draw_fences{};

static_assert(true); // There's a clang bug that gives a warning unless this fucking thing is here
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc23-extensions"
constexpr char shader_data[] = {
	#embed "../shaders/slang.spv"
};
#pragma clang diagnostic pop

bool create_window() {
	SDL_SetAppMetadata("neutral face engine", "0.1", "");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        return false;
    }

    if (window = SDL_CreateWindow("neutral face engine", 1920, 1080, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN); !window) {
        return false;
    }

	auto proc_address = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
    vulkan_context = new vk::raii::Context{proc_address};

	u32 extension_count{};
	auto extensions = SDL_Vulkan_GetInstanceExtensions(&extension_count);
	constexpr vk::ApplicationInfo application_info{.pApplicationName   = "neutral face engine",
												   .applicationVersion = VK_MAKE_VERSION( 1, 0, 0 ),
												   .pEngineName        = "neutral face engine",
												   .engineVersion      = VK_MAKE_VERSION( 1, 0, 0 ),
												   .apiVersion         = vk::ApiVersion14};
	vk::InstanceCreateInfo create_info = {
		.pApplicationInfo = &application_info,
		.enabledExtensionCount = extension_count,
		.ppEnabledExtensionNames = extensions};
	vulkan_instance = {*vulkan_context, create_info};

	VkSurfaceKHR surface_khr;
	SDL_Vulkan_CreateSurface(window, static_cast<vk::Instance>(vulkan_instance), nullptr, &surface_khr);
	vulkan_surface = vk::raii::SurfaceKHR(vulkan_instance, surface_khr);

	auto physical_devices = vulkan_instance.enumeratePhysicalDevices();
	if (physical_devices.empty()) return false;
	std::vector<const char*> required_extensions = {vk::KHRSwapchainExtensionName, vk::KHRShaderDrawParametersExtensionName};
	for (auto& device : physical_devices) {
		bool supports_vulkan_version = device.getProperties().apiVersion >= vk::ApiVersion13;
		bool supports_graphics = false;
		for (auto family : device.getQueueFamilyProperties()) {
			if (family.queueFlags & vk::QueueFlagBits::eGraphics) {
				supports_graphics = true;
				break;
			}
		}
		auto extensions = device.enumerateDeviceExtensionProperties();
		bool supports_required_extensions = std::ranges::all_of(required_extensions, [&extensions](auto const& required_extension) {
			for (auto& extension : extensions) {
				if (std::strcmp(extension.extensionName, required_extension)) {
					return true;
				}
			}
			return false;
		});
		auto features = device.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
		bool supports_required_features = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
			features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;
		if (supports_vulkan_version && supports_graphics && supports_required_extensions && supports_required_features) {
			vulkan_physical_device = device;
			break;
		}
	}

	auto families = vulkan_physical_device.getQueueFamilyProperties();
	u32 family_index{};
	for (int i = 0; i < families.size(); i++) {
		if (families[i].queueFlags & vk::QueueFlagBits::eGraphics && vulkan_physical_device.getSurfaceSupportKHR(i, *vulkan_surface)) {
			family_index = i;
			break;
		}
	}
	float queue_priority = 0.5f;
	vk::DeviceQueueCreateInfo device_queue_info {.queueFamilyIndex = family_index, .queueCount = 1, .pQueuePriorities = &queue_priority};
	vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> feature_chain = {
		{},
		{.dynamicRendering = true },
		{.extendedDynamicState = true }
	};
	vk::DeviceCreateInfo device_info {.pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>(),
									  .queueCreateInfoCount = 1,
									  .pQueueCreateInfos = &device_queue_info,
									  .enabledExtensionCount = static_cast<u32>(required_extensions.size()),
									  .ppEnabledExtensionNames = required_extensions.data()};
	vulkan_device = vk::raii::Device(vulkan_physical_device, device_info);

	vulkan_queue = vk::raii::Queue(vulkan_device, family_index, 0);

	auto surface_capabilities = vulkan_physical_device.getSurfaceCapabilitiesKHR(*vulkan_surface);
	std::vector<vk::SurfaceFormatKHR> available_formats = vulkan_physical_device.getSurfaceFormatsKHR(vulkan_surface);
	assert(!available_formats.empty());
	const auto format = std::ranges::find_if(available_formats, [](const auto& format){
		return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
	});
	int width, height;
	SDL_GetWindowSize(window, &width, &height);
	if (surface_capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
		extent = surface_capabilities.currentExtent;
	} else {
		extent = {std::clamp<uint32_t>(width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width),
				  std::clamp<uint32_t>(height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height)};
	}
	auto image_count = std::max(3u, surface_capabilities.minImageCount);
    if ((0 < surface_capabilities.maxImageCount) && (surface_capabilities.maxImageCount < image_count)) {
		image_count = surface_capabilities.maxImageCount;
	}
	vk::SwapchainCreateInfoKHR swapchain_info {
		.surface = *vulkan_surface,
		.minImageCount = image_count,
		.imageFormat = format->format,
		.imageColorSpace = format->colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
		.imageSharingMode = vk::SharingMode::eExclusive,
		.preTransform = surface_capabilities.currentTransform,
		.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
		.presentMode = vk::PresentModeKHR::eFifo,
		.clipped = true
	};
	swapchain = new vk::raii::SwapchainKHR(vulkan_device, swapchain_info);

	swapchain_images = swapchain->getImages();

	vk::ImageViewCreateInfo image_view_info {
		.viewType = vk::ImageViewType::e2D,
		.format = format->format,
		.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
	};
	for (auto& image : swapchain_images) {
		image_view_info.image = image;
		swapchain_image_views.emplace_back(vulkan_device, image_view_info);
	}

	vk::ShaderModuleCreateInfo shader_info{.codeSize = sizeof(shader_data), .pCode = reinterpret_cast<const uint32_t*>(shader_data)};
	vk::raii::ShaderModule shader_module = {vulkan_device, shader_info};
	vk::PipelineShaderStageCreateInfo vertex_shader_stage_info{.stage = vk::ShaderStageFlagBits::eVertex, .module = shader_module, .pName = "vert_main"};
	vk::PipelineShaderStageCreateInfo fragment_shader_stage_info{.stage = vk::ShaderStageFlagBits::eFragment, .module = shader_module, .pName = "frag_main"};
	vk::PipelineShaderStageCreateInfo shader_stages[] = {vertex_shader_stage_info, fragment_shader_stage_info};
	std::vector<vk::DynamicState> dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	vk::PipelineDynamicStateCreateInfo dynamic_state{.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()), .pDynamicStates = dynamic_states.data()};
	vk::PipelineVertexInputStateCreateInfo vertex_input_info{};
	vk::PipelineInputAssemblyStateCreateInfo input_assembly{.topology = vk::PrimitiveTopology::eTriangleList};
	vk::Viewport viewport{0.f, 0.f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.f, 1.f};
	vk::Rect2D scissor{vk::Offset2D{0,0}, extent};
	vk::PipelineViewportStateCreateInfo viewport_info{.viewportCount = 1, .pViewports = &viewport, .scissorCount = 1, .pScissors=&scissor};
	vk::PipelineRasterizationStateCreateInfo rasterizer{
		.depthClampEnable = vk::False,
		.rasterizerDiscardEnable = vk::False,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eBack,
		.frontFace = vk::FrontFace::eClockwise,
		.depthBiasEnable = vk::False,
		.lineWidth = 1.f};
	vk::PipelineMultisampleStateCreateInfo multisampling_info{.rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False};
	vk::PipelineColorBlendAttachmentState color_blend_attachment{
		.blendEnable = vk::True,
		.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
		.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
		.colorBlendOp        = vk::BlendOp::eAdd,
		.srcAlphaBlendFactor = vk::BlendFactor::eOne,
		.dstAlphaBlendFactor = vk::BlendFactor::eZero,
		.alphaBlendOp        = vk::BlendOp::eAdd,
		.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
	vk::PipelineColorBlendStateCreateInfo color_blending {
		.logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy,
		.attachmentCount = 1, .pAttachments = &color_blend_attachment};
	vk::PipelineLayoutCreateInfo pipeline_info = {.setLayoutCount = 0, .pushConstantRangeCount = 0};
	pipeline_layout = vk::raii::PipelineLayout(vulkan_device, pipeline_info);
	vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipeline_info_chain = {
		{
			.stageCount = 2,
			.pStages = shader_stages,
			.pVertexInputState = &vertex_input_info,
			.pInputAssemblyState = &input_assembly,
			.pViewportState = &viewport_info,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisampling_info,
			.pColorBlendState = &color_blending,
			.pDynamicState = &dynamic_state,
			.layout = pipeline_layout,
			.renderPass = nullptr
		},
		{
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &format->format
		}
	};
	vulkan_pipeline = vk::raii::Pipeline(vulkan_device, nullptr, pipeline_info_chain.get<vk::GraphicsPipelineCreateInfo>());

	vk::CommandPoolCreateInfo pool_info = {.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = family_index};
	command_pool = vk::raii::CommandPool(vulkan_device, pool_info);

	vk::CommandBufferAllocateInfo allocate_info{.commandPool = command_pool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = MAX_FRAMES_IN_FLIGHT};
	command_buffers = vk::raii::CommandBuffers(vulkan_device, allocate_info);

	assert(present_semaphores.empty() && render_semaphores.empty() && draw_fences.empty());
	for (size_t i = 0; i < swapchain_images.size(); i++) {
		render_semaphores.emplace_back(vulkan_device, vk::SemaphoreCreateInfo());
	}

	for (size_t i = 0; i < swapchain_images.size(); i++) {
		present_semaphores.emplace_back(vulkan_device, vk::SemaphoreCreateInfo());
		draw_fences.emplace_back(vulkan_device, vk::FenceCreateInfo{.flags = vk::FenceCreateFlagBits::eSignaled});
	}

	return true;
}

void transition_image_layout(u32 image_index, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::AccessFlags2 src_access_mask,
							 vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask)
{
	vk::ImageMemoryBarrier2 barrier = {
		.srcStageMask        = src_stage_mask,
		.srcAccessMask       = src_access_mask,
		.dstStageMask        = dst_stage_mask,
		.dstAccessMask       = dst_access_mask,
		.oldLayout           = old_layout,
		.newLayout           = new_layout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image               = swapchain_images[image_index],
		.subresourceRange    = {
			.aspectMask     = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel   = 0,
			.levelCount     = 1,
			.baseArrayLayer = 0,
			.layerCount     = 1}};

	vk::DependencyInfo dependency_info = {
		.dependencyFlags         = {},
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers    = &barrier};

    command_buffers[frame_index].pipelineBarrier2(dependency_info);
}

void record_command(u32 index) {
	command_buffers[frame_index].begin({});

	transition_image_layout(index, vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, {}, vk::AccessFlagBits2::eColorAttachmentWrite,
							vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eColorAttachmentOutput);
	vk::RenderingAttachmentInfo attachment_info = {.imageView = swapchain_image_views[index],
												   .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
												   .loadOp = vk::AttachmentLoadOp::eClear,
												   .storeOp = vk::AttachmentStoreOp::eStore,
												   .clearValue = vk::ClearColorValue(0.f, 0.f, 0.f, 1.0f)};
	vk::RenderingInfo rendering_info = {
		.renderArea = {.offset = {0, 0}, .extent = extent},
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &attachment_info};
	command_buffers[frame_index].beginRendering(rendering_info);
	command_buffers[frame_index].bindPipeline(vk::PipelineBindPoint::eGraphics, *vulkan_pipeline);
	command_buffers[frame_index].setViewport(0, vk::Viewport(0.f, 0.f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.f, 1.f));
	command_buffers[frame_index].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));
	command_buffers[frame_index].draw(3, 1, 0, 0);
	command_buffers[frame_index].endRendering();
	transition_image_layout(index, vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits2::eColorAttachmentWrite, {},
							vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe);
	command_buffers[frame_index].end();
}

void draw_frame() {
	auto fence_result = vulkan_device.waitForFences(*draw_fences[frame_index], vk::True, UINT64_MAX);
	vulkan_device.resetFences(*draw_fences[frame_index]);
	auto [result, index] = swapchain->acquireNextImage(UINT64_MAX, *present_semaphores[frame_index], nullptr);
	record_command(index);
	vk::PipelineStageFlags wait_destination_stage_mask{vk::PipelineStageFlagBits::eColorAttachmentOutput};
	vk::SubmitInfo submit_info = {
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*present_semaphores[frame_index],
		.pWaitDstStageMask = &wait_destination_stage_mask,
		.commandBufferCount = 1,
		.pCommandBuffers = &*command_buffers[frame_index],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &*render_semaphores[frame_index]};
	vulkan_queue.submit(submit_info, *draw_fences[frame_index]);
	vk::PresentInfoKHR present_info {
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*render_semaphores[frame_index],
		.swapchainCount = 1,
		.pSwapchains = &**swapchain,
		.pImageIndices = &index};
	result = vulkan_queue.presentKHR(present_info);
	frame_index = (frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

void destroy_window() {
	vulkan_device.waitIdle();
	SDL_DestroyWindow(window);
	SDL_Quit();
}
