#define VULKAN_HPP_ENABLE_DYNAMIC_LOADER_TOOL 0

#include "SDL3/SDL_video.h"
#include "SDL3/SDL_init.h"
#include "platform_render.h"
#include "SDL3/SDL_vulkan.h"
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#include "vulkan/vulkan_raii.hpp"
#include "definitions.h"
#include "vector.h"
#include "matrix.h"
#include "game.h"

struct Vertex {
	Vector3 position{};
	Vector3 color{};

	static vk::VertexInputBindingDescription get_vulkan_binding_description() {
		return {.binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex};
	}

	static std::array<vk::VertexInputAttributeDescription, 2> get_vulkan_attribute_description() {
		return {{{.location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, position)},
				 {.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, color)}}};
	}
};

struct UniformBufferObject {
	Matrix model;
	Matrix view;
	Matrix projection;
};

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
vk::SurfaceFormatKHR format;
vk::Extent2D extent;
vk::raii::SwapchainKHR* swapchain;
std::vector<vk::Image> swapchain_images{};
std::vector<vk::raii::ImageView> swapchain_image_views{};
vk::raii::DescriptorSetLayout descriptor_set_layout = nullptr;
vk::raii::PipelineLayout pipeline_layout = nullptr;
vk::raii::Pipeline vulkan_pipeline = nullptr;
vk::raii::CommandPool command_pool = nullptr;
vk::raii::Buffer vertex_buffer = nullptr;
vk::raii::DeviceMemory vertex_buffer_memory = nullptr;
std::vector<vk::raii::Buffer> uniform_buffers{};
std::vector<vk::raii::DeviceMemory> uniform_buffers_memory{};
std::vector<void*> uniform_buffers_mapped{};
vk::raii::DescriptorPool descriptor_pool = nullptr;
std::vector<vk::raii::DescriptorSet> descriptor_sets{};
std::vector<vk::raii::CommandBuffer> command_buffers{};
std::vector<vk::raii::Semaphore> present_semaphores{};
std::vector<vk::raii::Semaphore> render_semaphores{};
std::vector<vk::raii::Fence> draw_fences{};
bool frame_buffer_resized = false;
vk::raii::Image depth_image = nullptr;
vk::raii::DeviceMemory depth_image_memory = nullptr;
vk::raii::ImageView depth_image_view = nullptr;
vk::Format depth_format = vk::Format::eUndefined;

static_assert(true); // There's a clang bug that gives a warning unless this fucking thing is here
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc23-extensions"
constexpr char shader_data[] = {
	#embed "../shaders/slang.spv"
};

constexpr char VERTEX_DATA[] = {
	#embed "../tools/vertex_data"
};
#pragma clang diagnostic pop

const Vertex* vertices = reinterpret_cast<const Vertex*>(VERTEX_DATA);

SDL_Window* get_window() {
	return window;
}

void create_swapchain() {
	Vertex first = vertices[0];
	Vertex second = vertices[1];

	auto surface_capabilities = vulkan_physical_device.getSurfaceCapabilitiesKHR(*vulkan_surface);
	std::vector<vk::SurfaceFormatKHR> available_formats = vulkan_physical_device.getSurfaceFormatsKHR(vulkan_surface);
	assert(!available_formats.empty());
	format = *std::ranges::find_if(available_formats, [](const auto& format){
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
		.imageFormat = format.format,
		.imageColorSpace = format.colorSpace,
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
		.format = format.format,
		.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 }
	};
	for (auto& image : swapchain_images) {
		image_view_info.image = image;
		swapchain_image_views.emplace_back(vulkan_device, image_view_info);
	}
}

u32 find_memory_type(u32 type_filter, vk::MemoryPropertyFlags required_memory_properties) {
	vk::PhysicalDeviceMemoryProperties memory_properties = vulkan_physical_device.getMemoryProperties();

	for (u32 i = 0; i < memory_properties.memoryTypeCount; i++) {
		if ((type_filter & (1 << i)) &&
			(memory_properties.memoryTypes[i].propertyFlags & required_memory_properties) == required_memory_properties) {
			return i;
		}
	}

	throw std::runtime_error("wtf");
}

void create_buffer(vk::DeviceSize size, vk::BufferUsageFlags usage_flags, vk::MemoryPropertyFlags memory_flags, vk::raii::Buffer& buffer, vk::raii::DeviceMemory& memory) {
	vk::BufferCreateInfo staging_info = {.size = size, .usage = usage_flags, .sharingMode = vk::SharingMode::eExclusive};
	buffer = vk::raii::Buffer(vulkan_device, staging_info);
	vk::MemoryRequirements memory_requirements = buffer.getMemoryRequirements();
	vk::MemoryAllocateInfo allocate_info{.allocationSize = memory_requirements.size, .memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, memory_flags)};
	memory = vk::raii::DeviceMemory(vulkan_device, allocate_info);
	buffer.bindMemory(memory, 0);
}

void copy_buffer(vk::raii::Buffer& source, vk::raii::Buffer& destination, vk::DeviceSize size) {
	vk::CommandBufferAllocateInfo transfer_allocate_info{.commandPool = command_pool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1};
	vk::raii::CommandBuffer transfer_command_buffer = std::move(vulkan_device.allocateCommandBuffers(transfer_allocate_info).front());
	transfer_command_buffer.begin(vk::CommandBufferBeginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
	transfer_command_buffer.copyBuffer(source, destination, vk::BufferCopy(0, 0, size));
	transfer_command_buffer.end();
	vulkan_queue.submit(vk::SubmitInfo{.commandBufferCount = 1, .pCommandBuffers = &*transfer_command_buffer }, nullptr);
	vulkan_queue.waitIdle();
}

void create_depth_image() {
	static constexpr vk::Format candidate_formats[] {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint};
	static constexpr vk::FormatFeatureFlags features = vk::FormatFeatureFlagBits::eDepthStencilAttachment;
	for (const auto current_format : candidate_formats) {
		vk::FormatProperties properties = vulkan_physical_device.getFormatProperties(current_format);
		if ((properties.optimalTilingFeatures & features) == features) {
			depth_format = current_format;
			break;
		}
	}
	assert(depth_format != vk::Format::eUndefined);
	vk::ImageCreateInfo image_info{ .imageType = vk::ImageType::e2D, .format = depth_format,
									.extent = {extent.width, extent.height, 1}, .mipLevels = 1, .arrayLayers = 1,
									.samples = vk::SampleCountFlagBits::e1, .tiling = vk::ImageTiling::eOptimal,
									.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment, .sharingMode = vk::SharingMode::eExclusive };
    depth_image = vk::raii::Image(vulkan_device, image_info);
    vk::MemoryRequirements image_memory_requirements = depth_image.getMemoryRequirements();
    vk::MemoryAllocateInfo image_allocation_info{ .allocationSize = image_memory_requirements.size,
												  .memoryTypeIndex = find_memory_type(image_memory_requirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal) };
    depth_image_memory = vk::raii::DeviceMemory(vulkan_device, image_allocation_info);
    depth_image.bindMemory(depth_image_memory, 0);
	vk::ImageViewCreateInfo image_view_info{ .image = depth_image, .viewType = vk::ImageViewType::e2D,
											 .format = depth_format, .subresourceRange = { vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 } };
    depth_image_view = vk::raii::ImageView(vulkan_device, image_view_info);
}

bool start_render() {
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
			features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState &&
			features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2;
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
		{.synchronization2 = true, .dynamicRendering = true },
		{.extendedDynamicState = true }
	};
	vk::DeviceCreateInfo device_info {.pNext = &feature_chain.get<vk::PhysicalDeviceFeatures2>(),
									  .queueCreateInfoCount = 1,
									  .pQueueCreateInfos = &device_queue_info,
									  .enabledExtensionCount = static_cast<u32>(required_extensions.size()),
									  .ppEnabledExtensionNames = required_extensions.data()};
	vulkan_device = vk::raii::Device(vulkan_physical_device, device_info);

	vulkan_queue = vk::raii::Queue(vulkan_device, family_index, 0);

	create_swapchain();
	create_depth_image();

	vk::DescriptorSetLayoutBinding uniform_layout_binding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr);
	vk::DescriptorSetLayoutCreateInfo layout_info = {.bindingCount = 1, .pBindings = &uniform_layout_binding};
	descriptor_set_layout = vk::raii::DescriptorSetLayout(vulkan_device, layout_info);

	vk::ShaderModuleCreateInfo shader_info{.codeSize = sizeof(shader_data), .pCode = reinterpret_cast<const uint32_t*>(shader_data)};
	vk::raii::ShaderModule shader_module = {vulkan_device, shader_info};
	vk::PipelineShaderStageCreateInfo vertex_shader_stage_info{.stage = vk::ShaderStageFlagBits::eVertex, .module = shader_module, .pName = "vert_main"};
	vk::PipelineShaderStageCreateInfo fragment_shader_stage_info{.stage = vk::ShaderStageFlagBits::eFragment, .module = shader_module, .pName = "frag_main"};
	vk::PipelineShaderStageCreateInfo shader_stages[] = {vertex_shader_stage_info, fragment_shader_stage_info};
	std::vector<vk::DynamicState> dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	vk::PipelineDynamicStateCreateInfo dynamic_state{.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()), .pDynamicStates = dynamic_states.data()};
	auto binding_description = Vertex::get_vulkan_binding_description();
	auto attribute_descriptions = Vertex::get_vulkan_attribute_description();
	vk::PipelineVertexInputStateCreateInfo vertex_input_info{.vertexBindingDescriptionCount = 1,
															 .pVertexBindingDescriptions = &binding_description,
															 .vertexAttributeDescriptionCount = static_cast<u32>(attribute_descriptions.size()),
															 .pVertexAttributeDescriptions = attribute_descriptions.data()};
	vk::PipelineInputAssemblyStateCreateInfo input_assembly{.topology = vk::PrimitiveTopology::ePointList};
	vk::Viewport viewport{0.f, 0.f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.f, 1.f};
	vk::Rect2D scissor{vk::Offset2D{0,0}, extent};
	vk::PipelineViewportStateCreateInfo viewport_info{.viewportCount = 1, .pViewports = &viewport, .scissorCount = 1, .pScissors=&scissor};
	vk::PipelineRasterizationStateCreateInfo rasterizer{
		.depthClampEnable = vk::False,
		.rasterizerDiscardEnable = vk::False,
		.polygonMode = vk::PolygonMode::eFill,
		.cullMode = vk::CullModeFlagBits::eNone, // TEST
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
	vk::PipelineLayoutCreateInfo pipeline_info = {.setLayoutCount = 1, .pSetLayouts = &*descriptor_set_layout, .pushConstantRangeCount = 0};
	pipeline_layout = vk::raii::PipelineLayout(vulkan_device, pipeline_info);
	vk::PipelineDepthStencilStateCreateInfo depth_stencil {
		.depthTestEnable = vk::True,
		.depthWriteEnable = vk::True,
		.depthCompareOp = vk::CompareOp::eLess,
		.depthBoundsTestEnable = vk::False,
		.stencilTestEnable = vk::False
	};
	vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipeline_info_chain = {
		{
			.stageCount = 2,
			.pStages = shader_stages,
			.pVertexInputState = &vertex_input_info,
			.pInputAssemblyState = &input_assembly,
			.pViewportState = &viewport_info,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisampling_info,
			.pDepthStencilState = &depth_stencil,
			.pColorBlendState = &color_blending,
			.pDynamicState = &dynamic_state,
			.layout = pipeline_layout,
			.renderPass = nullptr
		},
		{
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &format.format,
			.depthAttachmentFormat = depth_format
		}
	};
	vulkan_pipeline = vk::raii::Pipeline(vulkan_device, nullptr, pipeline_info_chain.get<vk::GraphicsPipelineCreateInfo>());

	vk::CommandPoolCreateInfo pool_info = {.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = family_index};
	command_pool = vk::raii::CommandPool(vulkan_device, pool_info);

	{
		vk::DeviceSize buffer_size = sizeof(VERTEX_DATA);
		vk::raii::Buffer staging_buffer = nullptr;
		vk::raii::DeviceMemory staging_memory = nullptr;
		create_buffer(buffer_size, vk::BufferUsageFlagBits::eTransferSrc,
					  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, staging_buffer, staging_memory);
		void* staging_data = staging_memory.mapMemory(0, buffer_size);
		memcpy(staging_data, VERTEX_DATA, buffer_size);
		staging_memory.unmapMemory();

		create_buffer(buffer_size, vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
					  vk::MemoryPropertyFlagBits::eDeviceLocal, vertex_buffer, vertex_buffer_memory);
		copy_buffer(staging_buffer, vertex_buffer, buffer_size);
	}
	{
		for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vk::DeviceSize buffer_size = sizeof(UniformBufferObject);
			vk::raii::Buffer buffer = nullptr;
			vk::raii::DeviceMemory memory = nullptr;
			create_buffer(buffer_size, vk::BufferUsageFlagBits::eUniformBuffer,
						  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, buffer, memory);
			uniform_buffers.emplace_back(std::move(buffer));
			uniform_buffers_memory.emplace_back(std::move(memory));
			uniform_buffers_mapped.emplace_back(uniform_buffers_memory[i].mapMemory(0, buffer_size));
		}
	}

	vk::DescriptorPoolSize pool_size(vk::DescriptorType::eUniformBuffer, MAX_FRAMES_IN_FLIGHT);
	vk::DescriptorPoolCreateInfo descriptor_pool_info {.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
													   .maxSets = MAX_FRAMES_IN_FLIGHT, .poolSizeCount = 1, .pPoolSizes = &pool_size};
	descriptor_pool = vk::raii::DescriptorPool(vulkan_device, descriptor_pool_info);
	std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *descriptor_set_layout);
	vk::DescriptorSetAllocateInfo descriptor_allocate_info{.descriptorPool = descriptor_pool,
														   .descriptorSetCount = static_cast<u32>(layouts.size()), .pSetLayouts = layouts.data()};
	descriptor_sets = vulkan_device.allocateDescriptorSets(descriptor_allocate_info);
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		vk::DescriptorBufferInfo descriptor_buffer_info{.buffer = uniform_buffers[i], .offset = 0, .range = sizeof(UniformBufferObject)};
		vk::WriteDescriptorSet descriptor_write{.dstSet = descriptor_sets[i], .dstBinding = 0, .dstArrayElement = 0,
												.descriptorCount = 1, .descriptorType = vk::DescriptorType::eUniformBuffer, .pBufferInfo = &descriptor_buffer_info};
		vulkan_device.updateDescriptorSets(descriptor_write, {});
	}

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

void transition_image_layout(const vk::Image& image, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::AccessFlags2 src_access_mask,
							 vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask,
							 vk::ImageAspectFlags image_aspect_flags)
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
		.image               = image,
		.subresourceRange    = {
			.aspectMask     = image_aspect_flags,
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

	transition_image_layout(swapchain_images[index], vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal, {}, vk::AccessFlagBits2::eColorAttachmentWrite,
							vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::ImageAspectFlagBits::eColor);
	transition_image_layout(*depth_image, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
							vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
							vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
							vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
							vk::ImageAspectFlagBits::eDepth);
	vk::RenderingAttachmentInfo attachment_info = {
		.imageView = swapchain_image_views[index],
		.imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.clearValue = vk::ClearColorValue(0.f, 0.f, 0.f, 1.0f)};
	vk::RenderingAttachmentInfo depth_attachment_info = {
		.imageView = depth_image_view,
		.imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eDontCare,
		.clearValue = vk::ClearDepthStencilValue(1.0f, 0)
	};
	vk::RenderingInfo rendering_info = {
		.renderArea = {.offset = {0, 0}, .extent = extent},
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &attachment_info,
		.pDepthAttachment = &depth_attachment_info};
	command_buffers[frame_index].beginRendering(rendering_info);
	command_buffers[frame_index].bindPipeline(vk::PipelineBindPoint::eGraphics, *vulkan_pipeline);
	command_buffers[frame_index].bindVertexBuffers(0, *vertex_buffer, {0});
	command_buffers[frame_index].setViewport(0, vk::Viewport(0.f, 0.f, static_cast<float>(extent.width), static_cast<float>(extent.height), 0.f, 1.f));
	command_buffers[frame_index].setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), extent));
	command_buffers[frame_index].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline_layout, 0, *descriptor_sets[frame_index], nullptr);
	command_buffers[frame_index].draw(sizeof(VERTEX_DATA) / sizeof(Vertex), 1, 0, 0);
	command_buffers[frame_index].endRendering();
	transition_image_layout(swapchain_images[index], vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits2::eColorAttachmentWrite, {},
							vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::PipelineStageFlagBits2::eBottomOfPipe, vk::ImageAspectFlagBits::eColor);
	command_buffers[frame_index].end();
}

void reset_swapchain() {
	swapchain_image_views.clear();
	delete swapchain;
	swapchain = nullptr;

	while ((SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0) {
		SDL_Event event;
		SDL_WaitEvent(&event);
		if (event.type == SDL_EVENT_WINDOW_RESTORED) {
			break;
		}
	}

	vulkan_device.waitIdle();
	create_swapchain();
	create_depth_image();
}

void update_uniform_buffer(u32 index) {
	UniformBufferObject uniform_buffer{};
	uniform_buffer.model = Matrix::identity();
	uniform_buffer.view = Matrix::look_at(camera_direction.normalized() + camera_position, camera_position);
	uniform_buffer.projection = Matrix::perspective(M_PI / 4.f, static_cast<float>(extent.width) / static_cast<float>(extent.height), 0.1f, 10.f);
	memcpy(uniform_buffers_mapped[index], &uniform_buffer, sizeof(uniform_buffer));
}

void draw_frame() {
	auto fence_result = vulkan_device.waitForFences(*draw_fences[frame_index], vk::True, UINT64_MAX);
	assert(fence_result == vk::Result::eSuccess);
	auto [result, image_index] = swapchain->acquireNextImage(UINT64_MAX, *present_semaphores[frame_index], nullptr);
	if (result == vk::Result::eErrorOutOfDateKHR) {
		reset_swapchain();
		return;
	}
	assert(result == vk::Result::eSuccess || result == vk::Result::eSuboptimalKHR);
	update_uniform_buffer(frame_index);
	vulkan_device.resetFences(*draw_fences[frame_index]);
	record_command(image_index);

	vk::PipelineStageFlags wait_destination_stage_mask{vk::PipelineStageFlagBits::eColorAttachmentOutput};
	vk::SubmitInfo submit_info = {
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*present_semaphores[frame_index],
		.pWaitDstStageMask = &wait_destination_stage_mask,
		.commandBufferCount = 1,
		.pCommandBuffers = &*command_buffers[frame_index],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &*render_semaphores[image_index]};
	vulkan_queue.submit(submit_info, *draw_fences[frame_index]);

	vk::PresentInfoKHR present_info {
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &*render_semaphores[image_index],
		.swapchainCount = 1,
		.pSwapchains = &**swapchain,
		.pImageIndices = &image_index};
	result = vulkan_queue.presentKHR(present_info);
	if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR || frame_buffer_resized) {
		frame_buffer_resized = false;
		reset_swapchain();
	} else {
		assert(result == vk::Result::eSuccess);
	}

	frame_index = (frame_index + 1) % MAX_FRAMES_IN_FLIGHT;
}

void on_window_resized() {
	frame_buffer_resized = true;
}

void end_render() {
	vulkan_device.waitIdle();
	SDL_DestroyWindow(window);
	SDL_Quit();
}
