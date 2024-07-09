
#include <iostream>
#include <vector>
#include <array>
#include <cstdint>
#include <optional>
#include <set>
#include <bitset>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <memory>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <shellscalingapi.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_vulkan.h>


class VkGltfModel final
{

public:
	struct Primitive
	{
		uint32_t FirstIndex;
		uint32_t IndexCount;
	};

	struct Mesh
	{
		std::vector<Primitive> Primitives;
	};

	struct Node
	{
		std::shared_ptr<Node>              Parent;
		std::uint32_t                      Index;
		std::vector<std::shared_ptr<Node>> Children;
		Mesh                               Mesh;
		DirectX::XMFLOAT3                  Translation{};
		DirectX::XMFLOAT3                  Scale{1.0f, 1.0f, 1.0f};
		DirectX::XMFLOAT4                  Rotation{};
		std::int32_t                       Skin{-1};
		DirectX::XMFLOAT4X4                Matrix;

		DirectX::XMMATRIX                GetLocalMatrix() const
		{
			const DirectX::XMMATRIX XmMatrix            = DirectX::XMLoadFloat4x4(&Matrix);
			const DirectX::XMMATRIX XmMatrixScale       = DirectX::XMMatrixScalingFromVector(DirectX::XMLoadFloat3(&Scale));
			const DirectX::XMMATRIX XmMatrixRotation    = DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&Rotation));
			const DirectX::XMMATRIX XmMatrixTranslation = DirectX::XMMatrixTranslationFromVector(DirectX::XMLoadFloat3(&Translation));

			const DirectX::XMMATRIX XmLocalMatrix = DirectX::XMMatrixMultiply(XmMatrix, DirectX::XMMatrixMultiply(XmMatrixScale, DirectX::XMMatrixMultiply(XmMatrixRotation, XmMatrixTranslation)));

			return XmLocalMatrix;
		}
	};

	struct Vertex
	{
		DirectX::XMFLOAT3 Pos;
		DirectX::XMFLOAT3 Normal;
		DirectX::XMFLOAT2 Uv;
		DirectX::XMUINT4  JointIndices0;
		DirectX::XMFLOAT4 JointWeights0;
		DirectX::XMUINT4  JointIndices1;
		DirectX::XMFLOAT4 JointWeights1;
	};

	struct Skin
	{
		std::string                              Name;
		std::shared_ptr<Node>                    SkeletonRoot{nullptr};
		std::vector<DirectX::XMFLOAT4X4>         InverseBindMatrices;
		std::vector<std::shared_ptr<Node>>       Joints;
		std::tuple<vk::Buffer, vk::DeviceMemory> Ssbo;
		vk::DescriptorSet                        DescriptorSet;
	};


	struct AnimationSampler
	{
		std::string                    Interpolation;
		std::vector<float>             Inputs;
		std::vector<DirectX::XMFLOAT4> OutputsVec4;
	};

	struct AnimationChannel
	{
		std::string           Path;
		std::shared_ptr<Node> Node;
		std::uint32_t         SamplerIndex;
	};

	struct Animation
	{
		std::string                   Name;
		std::vector<AnimationSampler> Samplers;
		std::vector<AnimationChannel> Channels;
		float                         Start{std::numeric_limits<float>::max()};
		float                         End{std::numeric_limits<float>::min()};
		float                         CurrentTime{0.0f};
	};



public:
	VkGltfModel();
	~VkGltfModel();

	bool LoadFromFile(std::string FileName);
	void LoadNode(const tinygltf::Node& InputNode, std::shared_ptr<VkGltfModel::Node> NodeParent, std::uint32_t NodeIndex, std::vector<std::uint32_t>& HostIndexBuffer, std::vector<VkGltfModel::Vertex>& HostVertexBuffer);
	void LoadSkins();
	void LoadAnimations();

	std::shared_ptr<VkGltfModel::Node> FindNode(std::shared_ptr<Node> Parent, std::uint32_t Index) const;
	std::shared_ptr<VkGltfModel::Node> NodeFromIndex(std::uint32_t Index) const;
	DirectX::XMMATRIX GetNodeMatrix(std::shared_ptr<VkGltfModel::Node> Node);

	void UpdateJoints(std::shared_ptr<VkGltfModel::Node> Node);
	void UpdateAnimation(float DeltaTime);

	void Shutdown();

public:
	tinygltf::Model M_Model;
	std::vector<std::shared_ptr<Node>> M_Nodes;
	std::vector<std::shared_ptr<Node>> M_LinearNodes;
	std::vector<Skin>      M_Skins;
	std::vector<Animation> M_Animations;

	std::tuple<vk::Buffer, vk::DeviceMemory> M_VertexBufferTuple;
	std::tuple<vk::Buffer, vk::DeviceMemory> M_IndexBufferTuple;

	vk::DescriptorPool M_SkinsDescriptorPool = {};
};


void InitWindow();
void ShutdownWindow();

bool Render(bool bClearOnly = false);
void ImGuiRender(vk::CommandBuffer CommandBuffer);

std::uint32_t FindMemoryTypeIndex(std::uint32_t typeFilter, vk::MemoryPropertyFlags Properties);
vk::ShaderModule CreateShader(const std::string &fileName);
std::tuple<vk::Buffer, vk::DeviceMemory> CreateBuffer(vk::BufferUsageFlags UsageFlags, vk::DeviceSize ByteSize, void* DataPtr, bool bDeviceLocal = false);
vk::CommandBuffer BeginSingleUseCommandBuffer();
void EndSingleUseCommandBuffer(vk::CommandBuffer);

void InitVulkan();
void ShutdownVulkan();

void InitPhysicalDevice();
void InitQueueFamilies();
void InitDevice();
void InitQueues();

void InitCommandPools();
void InitCommandBuffers();
void InitSyncObjects();

void InitSurface();

void InitRenderPass();

void InitSwapchain();
void CreateSwapchain();
void DestroySwapchain();
void RecreateSwapchain();
void ShutdownSwapchain();

void InitImGui();
void ShutdownImGui();

///////////////////////////////////////////////////////////////////////////
void InitPipeline();
void ShutdownPipeline();

void InitModel();
void ShutdownModel();

LRESULT CALLBACK WndProc(HWND Hwnd, UINT Msg, WPARAM Wparam, LPARAM Lparam);





HINSTANCE G_Hinstance = {};
HWND G_Hwnd = {};

constexpr std::uint32_t G_PreferredImageCount = 2;
constexpr std::uint32_t G_MaxFramesInFlight = 2;
std::uint32_t G_CurrentFrame = 0;

vk::DispatchLoaderDynamic G_DLD = {};

vk::Instance G_VkInstance = {};
vk::PhysicalDevice G_PhysicalDevice = {};
vk::SampleCountFlagBits G_SampleCount = vk::SampleCountFlagBits::e1;

std::optional<std::uint32_t> G_GraphicsQueueFamilyIndex;
std::optional<std::uint32_t> G_PresentQueueFamilyIndex;
std::optional<std::uint32_t> G_ComputeQueueFamilyIndex;

vk::Device G_Device = {};
vk::Queue G_GraphicsQueue;
vk::Queue G_PresentQueue;
vk::Queue G_ComputeQueue;

vk::CommandPool G_DynamicCommandPool = {};
vk::CommandPool G_StaticCommandPool = {};

std::array<vk::CommandBuffer, G_MaxFramesInFlight> G_CommandBuffers = {};

std::array<bool, G_MaxFramesInFlight> G_WaitForFences = {};
std::array<vk::Fence, G_MaxFramesInFlight> G_InFlightFences = {};
std::array<vk::Semaphore, G_MaxFramesInFlight> G_ImageAvailableSemaphores = {};
std::array<vk::Semaphore, G_MaxFramesInFlight> G_RenderFinishedSemaphores = {};

vk::SurfaceKHR G_Surface = {};
vk::SurfaceFormatKHR G_SurfaceFormat = {};
vk::PresentModeKHR G_SurfacePresentMode = {};
std::uint32_t G_SurfaceImageCount = {};
vk::Format G_DepthFormat = {};

vk::RenderPass G_RenderPass = {};

bool G_SwapchainOK = false;
vk::Extent2D G_WindowSize = {};
vk::Extent2D G_SwapchainExtent = {};

vk::SwapchainKHR G_Swapchain = {};

std::vector<vk::Image> G_ColorBufferImages;
std::vector<vk::DeviceMemory> G_ColorBufferDeviceMemories;
std::vector<vk::ImageView> G_ColorBufferImageViews;

std::vector<vk::Image> G_DepthBufferImages;
std::vector<vk::DeviceMemory> G_DepthBufferDeviceMemories;
std::vector<vk::ImageView> G_DepthBufferImageViews;

std::vector<vk::Image> G_SwapchainImages = {};
std::vector<vk::ImageView> G_SwapchainImageViews = {};

std::vector<vk::Framebuffer> G_Framebuffers = {};

vk::DescriptorPool G_ImguiDescriptorPool = {};
ImGuiContext *G_ImGuiContext = {};
ImFont *G_ConsolasFont = {};


////////////////////////////////////////////////////
vk::DescriptorSetLayout G_SkinsDescriptorSetLayout = {};
vk::PipelineLayout G_PipelineLayout = {};
vk::Pipeline G_Pipeline = {};

VkGltfModel G_GltfModel;


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int nCmdShow)
{
	::SetProcessDpiAwareness(PROCESS_DPI_UNAWARE);

	G_Hinstance = hInstance;

	InitWindow();
	InitVulkan();

	ImGui::SetCurrentContext(G_ImGuiContext);
	Render(true);
	ImGui::SetCurrentContext(nullptr);

	MSG Msg = {};
	bool bContinue = true;

	::ShowWindow(G_Hwnd, SW_SHOW);
	::UpdateWindow(G_Hwnd);

	while(bContinue) {
		for (std::uint32_t i = 0; i < std::uint32_t(1); i++) {
			if (::PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE)) {
				if (Msg.message == WM_QUIT) {
					bContinue = false;
					break;
				}

				if (Msg.hwnd == G_Hwnd) ImGui::SetCurrentContext(G_ImGuiContext);
				::TranslateMessage(&Msg);
				::DispatchMessage(&Msg);
				ImGui::SetCurrentContext(nullptr);

			} else {
				break;
			}
		}

		ImGui::SetCurrentContext(G_ImGuiContext);
		if (!::IsIconic(G_Hwnd)) {
			if (!Render()) {
				RecreateSwapchain();
				Render();
			}
		}
		ImGui::SetCurrentContext(nullptr);
	}

	ShutdownVulkan();
	ShutdownWindow();

	return 0;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND Hwnd, UINT Msg, WPARAM Wparam, LPARAM Lparam)
{

	const auto InternalWndProc = [](HWND Hwnd, UINT Msg, WPARAM Wparam, LPARAM Lparam) -> LRESULT {
		switch(Msg)
		{
		case WM_SIZE:
		{
			G_SwapchainOK = false;
		}
		break;
		case WM_ERASEBKGND:
		{
			return 1;
		}
		break;
		case WM_DESTROY:
		{
			::PostQuitMessage(0);
		}
		break;
		}

		return ::DefWindowProcA(Hwnd, Msg, Wparam, Lparam);
	};


	const auto Result = InternalWndProc(Hwnd, Msg, Wparam, Lparam);
	ImGui_ImplWin32_WndProcHandler(Hwnd, Msg, Wparam, Lparam);

	return Result;
}



void InitWindow()
{

	static constexpr DWORD dwStyle = WS_CAPTION | WS_BORDER | WS_MINIMIZEBOX | WS_SYSMENU | WS_SIZEBOX | WS_MAXIMIZEBOX;
	RECT rc = RECT{0, 0, 480, 640};
	::AdjustWindowRectEx(&rc, dwStyle, FALSE, 0);

	const WNDCLASSEXA wcx = WNDCLASSEXA{
		sizeof(WNDCLASSEXA),
		0,
		WndProc,
		0,
		0,
		G_Hinstance,
		LoadIconA(nullptr, IDI_APPLICATION),
		LoadCursorA(nullptr, IDC_ARROW),
		0,
		0,
		"VkSkelAnimTestClass",
		LoadIconA(nullptr, IDI_APPLICATION)
	};
	::RegisterClassExA(&wcx);


	G_Hwnd = ::CreateWindowExA(
		0,
		"VkSkelAnimTestClass",
		"Vk Skeletal Animation Test",
		dwStyle,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		G_Hinstance,
		nullptr
	);
}

void ShutdownWindow()
{
	::UnregisterClassA(
		"VkSkelAnimTestClass",
		G_Hinstance
	);
}


bool Render(bool bClearOnly)
{
	if (!G_SwapchainOK) return false;

	if (G_WaitForFences[G_CurrentFrame]) {
		(void)G_Device.waitForFences(1, &G_InFlightFences[G_CurrentFrame], VK_TRUE, std::numeric_limits<std::uint64_t>::max(), G_DLD);
		G_WaitForFences[G_CurrentFrame] = false;
	} else {
		G_GraphicsQueue.waitIdle(G_DLD);
	}
	(void)G_Device.resetFences(1, &G_InFlightFences[G_CurrentFrame], G_DLD);

	std::uint32_t ImageIndex = 0;
	try {
		vk::ResultValue Acquire = G_Device.acquireNextImageKHR(
			G_Swapchain, std::numeric_limits<std::uint64_t>::max(),
			G_ImageAvailableSemaphores[G_CurrentFrame], nullptr,
			G_DLD
			);

		ImageIndex = Acquire.value;
	}
	catch (...) {
		G_SwapchainOK = false;
		return false;
	}

	vk::CommandBuffer CommandBuffer = G_CommandBuffers[G_CurrentFrame];
	CommandBuffer.reset({}, G_DLD);

	const vk::CommandBufferBeginInfo commandBufferBeginInfo = {};
	CommandBuffer.begin(commandBufferBeginInfo, G_DLD);


	vk::ClearColorValue ClearColor;
	std::memcpy(&ClearColor, DirectX::Colors::Black.f, sizeof(ClearColor));
	static constexpr vk::ClearColorValue ClearResolve(0.0f, 0.0f, 0.0f, 1.0f);
	static constexpr vk::ClearDepthStencilValue ClearDepth(1.0f, 0);

	const vk::ClearValue ClearValues[2] = {ClearColor, ClearDepth};
	const vk::ClearValue MultiSamplesClearValues[3] = {ClearColor, ClearResolve, ClearDepth};
	const vk::RenderPassBeginInfo RenderPassBeginInfo = vk::RenderPassBeginInfo(G_RenderPass, G_Framebuffers[ImageIndex], vk::Rect2D(vk::Offset2D(0,0), G_SwapchainExtent), (G_SampleCount == vk::SampleCountFlagBits::e1) ? 2 : 3, (G_SampleCount == vk::SampleCountFlagBits::e1) ? ClearValues : MultiSamplesClearValues);

	CommandBuffer.beginRenderPass(&RenderPassBeginInfo, vk::SubpassContents::eInline, G_DLD);

	if (!bClearOnly) {
		CommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, G_Pipeline, G_DLD);

		vk::DeviceSize VertexBufferOffset = 0;
		CommandBuffer.bindVertexBuffers(0, 1, &std::get<0>(G_GltfModel.M_VertexBufferTuple), &VertexBufferOffset, G_DLD);
		CommandBuffer.bindIndexBuffer(std::get<0>(G_GltfModel.M_IndexBufferTuple), 0, vk::IndexType::eUint32, G_DLD);

		CommandBuffer.setViewport(0, vk::Viewport{0.0f, 0.0f, float(G_SwapchainExtent.width), float(G_SwapchainExtent.height), 0.0f, 1.0f}, G_DLD);
		CommandBuffer.setScissor(0, vk::Rect2D{vk::Offset2D{0, 0}, vk::Extent2D{G_SwapchainExtent.width, G_SwapchainExtent.height}}, G_DLD);

		const DirectX::XMMATRIX MatProj = DirectX::XMMatrixPerspectiveFovRH(-DirectX::XMConvertToRadians(35), float(G_SwapchainExtent.width) / float(G_SwapchainExtent.height), 0.1f, 10000);
		const DirectX::XMMATRIX MatView = DirectX::XMMatrixLookAtRH(DirectX::XMVectorSet(-100.0f, 150.0f, 400.0f, 0.0f), DirectX::XMVectorSet(0.0f, 80.0f, 0.0f, 0.0f), DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
		const DirectX::XMMATRIX MatProjView = DirectX::XMMatrixMultiply(MatView, MatProj);
		DirectX::XMFLOAT4X4 MatProjViewDest;
		DirectX::XMStoreFloat4x4(&MatProjViewDest, MatProjView);

		static auto PrevTime = std::chrono::high_resolution_clock::now() - std::chrono::milliseconds(1);
		auto CurrentTime = std::chrono::high_resolution_clock::now();
		const float DeltaTime = float(std::chrono::duration_cast<std::chrono::milliseconds>(CurrentTime-PrevTime).count()) / 1000.0f;
		PrevTime = CurrentTime;


		G_GltfModel.UpdateAnimation(DeltaTime);

		for (auto &Node : G_GltfModel.M_LinearNodes) {

			if (Node->Skin < 0) continue;

			const DirectX::XMMATRIX MatModel = DirectX::XMMatrixIdentity();
			DirectX::XMFLOAT4X4 MatModelDest;
			DirectX::XMStoreFloat4x4(&MatModelDest, MatModel);

			CommandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, G_PipelineLayout, 0, G_GltfModel.M_Skins[Node->Skin].DescriptorSet, nullptr, G_DLD);

			for (auto& Primitive : Node->Mesh.Primitives) {


				CommandBuffer.pushConstants(G_PipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(DirectX::XMFLOAT4X4), &MatProjViewDest, G_DLD);
				CommandBuffer.pushConstants(G_PipelineLayout, vk::ShaderStageFlagBits::eVertex, sizeof(DirectX::XMFLOAT4X4), sizeof(DirectX::XMFLOAT4X4), &MatModelDest, G_DLD);
				CommandBuffer.pushConstants(G_PipelineLayout, vk::ShaderStageFlagBits::eFragment, sizeof(DirectX::XMFLOAT4X4) + sizeof(DirectX::XMFLOAT4X4), sizeof(DirectX::XMFLOAT3), DirectX::Colors::SpringGreen.f, G_DLD);

				CommandBuffer.drawIndexed(Primitive.IndexCount, 1, Primitive.FirstIndex, 0, 0, G_DLD);
			}
		}

		ImGuiRender(CommandBuffer);
	}

	CommandBuffer.endRenderPass(G_DLD);

	CommandBuffer.end(G_DLD);

	const vk::Semaphore waitSemaphores[] = { G_ImageAvailableSemaphores[G_CurrentFrame]};
	const vk::Semaphore signalSemaphores[] = { G_RenderFinishedSemaphores[G_CurrentFrame]};
	static constexpr vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	const vk::SubmitInfo submitInfo = vk::SubmitInfo(1, waitSemaphores, waitStages, 1, &CommandBuffer, 1, signalSemaphores);
	try {
		G_WaitForFences[G_CurrentFrame] = true;
		G_GraphicsQueue.submit(submitInfo, G_InFlightFences[G_CurrentFrame], G_DLD);
	}
	catch (...) {
		G_WaitForFences[G_CurrentFrame] = false;
		G_SwapchainOK = false;
		return false;
	}

	const vk::SwapchainKHR Swapchains[1] = {G_Swapchain};
	const vk::PresentInfoKHR presentInfo = vk::PresentInfoKHR(1, signalSemaphores, 1, Swapchains, &ImageIndex);
	try {
		(void)G_PresentQueue.presentKHR(presentInfo, G_DLD);
	}
	catch (...) {
		G_SwapchainOK = false;
		return false;
	}

	G_CurrentFrame = (G_CurrentFrame + 1) % G_MaxFramesInFlight;

	return true;
}

void ImGuiRender(vk::CommandBuffer CommandBuffer)
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

//	ImGui::Begin("Control", nullptr);
//	ImGui::PushFont(G_ConsolasFont);
//	ImGui::PopFont();
//	ImGui::End();

	static bool bFirst = true;
	if (bFirst) {
		ImGui::SetWindowFocus(nullptr);
		bFirst = false;
	}

	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), CommandBuffer);
}


std::uint32_t FindMemoryTypeIndex(std::uint32_t typeFilter, vk::MemoryPropertyFlags Properties)
{
	vk::PhysicalDeviceMemoryProperties MemProperties = G_PhysicalDevice.getMemoryProperties(G_DLD);

	for (std::uint32_t i = 0; i < MemProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && MemProperties.memoryTypes[i].propertyFlags == Properties) {
			return i;
		}
	}

	for (std::uint32_t i = 0; i < MemProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (MemProperties.memoryTypes[i].propertyFlags & Properties) == Properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

vk::ShaderModule CreateShader(const std::string &fileName)
{
	const std::string FilePath = std::string(APP_SOURCE_PATH) + std::string("/shaders/") + fileName;
	std::ifstream Ifs = std::ifstream(FilePath, std::ios::binary | std::ios::in);

	if (!Ifs.is_open()) {
		throw std::runtime_error("Could not open shader file");
	}

	const std::vector<char> Buf = std::vector<char>(std::istreambuf_iterator<char>(Ifs), std::istreambuf_iterator<char>());
	Ifs.close();

	const vk::ShaderModuleCreateInfo ShaderModuleCI = vk::ShaderModuleCreateInfo(
		{},
		Buf.size(),
		reinterpret_cast<const std::uint32_t*>(Buf.data())
		);

	return G_Device.createShaderModule(ShaderModuleCI, nullptr, G_DLD);
}

std::tuple<vk::Buffer, vk::DeviceMemory> CreateBuffer(vk::BufferUsageFlags UsageFlags, vk::DeviceSize ByteSize, void* DataPtr, bool bDeviceLocal)
{
	const vk::BufferCreateInfo BufferCI = vk::BufferCreateInfo(
		{},
		ByteSize,
		(bDeviceLocal ? (UsageFlags | vk::BufferUsageFlagBits::eTransferSrc) : UsageFlags),
		vk::SharingMode::eExclusive,
		0,
		nullptr
		);

	const vk::Buffer Buffer = G_Device.createBuffer(BufferCI, nullptr, G_DLD);

	const vk::MemoryRequirements MemReqs = G_Device.getBufferMemoryRequirements(Buffer, G_DLD);
	const std::uint32_t MemTypeIndex = FindMemoryTypeIndex(MemReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
	const vk::MemoryAllocateInfo MemAI = vk::MemoryAllocateInfo(MemReqs.size, MemTypeIndex);

	vk::DeviceMemory BufferMemory = G_Device.allocateMemory(MemAI, nullptr, G_DLD);
	G_Device.bindBufferMemory(Buffer, BufferMemory, 0, G_DLD);

	void* Mapped =G_Device.mapMemory(BufferMemory, 0, vk::WholeSize, vk::MemoryMapFlags(), G_DLD);
	if (DataPtr) {
		std::memcpy(Mapped, DataPtr, ByteSize);
	} else {
		std::memset(Mapped, 0, ByteSize);
	}
	G_Device.unmapMemory(BufferMemory, G_DLD);

	if (bDeviceLocal) {
		const vk::BufferCreateInfo LocalBufferCI = vk::BufferCreateInfo(
			{},
			ByteSize,
			UsageFlags | vk::BufferUsageFlagBits::eTransferDst,
			vk::SharingMode::eExclusive,
			0,
			nullptr
			);

		const vk::Buffer LocalBuffer = G_Device.createBuffer(LocalBufferCI, nullptr, G_DLD);
		const vk::MemoryRequirements LocalMemReqs = G_Device.getBufferMemoryRequirements(LocalBuffer, G_DLD);
		const std::uint32_t LocalMemTypeIndex = FindMemoryTypeIndex(LocalMemReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
		const vk::MemoryAllocateInfo LocalMemAI = vk::MemoryAllocateInfo(LocalMemReqs.size, LocalMemTypeIndex);

		const vk::DeviceMemory LocalBufferMemory = G_Device.allocateMemory(LocalMemAI, nullptr, G_DLD);
		G_Device.bindBufferMemory(LocalBuffer, LocalBufferMemory, 0, G_DLD);

		const vk::CommandBuffer CommandBuffer = BeginSingleUseCommandBuffer();
		const vk::BufferCopy BufferCopy = vk::BufferCopy(0, 0, ByteSize);
		CommandBuffer.copyBuffer(Buffer, LocalBuffer, BufferCopy, G_DLD);
		EndSingleUseCommandBuffer(CommandBuffer);

		G_Device.freeMemory(BufferMemory, nullptr, G_DLD);
		G_Device.destroyBuffer(Buffer, nullptr, G_DLD);

		return std::make_tuple(LocalBuffer, LocalBufferMemory);
	}

	return std::make_tuple(Buffer, BufferMemory);
}

vk::CommandBuffer BeginSingleUseCommandBuffer()
{
	const vk::CommandBufferAllocateInfo CommandBufferAI = vk::CommandBufferAllocateInfo(G_StaticCommandPool, vk::CommandBufferLevel::ePrimary, 1);
	vk::CommandBuffer CommandBuffer = G_Device.allocateCommandBuffers(CommandBufferAI, G_DLD)[0];
	const vk::CommandBufferBeginInfo CommandBufferBI = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	CommandBuffer.begin(CommandBufferBI, G_DLD);

	return CommandBuffer;
}

void EndSingleUseCommandBuffer(vk::CommandBuffer CommandBuffer)
{
	CommandBuffer.end(G_DLD);
	const vk::SubmitInfo SI = vk::SubmitInfo(0, nullptr, 0, 1, &CommandBuffer, 0, nullptr);
	G_GraphicsQueue.submit(SI, nullptr, G_DLD);
	G_GraphicsQueue.waitIdle(G_DLD);
}

void InitVulkan()
{
	G_DLD.init();

	const std::uint32_t VkApiVersion = vk::enumerateInstanceVersion(G_DLD) & (~std::uint32_t(0xFFFU));
	const vk::ApplicationInfo AppInfo = vk::ApplicationInfo(
		"VkSkeletalAnimationExample",
		VK_MAKE_API_VERSION(0, 1, 0, 0),
		"VkSkeletalAnimationExampleEngine",
		VK_MAKE_API_VERSION(0, 1, 0, 0),
		VkApiVersion
		);

	static constexpr std::array<const char*, 1> RequiredInstanceLayers = {
		"VK_LAYER_KHRONOS_validation",
	};
	static constexpr std::array<const char*, 3> RequiredInstanceExtensions = {
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	};
	const std::vector<vk::LayerProperties> SupportedInstanceLayers = vk::enumerateInstanceLayerProperties(G_DLD);
	const std::vector<vk::ExtensionProperties> SupportedInstanceExtensions = vk::enumerateInstanceExtensionProperties(nullptr, G_DLD);

	for (const char* RequiredLayer : RequiredInstanceLayers) {
		if (std::find_if(SupportedInstanceLayers.begin(), SupportedInstanceLayers.end(), [&RequiredLayer](const vk::LayerProperties& LayerProperties) {return std::strcmp(LayerProperties.layerName.data(), RequiredLayer) == 0; }) == SupportedInstanceLayers.end())
			throw std::runtime_error("Vulkan doesn't support required layers");
	}
	for (const char* RequiredExtension : RequiredInstanceExtensions) {
		if (std::find_if(SupportedInstanceExtensions.begin(), SupportedInstanceExtensions.end(), [&RequiredExtension](const vk::ExtensionProperties& ExtensionProperties) {return std::strcmp(ExtensionProperties.extensionName.data(), RequiredExtension) == 0; }) == SupportedInstanceExtensions.end())
			throw std::runtime_error("Vulkan doesn't support required extensions");
	}

	const vk::InstanceCreateInfo VkInstanceCI = vk::InstanceCreateInfo(
		{},
		&AppInfo,
		static_cast<std::uint32_t>(RequiredInstanceLayers.size()), RequiredInstanceLayers.data(),
		static_cast<std::uint32_t>(RequiredInstanceExtensions.size()), RequiredInstanceExtensions.data()
		);
	G_VkInstance = vk::createInstance(VkInstanceCI, nullptr, G_DLD);
	G_DLD.init(G_VkInstance);


	InitPhysicalDevice();
	InitQueueFamilies();
	InitDevice();
	InitQueues();
	InitCommandPools();
	InitCommandBuffers();
	InitSyncObjects();
	InitSurface();
	InitRenderPass();

	InitSwapchain();
	InitImGui();

	InitPipeline();
	InitModel();
}

void ShutdownVulkan()
{
	if (G_Device) {
		G_Device.waitIdle(G_DLD);


		ShutdownModel();
		ShutdownPipeline();

		ShutdownImGui();
		ShutdownSwapchain();

		if (G_RenderPass) {
			G_Device.destroyRenderPass(G_RenderPass, nullptr, G_DLD);
			G_RenderPass = nullptr;
		}

		if (G_Surface) {
			G_VkInstance.destroySurfaceKHR(G_Surface, nullptr, G_DLD);
			G_Surface = nullptr;
		}

		for (auto& Item : G_InFlightFences) {
			if (Item) {
				G_Device.destroyFence(Item, nullptr, G_DLD);
				Item = nullptr;
			}
		}

		for (auto& Item : G_RenderFinishedSemaphores) {
			if (Item) {
				G_Device.destroySemaphore(Item, nullptr, G_DLD);
				Item = nullptr;
			}
		}

		for (auto& Item : G_ImageAvailableSemaphores) {
			if (Item) {
				G_Device.destroySemaphore(Item, nullptr, G_DLD);
				Item = nullptr;
			}
		}

		for (auto& Item : G_CommandBuffers) {
			if (Item) {
				G_Device.freeCommandBuffers(G_DynamicCommandPool, Item, G_DLD);
				Item = nullptr;
			}
		}

		if (G_DynamicCommandPool) {
			G_Device.destroyCommandPool(G_DynamicCommandPool, nullptr, G_DLD);
			G_DynamicCommandPool = nullptr;
		}

		if (G_StaticCommandPool) {
			G_Device.destroyCommandPool(G_StaticCommandPool, nullptr, G_DLD);
			G_StaticCommandPool = nullptr;
		}

		G_Device.destroy(nullptr, G_DLD);
		G_Device = nullptr;
	}

	if (G_VkInstance) {
		G_VkInstance.destroy(nullptr, G_DLD);
		G_VkInstance = nullptr;
	}
}

void InitPhysicalDevice()
{
	const std::vector<vk::PhysicalDevice> AvailableDevices = G_VkInstance.enumeratePhysicalDevices(G_DLD);
	static constexpr std::array<const char*, 1> RequiredDeviceLayers = {
		"VK_LAYER_KHRONOS_validation",
	};
	static constexpr std::array<const char*, 1> RequiredDeviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};

	std::vector<vk::PhysicalDevice> SuitablePhysicalDevices;

	for (auto PhysDevice : AvailableDevices) {
		bool bAllExtensionsSupported = true;
		bool bAllLayersSupported = true;

		const std::vector<vk::ExtensionProperties> SupportedDeviceExtensions = PhysDevice.enumerateDeviceExtensionProperties(nullptr, G_DLD);
		const std::vector<vk::LayerProperties> SupportedDeviceLayers = PhysDevice.enumerateDeviceLayerProperties(G_DLD);

		for (const char* RequiredLayer : RequiredDeviceLayers) {
			if (std::find_if(SupportedDeviceLayers.begin(), SupportedDeviceLayers.end(), [&RequiredLayer](const vk::LayerProperties& ExtensionProperties) {return std::strcmp(ExtensionProperties.layerName.data(), RequiredLayer) == 0; }) == SupportedDeviceLayers.end()) {
				bAllLayersSupported = false;
				break;
			}
		}
		for (const char* RequiredExtension : RequiredDeviceExtensions) {
			if (std::find_if(SupportedDeviceExtensions.begin(), SupportedDeviceExtensions.end(), [&RequiredExtension](const vk::ExtensionProperties& ExtensionProperties) {return std::strcmp(ExtensionProperties.extensionName.data(), RequiredExtension) == 0; }) == SupportedDeviceExtensions.end()) {
				bAllExtensionsSupported = false;
				break;
			}
		}
		if (bAllLayersSupported && bAllExtensionsSupported) {
			SuitablePhysicalDevices.push_back(PhysDevice);
		}
	}

	if (!SuitablePhysicalDevices.empty()) {
		for (auto Item : SuitablePhysicalDevices) {
			auto Props = Item.getProperties(G_DLD);
			if (Props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
				G_PhysicalDevice = Item;
				break;
			}
		}

		if (!G_PhysicalDevice)
			G_PhysicalDevice = SuitablePhysicalDevices[0];
	}

	if (!G_PhysicalDevice)
		throw std::runtime_error("Could not find any suitable physical device");


	const vk::PhysicalDeviceProperties PhysDeviceProps = G_PhysicalDevice.getProperties(G_DLD);

	const vk::SampleCountFlags SampleCountFlags = PhysDeviceProps.limits.framebufferColorSampleCounts & PhysDeviceProps.limits.framebufferDepthSampleCounts;
	if (SampleCountFlags & vk::SampleCountFlagBits::e8) { G_SampleCount = vk::SampleCountFlagBits::e8; }
	else if (SampleCountFlags & vk::SampleCountFlagBits::e4) { G_SampleCount = vk::SampleCountFlagBits::e4; }
	else if (SampleCountFlags & vk::SampleCountFlagBits::e2) { G_SampleCount = vk::SampleCountFlagBits::e2; }
	else { G_SampleCount = vk::SampleCountFlagBits::e1; }

}

void InitQueueFamilies()
{

	G_GraphicsQueueFamilyIndex.reset();
	G_PresentQueueFamilyIndex.reset();
	G_ComputeQueueFamilyIndex.reset();

	const std::vector<vk::QueueFamilyProperties> QueueFamilies = G_PhysicalDevice.getQueueFamilyProperties(G_DLD);

	std::vector<vk::Bool32> SupportsPresent = std::vector<vk::Bool32>(QueueFamilies.size(), vk::False);
	for (std::uint32_t i = 0; i < QueueFamilies.size(); i++) {
		SupportsPresent[i] = G_PhysicalDevice.getWin32PresentationSupportKHR(i, G_DLD);
	}

	for (std::uint32_t i = 0; i < QueueFamilies.size(); i++) {
		if (QueueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
			if (SupportsPresent[i] == vk::True) {
				G_GraphicsQueueFamilyIndex = static_cast<std::uint32_t>(i);
				G_PresentQueueFamilyIndex = static_cast<std::uint32_t>(i);
				break;
			}
			if (!G_GraphicsQueueFamilyIndex.has_value()) {
				G_GraphicsQueueFamilyIndex = static_cast<std::uint32_t>(i);
			}
		}
	}
	if (!G_PresentQueueFamilyIndex.has_value()) {
		for (std::uint32_t i = 0; i < QueueFamilies.size(); i++) {
			if (SupportsPresent[i] == vk::True) {
				G_PresentQueueFamilyIndex = static_cast<std::uint32_t>(i);
				break;
			}
		}
	}

	for (std::uint32_t i = 0; i < QueueFamilies.size(); i++) {
		if (QueueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) {
			if (!(QueueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics)) {
				G_ComputeQueueFamilyIndex = static_cast<std::uint32_t>(i);
				break;
			}
		}
	}

	if (!G_ComputeQueueFamilyIndex.has_value()) {
		for (std::uint32_t i = 0; i < QueueFamilies.size(); i++) {
			if (QueueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) {
				G_ComputeQueueFamilyIndex = static_cast<std::uint32_t>(i);
				break;
			}
		}
	}

	if (!G_GraphicsQueueFamilyIndex.has_value() ||
		!G_PresentQueueFamilyIndex.has_value() ||
		!G_ComputeQueueFamilyIndex.has_value()) {
		throw std::runtime_error("Failed to find all required queue families");
	}

}

void InitDevice()
{
	std::set<std::uint32_t> UniqueIndices;
	UniqueIndices.insert(G_GraphicsQueueFamilyIndex.value());
	UniqueIndices.insert(G_PresentQueueFamilyIndex.value());
	UniqueIndices.insert(G_ComputeQueueFamilyIndex.value());

	std::vector<vk::DeviceQueueCreateInfo> QueueCIs;
	const float QueuePriority = 1.0f;
	for (std::uint32_t QueueFamilyIndex : UniqueIndices) {
		QueueCIs.push_back(
			vk::DeviceQueueCreateInfo(
				{},
				QueueFamilyIndex,
				1,
				&QueuePriority
				)
			);
	}

	static constexpr std::array<const char*, 1> EnabledLayers = {
		"VK_LAYER_KHRONOS_validation",
	};
	static constexpr std::array<const char*, 1> EnabledExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
	vk::PhysicalDeviceFeatures EnabledFeatures = vk::PhysicalDeviceFeatures{};
	//EnabledFeatures.setTessellationShader(vk::True);
	EnabledFeatures.setFillModeNonSolid(vk::True);

	const vk::DeviceCreateInfo DeviceCI = vk::DeviceCreateInfo(
		{},
		static_cast<std::uint32_t>(QueueCIs.size()), QueueCIs.data(),
		static_cast<std::uint32_t>(EnabledLayers.size()), EnabledLayers.data(),
		static_cast<std::uint32_t>(EnabledExtensions.size()), EnabledExtensions.data(),
		&EnabledFeatures
	);

	G_Device = G_PhysicalDevice.createDevice(DeviceCI, nullptr, G_DLD);
	if (!G_Device)
		throw std::runtime_error("Failed to create logical device");
	G_DLD.init(G_Device);
}

void InitQueues()
{
	G_GraphicsQueue = G_Device.getQueue(G_GraphicsQueueFamilyIndex.value(), 0, G_DLD);
	G_PresentQueue = G_Device.getQueue(G_PresentQueueFamilyIndex.value(), 0, G_DLD);
	G_ComputeQueue = G_Device.getQueue(G_ComputeQueueFamilyIndex.value(), 0, G_DLD);
}

void InitCommandPools()
{
	const vk::CommandPoolCreateInfo DynamicCommandPoolCI = vk::CommandPoolCreateInfo{
		vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		G_GraphicsQueueFamilyIndex.value()
	};
	G_DynamicCommandPool = G_Device.createCommandPool(DynamicCommandPoolCI, nullptr, G_DLD);

	const vk::CommandPoolCreateInfo StaticCommandPoolCI = vk::CommandPoolCreateInfo{
		{},
		G_GraphicsQueueFamilyIndex.value()
	};
	G_StaticCommandPool = G_Device.createCommandPool(StaticCommandPoolCI, nullptr, G_DLD);
}

void InitCommandBuffers()
{
	const vk::CommandBufferAllocateInfo CommandBufferAI = vk::CommandBufferAllocateInfo(
		G_DynamicCommandPool,
		vk::CommandBufferLevel::ePrimary,
		G_MaxFramesInFlight
	);

	const std::vector<vk::CommandBuffer> CmdBuffers = G_Device.allocateCommandBuffers(CommandBufferAI, G_DLD);
	for (std::uint32_t i = 0; i < G_MaxFramesInFlight; i++) {
		G_CommandBuffers[i] = CmdBuffers[i];
	}
}

void InitSyncObjects()
{
	const vk::SemaphoreCreateInfo SemaphoreCI = {};
	const vk::FenceCreateInfo FenceCI = vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled);

	for (std::size_t i = 0; i < G_MaxFramesInFlight; i++) {
		G_ImageAvailableSemaphores[i] = G_Device.createSemaphore(SemaphoreCI, nullptr, G_DLD);
		G_RenderFinishedSemaphores[i] = G_Device.createSemaphore(SemaphoreCI, nullptr, G_DLD);
		G_InFlightFences[i] = G_Device.createFence(FenceCI, nullptr, G_DLD);
		G_WaitForFences[i] = true;
	}
}

void InitSurface()
{
	const vk::Win32SurfaceCreateInfoKHR SurfaceCI = vk::Win32SurfaceCreateInfoKHR({}, G_Hinstance, G_Hwnd, {});

	G_Surface = G_VkInstance.createWin32SurfaceKHR(SurfaceCI, nullptr, G_DLD);
	if (!G_Device)
		throw std::runtime_error("Failed to create win32 surface");


	const std::vector<vk::SurfaceFormatKHR> SurfaceFormats = G_PhysicalDevice.getSurfaceFormatsKHR(G_Surface, G_DLD);
	if (SurfaceFormats.empty())
		throw std::runtime_error("The surface doesn't support any format");

	G_SurfaceFormat = SurfaceFormats[0];
	for (auto& SurfaceFormat : SurfaceFormats) {
		if (SurfaceFormat.format == vk::Format::eB8G8R8A8Unorm && SurfaceFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
			G_SurfaceFormat = SurfaceFormat;
			break;
		}
	}

	const std::vector<vk::PresentModeKHR> PresentModes = G_PhysicalDevice.getSurfacePresentModesKHR(G_Surface, G_DLD);
	if (PresentModes.empty())
		throw std::runtime_error("The surface doesn't support any present mode");

	G_SurfacePresentMode = PresentModes[0];
	if (G_SurfacePresentMode != vk::PresentModeKHR::eFifo) {
		for (auto& PresentMode : PresentModes) {
			if (PresentMode == vk::PresentModeKHR::eFifo) {
				G_SurfacePresentMode = PresentMode;
				break;
			}
		}
	}
	if (G_SurfacePresentMode != vk::PresentModeKHR::eMailbox && G_SurfacePresentMode != vk::PresentModeKHR::eFifo) {
		for (auto& PresentMode : PresentModes) {
			if (PresentMode == vk::PresentModeKHR::eMailbox) {
				G_SurfacePresentMode = PresentMode;
				break;
			}
		}
	}

	vk::SurfaceCapabilitiesKHR SurfaceCapabilities = G_PhysicalDevice.getSurfaceCapabilitiesKHR(G_Surface, G_DLD);
	G_SurfaceImageCount = std::min(
		SurfaceCapabilities.maxImageCount,
		std::max(SurfaceCapabilities.minImageCount, G_PreferredImageCount)
	);


	static constexpr std::array<vk::Format, 5> CandidateDepthFormats = std::array<vk::Format, 5>(
		{
			vk::Format::eD32SfloatS8Uint,
			vk::Format::eD24UnormS8Uint,
			vk::Format::eD16UnormS8Uint,
			vk::Format::eD32Sfloat,
			vk::Format::eD16Unorm,
		}
	);

	G_DepthFormat = vk::Format::eUndefined;
	for (auto& Fmt : CandidateDepthFormats) {
		const vk::FormatProperties FormatProps = G_PhysicalDevice.getFormatProperties(Fmt, G_DLD);
		if ((FormatProps.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) == vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
			G_DepthFormat = Fmt;
			break;
		}
	}
	if (G_DepthFormat == vk::Format::eUndefined)
		throw std::runtime_error("Failed to find proper depth format");

}

void InitRenderPass()
{

	if (G_SampleCount == vk::SampleCountFlagBits::e1) {

		const std::array<vk::AttachmentDescription, 2> attachments = {
			vk::AttachmentDescription(
				{},
				G_SurfaceFormat.format,
				vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eClear,
				vk::AttachmentStoreOp::eStore,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eDontCare,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::ePresentSrcKHR
				),
			vk::AttachmentDescription(
				{},
				G_DepthFormat,
				vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eClear,
				vk::AttachmentStoreOp::eDontCare,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eDontCare,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eDepthStencilAttachmentOptimal
				),
			};

		static constexpr std::array<vk::AttachmentReference, 1> ColorAttachmentRefs = {
			vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal),
		};
		static constexpr std::array<vk::AttachmentReference, 1> DepthAttachmentRefs = {
			vk::AttachmentReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal),
		};

		static constexpr vk::SubpassDescription subpass = vk::SubpassDescription(
			{},
			vk::PipelineBindPoint::eGraphics,
			0,
			nullptr,
			1,
			&ColorAttachmentRefs[0],
			nullptr,
			&DepthAttachmentRefs[0],
			0,
			nullptr
			);

		const vk::RenderPassCreateInfo renderPassCI = vk::RenderPassCreateInfo(
			{},
			static_cast<std::uint32_t>(attachments.size()),
			attachments.data(),
			1,
			&subpass
			);

		G_RenderPass = G_Device.createRenderPass(renderPassCI, nullptr, G_DLD);
		if (!G_RenderPass)
			throw std::runtime_error("Failed to create renderpass");


	} else {

		const std::array<vk::AttachmentDescription, 3> attachments = {
			vk::AttachmentDescription(
				{},
				G_SurfaceFormat.format,
				G_SampleCount,
				vk::AttachmentLoadOp::eClear,
				vk::AttachmentStoreOp::eDontCare,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eDontCare,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eColorAttachmentOptimal
				),
			vk::AttachmentDescription(
				{},
				G_SurfaceFormat.format,
				vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eStore,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eDontCare,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::ePresentSrcKHR
				),
			vk::AttachmentDescription(
				{},
				G_DepthFormat,
				G_SampleCount,
				vk::AttachmentLoadOp::eClear,
				vk::AttachmentStoreOp::eDontCare,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eDontCare,
				vk::ImageLayout::eUndefined,
				vk::ImageLayout::eDepthStencilAttachmentOptimal
				),
			};

		static constexpr std::array<vk::AttachmentReference, 1> ColorAttachmentRefs = {
			vk::AttachmentReference(0, vk::ImageLayout::eColorAttachmentOptimal),
		};
		static constexpr std::array<vk::AttachmentReference, 1> ResolveAttachmentRefs = {
			vk::AttachmentReference(1, vk::ImageLayout::eColorAttachmentOptimal),
		};
		static constexpr std::array<vk::AttachmentReference, 1> DepthAttachmentRefs = {
			vk::AttachmentReference(2, vk::ImageLayout::eDepthStencilAttachmentOptimal),
		};

		static constexpr vk::SubpassDescription subpass = vk::SubpassDescription(
			{},
			vk::PipelineBindPoint::eGraphics,
			0,
			nullptr,
			1,
			&ColorAttachmentRefs[0],
			&ResolveAttachmentRefs[0],
			&DepthAttachmentRefs[0],
			0,
			nullptr
			);

		const vk::RenderPassCreateInfo renderPassCI = vk::RenderPassCreateInfo(
			{},
			static_cast<std::uint32_t>(attachments.size()),
			attachments.data(),
			1,
			&subpass
			);

		G_RenderPass = G_Device.createRenderPass(renderPassCI, nullptr, G_DLD);
		if (!G_RenderPass)
			throw std::runtime_error("Failed to create multisampled renderpass");
	}
}



void InitSwapchain()
{
	CreateSwapchain();
}

void CreateSwapchain()
{
	if (G_SwapchainOK) {
		DestroySwapchain();
	}

	RECT rc = {};
	::GetClientRect(G_Hwnd, &rc);
	G_WindowSize = vk::Extent2D{static_cast<std::uint32_t>(rc.right - rc.left), static_cast<std::uint32_t>(rc.bottom - rc.top)};

	try {

		const vk::SurfaceCapabilitiesKHR SurfaceCapabilities = G_PhysicalDevice.getSurfaceCapabilitiesKHR(G_Surface, G_DLD);

		G_SwapchainExtent = SurfaceCapabilities.currentExtent;
		if (G_SwapchainExtent.width == std::numeric_limits<std::uint32_t>::max()) {
			G_SwapchainExtent.width = G_WindowSize.width;
			G_SwapchainExtent.width = std::min(SurfaceCapabilities.maxImageExtent.width,std::max(SurfaceCapabilities.minImageExtent.width, G_SwapchainExtent.width));
		}
		if (G_SwapchainExtent.height == std::numeric_limits<std::uint32_t>::max()) {
			G_SwapchainExtent.height = G_WindowSize.height;
			G_SwapchainExtent.height = std::min(SurfaceCapabilities.maxImageExtent.height,std::max(SurfaceCapabilities.minImageExtent.height, G_SwapchainExtent.height));
		}
		if (G_SwapchainExtent.width == 0 || G_SwapchainExtent.height == 0) return;

		if (G_GraphicsQueueFamilyIndex.value() != G_PresentQueueFamilyIndex.value()) {
			const std::uint32_t QueueFamilyIndices[] = { G_GraphicsQueueFamilyIndex.value(), G_PresentQueueFamilyIndex.value()};
			const vk::SwapchainCreateInfoKHR swapchainCI = vk::SwapchainCreateInfoKHR(
				{},
				G_Surface,
				G_SurfaceImageCount,
				G_SurfaceFormat.format,
				G_SurfaceFormat.colorSpace,
				G_SwapchainExtent,
				1,
				vk::ImageUsageFlagBits::eColorAttachment,
				vk::SharingMode::eConcurrent,
				2,
				QueueFamilyIndices,
				SurfaceCapabilities.currentTransform,
				vk::CompositeAlphaFlagBitsKHR::eOpaque,
				G_SurfacePresentMode,
				vk::True
				);
			G_Swapchain = G_Device.createSwapchainKHR(swapchainCI, nullptr, G_DLD);
		}
		else {
			const vk::SwapchainCreateInfoKHR swapchainCI = vk::SwapchainCreateInfoKHR(
				{},
				G_Surface,
				G_SurfaceImageCount,
				G_SurfaceFormat.format,
				G_SurfaceFormat.colorSpace,
				G_SwapchainExtent,
				1,
				vk::ImageUsageFlagBits::eColorAttachment,
				vk::SharingMode::eExclusive,
				{},
				{},
				SurfaceCapabilities.currentTransform,
				vk::CompositeAlphaFlagBitsKHR::eOpaque,
				G_SurfacePresentMode,
				vk::True
				);
			G_Swapchain = G_Device.createSwapchainKHR(swapchainCI, nullptr, G_DLD);
		}

		if (!G_Swapchain)return;
		G_SwapchainOK = true;


	} catch(...) {
		return;
	}

	G_SwapchainImages = G_Device.getSwapchainImagesKHR(G_Swapchain, G_DLD);
	if (G_SwapchainImages.empty()) {
		DestroySwapchain();
		return;
	}

	const std::size_t NumImages = G_SwapchainImages.size();
	G_SwapchainImageViews.resize(NumImages);
	std::fill(G_SwapchainImageViews.begin(), G_SwapchainImageViews.end(), nullptr);

	for (std::size_t i = 0; i < NumImages; ++i) {
		const vk::ImageViewCreateInfo ImageViewCI = vk::ImageViewCreateInfo(
			{},
			G_SwapchainImages[i],
			vk::ImageViewType::e2D,
			G_SurfaceFormat.format,
			vk::ComponentMapping(),
			vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1)
			);

		G_SwapchainImageViews[i] = G_Device.createImageView(ImageViewCI, nullptr, G_DLD);
		if (!G_SwapchainImageViews[i]) {
			DestroySwapchain();
			return;
		}
	}

	if (G_SampleCount != vk::SampleCountFlagBits::e1) {

		G_ColorBufferImages.resize(NumImages);
		G_ColorBufferDeviceMemories.resize(NumImages);
		G_ColorBufferImageViews.resize(NumImages);
		std::fill(G_ColorBufferImages.begin(), G_ColorBufferImages.end(), nullptr);
		std::fill(G_ColorBufferDeviceMemories.begin(), G_ColorBufferDeviceMemories.end(), nullptr);
		std::fill(G_ColorBufferImageViews.begin(), G_ColorBufferImageViews.end(), nullptr);


		for (std::size_t i = 0; i < NumImages; i++) {

			const vk::ImageCreateInfo ImageCI = vk::ImageCreateInfo(
				{},
				vk::ImageType::e2D,
				G_SurfaceFormat.format,
				vk::Extent3D{G_SwapchainExtent.width, G_SwapchainExtent.height, 1},
				1,
				1,
				G_SampleCount,
				vk::ImageTiling::eOptimal,
				vk::ImageUsageFlagBits::eColorAttachment,
				vk::SharingMode::eExclusive,
				{},
				vk::ImageLayout::eUndefined
				);

			G_ColorBufferImages[i] = G_Device.createImage(ImageCI, nullptr, G_DLD);
			if (!G_ColorBufferImages[i]) {
				DestroySwapchain();
				return;
			}

			const vk::MemoryRequirements ImageMemReqs = G_Device.getImageMemoryRequirements(G_ColorBufferImages[i], G_DLD);
			const std::uint32_t ImageMemoryTypeIndex = FindMemoryTypeIndex(ImageMemReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
			const vk::MemoryAllocateInfo ImageMemoryAI = vk::MemoryAllocateInfo{ImageMemReqs.size, ImageMemoryTypeIndex};

			G_ColorBufferDeviceMemories[i] = G_Device.allocateMemory(ImageMemoryAI, nullptr, G_DLD);
			if (!G_ColorBufferDeviceMemories[i]) {
				DestroySwapchain();
				return;
			}

			G_Device.bindImageMemory(G_ColorBufferImages[i], G_ColorBufferDeviceMemories[i], 0, G_DLD);

			const vk::ImageViewCreateInfo ImageViewCI = vk::ImageViewCreateInfo(
				{},
				G_ColorBufferImages[i],
				vk::ImageViewType::e2D,
				G_SurfaceFormat.format,
				{},
				vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}
				);

			G_ColorBufferImageViews[i] = G_Device.createImageView(ImageViewCI, nullptr, G_DLD);
			if (!G_ColorBufferImageViews[i]) {
				DestroySwapchain();
				return;
			}
		}
	}


	G_DepthBufferImages.resize(NumImages);
	G_DepthBufferDeviceMemories.resize(NumImages);
	G_DepthBufferImageViews.resize(NumImages);
	std::fill(G_DepthBufferImages.begin(), G_DepthBufferImages.end(), nullptr);
	std::fill(G_DepthBufferDeviceMemories.begin(), G_DepthBufferDeviceMemories.end(), nullptr);
	std::fill(G_DepthBufferImageViews.begin(), G_DepthBufferImageViews.end(), nullptr);


	for (std::size_t i = 0; i < NumImages; i++) {

		const vk::ImageCreateInfo ImageCI = vk::ImageCreateInfo(
			{},
			vk::ImageType::e2D,
			G_DepthFormat,
			vk::Extent3D{G_SwapchainExtent.width, G_SwapchainExtent.height, 1},
			1,
			1,
			G_SampleCount,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::SharingMode::eExclusive,
			{},
			vk::ImageLayout::eUndefined
		);

		G_DepthBufferImages[i] = G_Device.createImage(ImageCI, nullptr, G_DLD);
		if (!G_DepthBufferImages[i]) {
			DestroySwapchain();
			return;
		}

		const vk::MemoryRequirements ImageMemReqs = G_Device.getImageMemoryRequirements(G_DepthBufferImages[i], G_DLD);
		const std::uint32_t ImageMemoryTypeIndex = FindMemoryTypeIndex(ImageMemReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
		const vk::MemoryAllocateInfo ImageMemoryAI = vk::MemoryAllocateInfo{ImageMemReqs.size, ImageMemoryTypeIndex};

		G_DepthBufferDeviceMemories[i] = G_Device.allocateMemory(ImageMemoryAI, nullptr, G_DLD);
		if (!G_DepthBufferDeviceMemories[i]) {
			DestroySwapchain();
			return;
		}

		G_Device.bindImageMemory(G_DepthBufferImages[i], G_DepthBufferDeviceMemories[i], 0, G_DLD);

		const vk::ImageViewCreateInfo ImageViewCI = vk::ImageViewCreateInfo(
			{},
			G_DepthBufferImages[i],
			vk::ImageViewType::e2D,
			G_DepthFormat,
			{},
			vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}
		);

		G_DepthBufferImageViews[i] = G_Device.createImageView(ImageViewCI, nullptr, G_DLD);
		if (!G_DepthBufferImageViews[i]) {
			DestroySwapchain();
			return;
		}
	}

	if (G_SampleCount == vk::SampleCountFlagBits::e1) {

		G_Framebuffers.resize(NumImages);
		std::fill(G_Framebuffers.begin(), G_Framebuffers.end(), nullptr);

		for (std::size_t i = 0; i < NumImages; ++i) {
			const std::vector<vk::ImageView> Attachments = {
				G_SwapchainImageViews[i],
				G_DepthBufferImageViews[i]
			};

			const vk::FramebufferCreateInfo FramebufferCI = vk::FramebufferCreateInfo(
				{},
				G_RenderPass,
				static_cast<std::uint32_t>(Attachments.size()),
				Attachments.data(),
				G_SwapchainExtent.width,
				G_SwapchainExtent.height,
				1
				);

			G_Framebuffers[i] = G_Device.createFramebuffer(FramebufferCI, nullptr, G_DLD);
			if (!G_Framebuffers[i]) {
				DestroySwapchain();
				return;
			}
		}

	} else {

		G_Framebuffers.resize(NumImages);
		std::fill(G_Framebuffers.begin(), G_Framebuffers.end(), nullptr);

		for (std::size_t i = 0; i < NumImages; ++i) {
			const std::vector<vk::ImageView> Attachments = {
				G_ColorBufferImageViews[i],
				G_SwapchainImageViews[i],
				G_DepthBufferImageViews[i]
			};

			const vk::FramebufferCreateInfo FramebufferCI = vk::FramebufferCreateInfo(
				{},
				G_RenderPass,
				static_cast<std::uint32_t>(Attachments.size()),
				Attachments.data(),
				G_SwapchainExtent.width,
				G_SwapchainExtent.height,
				1
				);

			G_Framebuffers[i] = G_Device.createFramebuffer(FramebufferCI, nullptr, G_DLD);
			if (!G_Framebuffers[i]) {
				DestroySwapchain();
				return;
			}
		}


	}
}

void DestroySwapchain()
{
	G_SwapchainOK = false;

	if (G_Device) {
		G_Device.waitIdle(G_DLD);

		for(auto& Item : G_Framebuffers) {
			if (Item) {
				G_Device.destroyFramebuffer(Item, nullptr, G_DLD);
				Item = nullptr;
			}
			G_Framebuffers.clear();
		}

		for(auto& Item : G_DepthBufferImageViews) {
			if (Item) {
				G_Device.destroyImageView(Item, nullptr, G_DLD);
				Item = nullptr;
			}
			G_DepthBufferImageViews.clear();
		}
		for(auto& Item : G_DepthBufferDeviceMemories) {
			if (Item) {
				G_Device.freeMemory(Item, nullptr, G_DLD);
				Item = nullptr;
			}
			G_DepthBufferDeviceMemories.clear();
		}
		for(auto& Item : G_DepthBufferImages) {
			if (Item) {
				G_Device.destroyImage(Item, nullptr, G_DLD);
				Item = nullptr;
			}
			G_DepthBufferImages.clear();
		}


		for(auto& Item : G_ColorBufferImageViews) {
			if (Item) {
				G_Device.destroyImageView(Item, nullptr, G_DLD);
				Item = nullptr;
			}
			G_ColorBufferImageViews.clear();
		}
		for(auto& Item : G_ColorBufferDeviceMemories) {
			if (Item) {
				G_Device.freeMemory(Item, nullptr, G_DLD);
				Item = nullptr;
			}
			G_ColorBufferDeviceMemories.clear();
		}
		for(auto& Item : G_ColorBufferImages) {
			if (Item) {
				G_Device.destroyImage(Item, nullptr, G_DLD);
				Item = nullptr;
			}
			G_ColorBufferImages.clear();
		}

		for(auto& Item : G_SwapchainImageViews) {
			if (Item) {
				G_Device.destroyImageView(Item, nullptr, G_DLD);
				Item = nullptr;
			}
			G_SwapchainImageViews.clear();
		}

		G_SwapchainImages.clear();

		if (G_Swapchain) {
			G_Device.destroySwapchainKHR(G_Swapchain, nullptr, G_DLD);
			G_Swapchain = nullptr;
		}
	}
}

void RecreateSwapchain()
{
	DestroySwapchain();
	CreateSwapchain();
}

void ShutdownSwapchain()
{
	DestroySwapchain();
}

void InitImGui()
{
	static constexpr vk::DescriptorPoolSize PoolSizes[1] = {
		vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 10),
	};
	static constexpr vk::DescriptorPoolCreateInfo DescriptorPoolCI = vk::DescriptorPoolCreateInfo(
		vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		10,
		1,
		PoolSizes
	);
	G_ImguiDescriptorPool = G_Device.createDescriptorPool(DescriptorPoolCI, nullptr, G_DLD);


	IMGUI_CHECKVERSION();
	G_ImGuiContext = ImGui::CreateContext();
	ImGui::SetCurrentContext(G_ImGuiContext);
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	//G_ConsolasFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 20.0f, NULL, io.Fonts->GetGlyphRangesDefault());

	ImGui_ImplWin32_Init(G_Hwnd);

	auto VkLoaderFunction = [](const char* function_name, void* user_data) -> PFN_vkVoidFunction {
		auto pfn = G_DLD.vkGetInstanceProcAddr(G_VkInstance, function_name);
		if (!pfn) pfn = G_DLD.vkGetDeviceProcAddr(G_Device, function_name);
		return pfn;
	};
	ImGui_ImplVulkan_LoadFunctions(VkLoaderFunction, nullptr);

	auto VkCheckError = [](VkResult err){};

	ImGui_ImplVulkan_InitInfo VkInitInfo;
	VkInitInfo.Instance            = G_VkInstance;
	VkInitInfo.PhysicalDevice      = G_PhysicalDevice;
	VkInitInfo.Device              = G_Device;
	VkInitInfo.QueueFamily         = G_GraphicsQueueFamilyIndex.value();
	VkInitInfo.Queue               = G_GraphicsQueue;
	VkInitInfo.DescriptorPool      = G_ImguiDescriptorPool;
	VkInitInfo.RenderPass          = G_RenderPass;
	VkInitInfo.MinImageCount       = 2;
	VkInitInfo.ImageCount          = 2;
	VkInitInfo.MSAASamples         = static_cast<VkSampleCountFlagBits>(G_SampleCount);
	VkInitInfo.PipelineCache       = nullptr;
	VkInitInfo.Subpass             = 0;
	VkInitInfo.UseDynamicRendering = false;
	VkInitInfo.Allocator           = nullptr;
	VkInitInfo.CheckVkResultFn     = VkCheckError;
	VkInitInfo.MinAllocationSize   = 1024 * 1024;
	ImGui_ImplVulkan_Init(&VkInitInfo);



	ImGui::SetCurrentContext(nullptr);
}

void ShutdownImGui()
{
	ImGui::SetCurrentContext(G_ImGuiContext);

	ImGui_ImplVulkan_Shutdown();

	if (G_ImguiDescriptorPool) {
		G_Device.destroyDescriptorPool(G_ImguiDescriptorPool, nullptr, G_DLD);
		G_ImguiDescriptorPool = nullptr;
	}

	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext(G_ImGuiContext);
	ImGui::SetCurrentContext(nullptr);
	G_ImGuiContext = nullptr;
}

void InitPipeline()
{
	static constexpr vk::DescriptorSetLayoutBinding DescriptorSetLayoutBinding = vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eVertex, nullptr);
	static constexpr vk::DescriptorSetLayoutCreateInfo DescriptorSetLayoutCI = vk::DescriptorSetLayoutCreateInfo({}, 1, &DescriptorSetLayoutBinding);
	G_SkinsDescriptorSetLayout = G_Device.createDescriptorSetLayout(DescriptorSetLayoutCI, nullptr, G_DLD);

	static constexpr vk::PushConstantRange PushConstantRanges[2] = {
		vk::PushConstantRange(vk::ShaderStageFlagBits::eVertex, 0, 2 * sizeof(DirectX::XMFLOAT4X4)),
		vk::PushConstantRange(vk::ShaderStageFlagBits::eFragment, 2 * sizeof(DirectX::XMFLOAT4X4), sizeof(DirectX::XMFLOAT3)),
	};
	static constexpr vk::PipelineLayoutCreateInfo PipelineLayoutCI = vk::PipelineLayoutCreateInfo{{}, 1, &G_SkinsDescriptorSetLayout, 2, PushConstantRanges};
	G_PipelineLayout = G_Device.createPipelineLayout(PipelineLayoutCI, nullptr, G_DLD);

	vk::ShaderModule ShaderModuleVS = CreateShader("DefaultVS.spv");
	vk::ShaderModule ShaderModuleFS = CreateShader("DefaultFS.spv");

	const std::array<vk::PipelineShaderStageCreateInfo, 2> ShaderStageCIs = {
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, ShaderModuleVS, "main"),
		vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, ShaderModuleFS, "main"),
	};

	static constexpr vk::VertexInputBindingDescription VertexInputBindingDescriptions[1] = {
		vk::VertexInputBindingDescription(0, sizeof(VkGltfModel::Vertex), vk::VertexInputRate::eVertex),
	};
	static constexpr vk::VertexInputAttributeDescription VertexInputAttributeDescriptions[7] = {
		vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat,    0),
		vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat,    sizeof(DirectX::XMFLOAT3)),
		vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32Sfloat,       sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT3)),
		vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32B32A32Uint,   sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2)),
		vk::VertexInputAttributeDescription(4, 0, vk::Format::eR32G32B32A32Sfloat, sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2) + sizeof(DirectX::XMUINT4)),
		vk::VertexInputAttributeDescription(5, 0, vk::Format::eR32G32B32A32Uint,   sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2) + sizeof(DirectX::XMUINT4) + sizeof(DirectX::XMFLOAT4)),
		vk::VertexInputAttributeDescription(6, 0, vk::Format::eR32G32B32A32Sfloat, sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT3) + sizeof(DirectX::XMFLOAT2) + sizeof(DirectX::XMUINT4) + sizeof(DirectX::XMFLOAT4) + sizeof(DirectX::XMUINT4)),
	};
	static constexpr vk::PipelineVertexInputStateCreateInfo VertexInputStateCI = vk::PipelineVertexInputStateCreateInfo({}, 1, VertexInputBindingDescriptions, 7, VertexInputAttributeDescriptions);

	static constexpr vk::PipelineInputAssemblyStateCreateInfo InputAssemblyCI = vk::PipelineInputAssemblyStateCreateInfo({}, vk::PrimitiveTopology::eTriangleList);
	static constexpr vk::PipelineTessellationStateCreateInfo TesselationStateCI = vk::PipelineTessellationStateCreateInfo();

	const vk::Viewport Viewport(0.0f, 0.0f, float(G_SwapchainExtent.width), float(G_SwapchainExtent.height));
	const vk::Rect2D Scissor(vk::Offset2D(), vk::Extent2D(G_SwapchainExtent.width, G_SwapchainExtent.height));
	static const vk::PipelineViewportStateCreateInfo ViewportStateCI = vk::PipelineViewportStateCreateInfo({}, 1, &Viewport, 1, &Scissor);

	static constexpr vk::PipelineRasterizationStateCreateInfo RasterizationStateCI = vk::PipelineRasterizationStateCreateInfo({}, {}, {}, vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eClockwise, {}, {}, {}, {}, 1.0f);
	const vk::PipelineMultisampleStateCreateInfo MultisampleCI = vk::PipelineMultisampleStateCreateInfo({}, G_SampleCount);

	static constexpr vk::PipelineDepthStencilStateCreateInfo DepthStencilCI = vk::PipelineDepthStencilStateCreateInfo({}, vk::True, vk::True, vk::CompareOp::eLess, vk::False, vk::False, {}, {}, 0.0f, 1.0f);
	static constexpr vk::PipelineColorBlendAttachmentState ColorBlendAttachmentState = vk::PipelineColorBlendAttachmentState({}, {}, {}, {}, {}, {}, {}, vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
	static constexpr vk::PipelineColorBlendStateCreateInfo BlendStateCI = vk::PipelineColorBlendStateCreateInfo({}, {}, {}, 1, &ColorBlendAttachmentState);

	static constexpr std::array<vk::DynamicState, 2> DynamicStates = { vk::DynamicState::eViewport, vk::DynamicState::eScissor};
	static constexpr vk::PipelineDynamicStateCreateInfo DynamicStateCI = vk::PipelineDynamicStateCreateInfo({}, static_cast<std::uint32_t>(DynamicStates.size()), DynamicStates.data());

	const vk::GraphicsPipelineCreateInfo GraphicsPipelineCI = vk::GraphicsPipelineCreateInfo(
		vk::PipelineCreateFlags(),
		ShaderStageCIs,
		&VertexInputStateCI,
		&InputAssemblyCI,
		&TesselationStateCI,
		&ViewportStateCI,
		&RasterizationStateCI,
		&MultisampleCI,
		&DepthStencilCI,
		&BlendStateCI,
		&DynamicStateCI,
		G_PipelineLayout,
		G_RenderPass,
		0
	);

	G_Pipeline = G_Device.createGraphicsPipeline({}, GraphicsPipelineCI, nullptr, G_DLD).value;

	G_Device.destroyShaderModule(ShaderModuleVS, nullptr, G_DLD);
	G_Device.destroyShaderModule(ShaderModuleFS, nullptr, G_DLD);
}

void ShutdownPipeline()
{
	if (G_Pipeline) {
		G_Device.destroyPipeline(G_Pipeline, nullptr, G_DLD);
		G_Pipeline = nullptr;
	}

	if (G_PipelineLayout) {
		G_Device.destroyPipelineLayout(G_PipelineLayout, nullptr, G_DLD);
		G_PipelineLayout = nullptr;
	}

	if (G_SkinsDescriptorSetLayout) {

		G_Device.destroyDescriptorSetLayout(G_SkinsDescriptorSetLayout, nullptr, G_DLD);
		G_SkinsDescriptorSetLayout = nullptr;
	}
}

void InitModel()
{
	if (!G_GltfModel.LoadFromFile("Bot_Running.glb")) {
		throw std::runtime_error("Failed to load the model");
	}
}

void ShutdownModel()
{
	G_GltfModel.Shutdown();
}



VkGltfModel::VkGltfModel()
{

}

VkGltfModel::~VkGltfModel()
{

}

bool VkGltfModel::LoadFromFile(std::string FileName)
{
	tinygltf::TinyGLTF Loader;

	std::string StrErr;
	std::string StrWarn;
	const std::string FilePath = std::string(APP_SOURCE_PATH) + std::string("/models/") + FileName;

	if (!Loader.LoadASCIIFromFile(&M_Model, &StrErr, &StrWarn, FilePath)) {
		if (!Loader.LoadBinaryFromFile(&M_Model, &StrErr, &StrWarn, FilePath)) {
			return false;
		}
	}

	std::vector<std::uint32_t> HostIndexBuffer;
	std::vector<VkGltfModel::Vertex> HostVertexBuffer;

	const tinygltf::Scene& Scene = M_Model.scenes[0];
	for (std::size_t i = 0; i < Scene.nodes.size(); i++) {
		const tinygltf::Node Node = M_Model.nodes[Scene.nodes[i]];
		LoadNode(Node, nullptr, Scene.nodes[i], HostIndexBuffer, HostVertexBuffer);

	}
	LoadSkins();
	LoadAnimations();



	for (auto Node : M_Nodes)
	{
		UpdateJoints(Node);
	}
	UpdateAnimation(0.1);


	M_VertexBufferTuple = CreateBuffer(vk::BufferUsageFlagBits::eVertexBuffer, HostVertexBuffer.size() * sizeof(Vertex), HostVertexBuffer.data(), true);
	M_IndexBufferTuple = CreateBuffer(vk::BufferUsageFlagBits::eIndexBuffer, HostIndexBuffer.size() * sizeof(std::uint32_t), HostIndexBuffer.data(), true);

	return true;
}

void VkGltfModel::LoadNode(const tinygltf::Node &InputNode,  std::shared_ptr<VkGltfModel::Node> NodeParent, std::uint32_t NodeIndex, std::vector<uint32_t> &HostIndexBuffer, std::vector<Vertex> &HostVertexBuffer)
{
	std::shared_ptr<VkGltfModel::Node> Node(new VkGltfModel::Node());
	Node->Parent = NodeParent;
	Node->Index = NodeIndex;
	Node->Skin = InputNode.skin;

	const DirectX::XMMATRIX IdentityMatrix = DirectX::XMMatrixIdentity();
	DirectX::XMStoreFloat4x4(&Node->Matrix, IdentityMatrix);

	if (InputNode.translation.size() == 3) {
		std::copy_n(InputNode.translation.begin(), 3, reinterpret_cast<float*>(&Node->Translation));
	}
	if (InputNode.rotation.size() == 4) {
		std::copy_n(InputNode.rotation.begin(), 4, reinterpret_cast<float*>(&Node->Rotation));
	}
	if (InputNode.scale.size() == 3) {
		std::copy_n(InputNode.scale.begin(), 3, reinterpret_cast<float*>(&Node->Scale));
	}
	if (InputNode.matrix.size() == 16) {
		std::copy_n(InputNode.matrix.begin(), 16, reinterpret_cast<float*>(&Node->Matrix));
	}

	M_LinearNodes.push_back(Node);

	if (InputNode.children.size() > 0)
	{
		for (size_t i = 0; i < InputNode.children.size(); i++)
		{
			LoadNode(M_Model.nodes[InputNode.children[i]], Node, InputNode.children[i], HostIndexBuffer, HostVertexBuffer);
		}
	}

	if (InputNode.mesh > -1) {

		const tinygltf::Mesh Mesh = M_Model.meshes[InputNode.mesh];

		for (std::size_t i = 0; i < Mesh.primitives.size(); i++)
		{
			const tinygltf::Primitive &GlTFPrimitive = Mesh.primitives[i];
			std::uint32_t              FirstIndex    = static_cast<std::uint32_t>(HostIndexBuffer.size());
			std::uint32_t              VertexStart   = static_cast<std::uint32_t>(HostVertexBuffer.size());
			std::uint32_t              IndexCount    = 0;
			std::vector<Vertex>        LocalVertexBuffer;

			{
				const float*         PositionBuffer      = nullptr;
				const float*         NormalsBuffer       = nullptr;
				const float*         TexCoordsBuffer     = nullptr;
				const std::uint32_t* JointIndicesBuffer0 = nullptr;
				const float*         JointWeightsBuffer0 = nullptr;
				const std::uint32_t* JointIndicesBuffer1 = nullptr;
				const float*         JointWeightsBuffer1 = nullptr;
				std::size_t          VertexCount         = 0;


				if (GlTFPrimitive.attributes.find("POSITION") != GlTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor&   Accessor = M_Model.accessors[GlTFPrimitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView& View     = M_Model.bufferViews[Accessor.bufferView];
					PositionBuffer                       = reinterpret_cast<const float *>(&(M_Model.buffers[View.buffer].data[Accessor.byteOffset + View.byteOffset]));
					VertexCount                          = Accessor.count;

					LocalVertexBuffer.reserve(VertexCount);

					for (std::size_t v = 0; v < VertexCount; v++) {
						LocalVertexBuffer.emplace_back(Vertex{});
						Vertex& Vtx = LocalVertexBuffer.back();

						std::memset(&Vtx, 0, sizeof(Vertex));

						switch(Accessor.componentType)
						{
						case TINYGLTF_COMPONENT_TYPE_FLOAT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.Pos.x = PositionBuffer[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.Pos.x = PositionBuffer[(v*2)+0];
								Vtx.Pos.y = PositionBuffer[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.Pos.x = PositionBuffer[(v*3)+0];
								Vtx.Pos.y = PositionBuffer[(v*3)+1];
								Vtx.Pos.z = PositionBuffer[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.Pos.x = PositionBuffer[(v*4)+0];
								Vtx.Pos.y = PositionBuffer[(v*4)+1];
								Vtx.Pos.z = PositionBuffer[(v*4)+2];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_DOUBLE:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.Pos.x = reinterpret_cast<const double*>(PositionBuffer)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.Pos.x = reinterpret_cast<const double*>(PositionBuffer)[(v*2)+0];
								Vtx.Pos.y = reinterpret_cast<const double*>(PositionBuffer)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.Pos.x = reinterpret_cast<const double*>(PositionBuffer)[(v*3)+0];
								Vtx.Pos.y = reinterpret_cast<const double*>(PositionBuffer)[(v*3)+1];
								Vtx.Pos.z = reinterpret_cast<const double*>(PositionBuffer)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.Pos.x = reinterpret_cast<const double*>(PositionBuffer)[(v*4)+0];
								Vtx.Pos.y = reinterpret_cast<const double*>(PositionBuffer)[(v*4)+1];
								Vtx.Pos.z = reinterpret_cast<const double*>(PositionBuffer)[(v*4)+2];
								break;
							}
							break;
						}
					}

				} else {
					continue;
				}

				if (GlTFPrimitive.attributes.find("NORMAL") != GlTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor&   Accessor = M_Model.accessors[GlTFPrimitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView& View     = M_Model.bufferViews[Accessor.bufferView];
					NormalsBuffer                        = reinterpret_cast<const float *>(&(M_Model.buffers[View.buffer].data[Accessor.byteOffset + View.byteOffset]));

					for (std::size_t v = 0; v < VertexCount; v++) {
						Vertex& Vtx = LocalVertexBuffer[v];

						switch(Accessor.componentType)
						{
						case TINYGLTF_COMPONENT_TYPE_FLOAT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.Normal.x = NormalsBuffer[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.Normal.x = NormalsBuffer[(v*2)+0];
								Vtx.Normal.y = NormalsBuffer[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.Normal.x = NormalsBuffer[(v*3)+0];
								Vtx.Normal.y = NormalsBuffer[(v*3)+1];
								Vtx.Normal.z = NormalsBuffer[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.Normal.x = NormalsBuffer[(v*4)+0];
								Vtx.Normal.y = NormalsBuffer[(v*4)+1];
								Vtx.Normal.z = NormalsBuffer[(v*4)+2];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_DOUBLE:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.Normal.x = reinterpret_cast<const double*>(NormalsBuffer)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.Normal.x = reinterpret_cast<const double*>(NormalsBuffer)[(v*2)+0];
								Vtx.Normal.y = reinterpret_cast<const double*>(NormalsBuffer)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.Normal.x = reinterpret_cast<const double*>(NormalsBuffer)[(v*3)+0];
								Vtx.Normal.y = reinterpret_cast<const double*>(NormalsBuffer)[(v*3)+1];
								Vtx.Normal.z = reinterpret_cast<const double*>(NormalsBuffer)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.Normal.x = reinterpret_cast<const double*>(NormalsBuffer)[(v*4)+0];
								Vtx.Normal.y = reinterpret_cast<const double*>(NormalsBuffer)[(v*4)+1];
								Vtx.Normal.z = reinterpret_cast<const double*>(NormalsBuffer)[(v*4)+2];
								break;
							}
							break;
						}
					}
				}


				if (GlTFPrimitive.attributes.find("TEXCOORD_0") != GlTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor&   Accessor = M_Model.accessors[GlTFPrimitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView& View     = M_Model.bufferViews[Accessor.bufferView];
					TexCoordsBuffer                      = reinterpret_cast<const float *>(&(M_Model.buffers[View.buffer].data[Accessor.byteOffset + View.byteOffset]));

					for (std::size_t v = 0; v < VertexCount; v++) {
						Vertex& Vtx = LocalVertexBuffer[v];

						switch(Accessor.componentType)
						{
						case TINYGLTF_COMPONENT_TYPE_FLOAT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.Uv.x = TexCoordsBuffer[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.Uv.x = TexCoordsBuffer[(v*2)+0];
								Vtx.Uv.y = TexCoordsBuffer[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.Uv.x = TexCoordsBuffer[(v*3)+0];
								Vtx.Uv.y = TexCoordsBuffer[(v*3)+1];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.Uv.x = TexCoordsBuffer[(v*4)+0];
								Vtx.Uv.y = TexCoordsBuffer[(v*4)+1];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_DOUBLE:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.Uv.x = reinterpret_cast<const double*>(TexCoordsBuffer)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.Uv.x = reinterpret_cast<const double*>(TexCoordsBuffer)[(v*2)+0];
								Vtx.Uv.y = reinterpret_cast<const double*>(TexCoordsBuffer)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.Uv.x = reinterpret_cast<const double*>(TexCoordsBuffer)[(v*3)+0];
								Vtx.Uv.y = reinterpret_cast<const double*>(TexCoordsBuffer)[(v*3)+1];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.Uv.x = reinterpret_cast<const double*>(TexCoordsBuffer)[(v*4)+0];
								Vtx.Uv.y = reinterpret_cast<const double*>(TexCoordsBuffer)[(v*4)+1];
								break;
							}
							break;
						}
					}
				}

				if (GlTFPrimitive.attributes.find("JOINTS_0") != GlTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor&   Accessor = M_Model.accessors[GlTFPrimitive.attributes.find("JOINTS_0")->second];
					const tinygltf::BufferView& View     = M_Model.bufferViews[Accessor.bufferView];
					JointIndicesBuffer0                  = reinterpret_cast<const uint32_t *>(&(M_Model.buffers[View.buffer].data[Accessor.byteOffset + View.byteOffset]));

					for (std::size_t v = 0; v < VertexCount; v++) {
						Vertex& Vtx = LocalVertexBuffer[v];

						switch(Accessor.componentType)
						{
						case TINYGLTF_COMPONENT_TYPE_BYTE:
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointIndices0.x = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer0)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointIndices0.x = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer0)[(v*2)+0];
								Vtx.JointIndices0.y = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer0)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointIndices0.x = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer0)[(v*3)+0];
								Vtx.JointIndices0.y = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer0)[(v*3)+1];
								Vtx.JointIndices0.z = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer0)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointIndices0.x = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer0)[(v*4)+0];
								Vtx.JointIndices0.y = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer0)[(v*4)+1];
								Vtx.JointIndices0.z = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer0)[(v*4)+2];
								Vtx.JointIndices0.w = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer0)[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_SHORT:
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointIndices0.x = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer0)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointIndices0.x = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer0)[(v*2)+0];
								Vtx.JointIndices0.y = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer0)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointIndices0.x = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer0)[(v*3)+0];
								Vtx.JointIndices0.y = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer0)[(v*3)+1];
								Vtx.JointIndices0.z = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer0)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointIndices0.x = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer0)[(v*4)+0];
								Vtx.JointIndices0.y = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer0)[(v*4)+1];
								Vtx.JointIndices0.z = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer0)[(v*4)+2];
								Vtx.JointIndices0.w = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer0)[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_INT:
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointIndices0.x = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer0)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointIndices0.x = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer0)[(v*2)+0];
								Vtx.JointIndices0.y = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer0)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointIndices0.x = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer0)[(v*3)+0];
								Vtx.JointIndices0.y = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer0)[(v*3)+1];
								Vtx.JointIndices0.z = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer0)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointIndices0.x = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer0)[(v*4)+0];
								Vtx.JointIndices0.y = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer0)[(v*4)+1];
								Vtx.JointIndices0.z = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer0)[(v*4)+2];
								Vtx.JointIndices0.w = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer0)[(v*4)+3];
								break;
							}
							break;
						}
					}
				}

				if (GlTFPrimitive.attributes.find("WEIGHTS_0") != GlTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor&   Accessor = M_Model.accessors[GlTFPrimitive.attributes.find("WEIGHTS_0")->second];
					const tinygltf::BufferView& View     = M_Model.bufferViews[Accessor.bufferView];
					JointWeightsBuffer0                  = reinterpret_cast<const float *>(&(M_Model.buffers[View.buffer].data[Accessor.byteOffset + View.byteOffset]));

					for (std::size_t v = 0; v < VertexCount; v++) {
						Vertex& Vtx = LocalVertexBuffer[v];


						switch(Accessor.componentType)
						{
						case TINYGLTF_COMPONENT_TYPE_FLOAT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointWeights0.x = JointWeightsBuffer0[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointWeights0.x = JointWeightsBuffer0[(v*2)+0];
								Vtx.JointWeights0.y = JointWeightsBuffer0[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointWeights0.x = JointWeightsBuffer0[(v*3)+0];
								Vtx.JointWeights0.y = JointWeightsBuffer0[(v*3)+1];
								Vtx.JointWeights0.z = JointWeightsBuffer0[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointWeights0.x = JointWeightsBuffer0[(v*4)+0];
								Vtx.JointWeights0.y = JointWeightsBuffer0[(v*4)+1];
								Vtx.JointWeights0.z = JointWeightsBuffer0[(v*4)+2];
								Vtx.JointWeights0.w = JointWeightsBuffer0[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_DOUBLE:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointWeights0.x = reinterpret_cast<const double*>(JointWeightsBuffer0)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointWeights0.x = reinterpret_cast<const double*>(JointWeightsBuffer0)[(v*2)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const double*>(JointWeightsBuffer0)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointWeights0.x = reinterpret_cast<const double*>(JointWeightsBuffer0)[(v*3)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const double*>(JointWeightsBuffer0)[(v*3)+1];
								Vtx.JointWeights0.z = reinterpret_cast<const double*>(JointWeightsBuffer0)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointWeights0.x = reinterpret_cast<const double*>(JointWeightsBuffer0)[(v*4)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const double*>(JointWeightsBuffer0)[(v*4)+1];
								Vtx.JointWeights0.z = reinterpret_cast<const double*>(JointWeightsBuffer0)[(v*4)+2];
								Vtx.JointWeights0.w = reinterpret_cast<const double*>(JointWeightsBuffer0)[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_BYTE:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointWeights0.x = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer0)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointWeights0.x = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer0)[(v*2)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer0)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointWeights0.x = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer0)[(v*3)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer0)[(v*3)+1];
								Vtx.JointWeights0.z = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer0)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointWeights0.x = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer0)[(v*4)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer0)[(v*4)+1];
								Vtx.JointWeights0.z = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer0)[(v*4)+2];
								Vtx.JointWeights0.w = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer0)[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointWeights0.x = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer0)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointWeights0.x = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer0)[(v*2)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer0)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointWeights0.x = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer0)[(v*3)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer0)[(v*3)+1];
								Vtx.JointWeights0.z = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer0)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointWeights0.x = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer0)[(v*4)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer0)[(v*4)+1];
								Vtx.JointWeights0.z = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer0)[(v*4)+2];
								Vtx.JointWeights0.w = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer0)[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_SHORT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointWeights0.x = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer0)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointWeights0.x = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer0)[(v*2)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer0)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointWeights0.x = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer0)[(v*3)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer0)[(v*3)+1];
								Vtx.JointWeights0.z = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer0)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointWeights0.x = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer0)[(v*4)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer0)[(v*4)+1];
								Vtx.JointWeights0.z = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer0)[(v*4)+2];
								Vtx.JointWeights0.w = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer0)[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointWeights0.x = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer0)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointWeights0.x = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer0)[(v*2)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer0)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointWeights0.x = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer0)[(v*3)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer0)[(v*3)+1];
								Vtx.JointWeights0.z = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer0)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointWeights0.x = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer0)[(v*4)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer0)[(v*4)+1];
								Vtx.JointWeights0.z = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer0)[(v*4)+2];
								Vtx.JointWeights0.w = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer0)[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_INT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointWeights0.x = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer0)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointWeights0.x = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer0)[(v*2)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer0)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointWeights0.x = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer0)[(v*3)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer0)[(v*3)+1];
								Vtx.JointWeights0.z = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer0)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointWeights0.x = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer0)[(v*4)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer0)[(v*4)+1];
								Vtx.JointWeights0.z = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer0)[(v*4)+2];
								Vtx.JointWeights0.w = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer0)[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointWeights0.x = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer0)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointWeights0.x = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer0)[(v*2)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer0)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointWeights0.x = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer0)[(v*3)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer0)[(v*3)+1];
								Vtx.JointWeights0.z = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer0)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointWeights0.x = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer0)[(v*4)+0];
								Vtx.JointWeights0.y = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer0)[(v*4)+1];
								Vtx.JointWeights0.z = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer0)[(v*4)+2];
								Vtx.JointWeights0.w = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer0)[(v*4)+3];
								break;
							}
							break;
						}
					}
				}

				if (GlTFPrimitive.attributes.find("JOINTS_1") != GlTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor&   Accessor = M_Model.accessors[GlTFPrimitive.attributes.find("JOINTS_1")->second];
					const tinygltf::BufferView& View     = M_Model.bufferViews[Accessor.bufferView];
					JointIndicesBuffer1                  = reinterpret_cast<const uint32_t *>(&(M_Model.buffers[View.buffer].data[Accessor.byteOffset + View.byteOffset]));

					for (std::size_t v = 0; v < VertexCount; v++) {
						Vertex& Vtx = LocalVertexBuffer[v];

						switch(Accessor.componentType)
						{
						case TINYGLTF_COMPONENT_TYPE_BYTE:
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointIndices1.x = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer1)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointIndices1.x = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer1)[(v*2)+0];
								Vtx.JointIndices1.y = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer1)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointIndices1.x = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer1)[(v*3)+0];
								Vtx.JointIndices1.y = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer1)[(v*3)+1];
								Vtx.JointIndices1.z = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer1)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointIndices1.x = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer1)[(v*4)+0];
								Vtx.JointIndices1.y = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer1)[(v*4)+1];
								Vtx.JointIndices1.z = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer1)[(v*4)+2];
								Vtx.JointIndices1.w = reinterpret_cast<const std::uint8_t*>(JointIndicesBuffer1)[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_SHORT:
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointIndices1.x = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer1)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointIndices1.x = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer1)[(v*2)+0];
								Vtx.JointIndices1.y = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer1)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointIndices1.x = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer1)[(v*3)+0];
								Vtx.JointIndices1.y = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer1)[(v*3)+1];
								Vtx.JointIndices1.z = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer1)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointIndices1.x = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer1)[(v*4)+0];
								Vtx.JointIndices1.y = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer1)[(v*4)+1];
								Vtx.JointIndices1.z = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer1)[(v*4)+2];
								Vtx.JointIndices1.w = reinterpret_cast<const std::uint16_t*>(JointIndicesBuffer1)[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_INT:
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointIndices1.x = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer1)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointIndices1.x = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer1)[(v*2)+0];
								Vtx.JointIndices1.y = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer1)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointIndices1.x = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer1)[(v*3)+0];
								Vtx.JointIndices1.y = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer1)[(v*3)+1];
								Vtx.JointIndices1.z = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer1)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointIndices1.x = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer1)[(v*4)+0];
								Vtx.JointIndices1.y = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer1)[(v*4)+1];
								Vtx.JointIndices1.z = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer1)[(v*4)+2];
								Vtx.JointIndices1.w = reinterpret_cast<const std::uint32_t*>(JointIndicesBuffer1)[(v*4)+3];
								break;
							}
							break;
						}
					}
				}

				if (GlTFPrimitive.attributes.find("WEIGHTS_1") != GlTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor&   Accessor = M_Model.accessors[GlTFPrimitive.attributes.find("WEIGHTS_1")->second];
					const tinygltf::BufferView& View     = M_Model.bufferViews[Accessor.bufferView];
					JointWeightsBuffer1                  = reinterpret_cast<const float *>(&(M_Model.buffers[View.buffer].data[Accessor.byteOffset + View.byteOffset]));

					for (std::size_t v = 0; v < VertexCount; v++) {
						Vertex& Vtx = LocalVertexBuffer[v];

						switch(Accessor.componentType)
						{
						case TINYGLTF_COMPONENT_TYPE_FLOAT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointWeights1.x = JointWeightsBuffer1[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointWeights1.x = JointWeightsBuffer1[(v*2)+0];
								Vtx.JointWeights1.y = JointWeightsBuffer1[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointWeights1.x = JointWeightsBuffer1[(v*3)+0];
								Vtx.JointWeights1.y = JointWeightsBuffer1[(v*3)+1];
								Vtx.JointWeights1.z = JointWeightsBuffer1[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointWeights1.x = JointWeightsBuffer1[(v*4)+0];
								Vtx.JointWeights1.y = JointWeightsBuffer1[(v*4)+1];
								Vtx.JointWeights1.z = JointWeightsBuffer1[(v*4)+2];
								Vtx.JointWeights1.w = JointWeightsBuffer1[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_DOUBLE:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointWeights1.x = reinterpret_cast<const double*>(JointWeightsBuffer1)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointWeights1.x = reinterpret_cast<const double*>(JointWeightsBuffer1)[(v*2)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const double*>(JointWeightsBuffer1)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointWeights1.x = reinterpret_cast<const double*>(JointWeightsBuffer1)[(v*3)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const double*>(JointWeightsBuffer1)[(v*3)+1];
								Vtx.JointWeights1.z = reinterpret_cast<const double*>(JointWeightsBuffer1)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointWeights1.x = reinterpret_cast<const double*>(JointWeightsBuffer1)[(v*4)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const double*>(JointWeightsBuffer1)[(v*4)+1];
								Vtx.JointWeights1.z = reinterpret_cast<const double*>(JointWeightsBuffer1)[(v*4)+2];
								Vtx.JointWeights1.w = reinterpret_cast<const double*>(JointWeightsBuffer1)[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_BYTE:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointWeights1.x = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer1)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointWeights1.x = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer1)[(v*2)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer1)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointWeights1.x = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer1)[(v*3)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer1)[(v*3)+1];
								Vtx.JointWeights1.z = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer1)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointWeights1.x = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer1)[(v*4)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer1)[(v*4)+1];
								Vtx.JointWeights1.z = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer1)[(v*4)+2];
								Vtx.JointWeights1.w = reinterpret_cast<const std::int8_t*>(JointWeightsBuffer1)[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointWeights1.x = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer1)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointWeights1.x = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer1)[(v*2)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer1)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointWeights1.x = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer1)[(v*3)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer1)[(v*3)+1];
								Vtx.JointWeights1.z = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer1)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointWeights1.x = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer1)[(v*4)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer1)[(v*4)+1];
								Vtx.JointWeights1.z = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer1)[(v*4)+2];
								Vtx.JointWeights1.w = reinterpret_cast<const std::uint8_t*>(JointWeightsBuffer1)[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_SHORT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointWeights1.x = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer1)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointWeights1.x = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer1)[(v*2)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer1)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointWeights1.x = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer1)[(v*3)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer1)[(v*3)+1];
								Vtx.JointWeights1.z = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer1)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointWeights1.x = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer1)[(v*4)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer1)[(v*4)+1];
								Vtx.JointWeights1.z = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer1)[(v*4)+2];
								Vtx.JointWeights1.w = reinterpret_cast<const std::int16_t*>(JointWeightsBuffer1)[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointWeights1.x = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer1)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointWeights1.x = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer1)[(v*2)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer1)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointWeights1.x = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer1)[(v*3)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer1)[(v*3)+1];
								Vtx.JointWeights1.z = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer1)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointWeights1.x = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer1)[(v*4)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer1)[(v*4)+1];
								Vtx.JointWeights1.z = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer1)[(v*4)+2];
								Vtx.JointWeights1.w = reinterpret_cast<const std::uint16_t*>(JointWeightsBuffer1)[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_INT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointWeights1.x = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer1)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointWeights1.x = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer1)[(v*2)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer1)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointWeights1.x = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer1)[(v*3)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer1)[(v*3)+1];
								Vtx.JointWeights1.z = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer1)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointWeights1.x = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer1)[(v*4)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer1)[(v*4)+1];
								Vtx.JointWeights1.z = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer1)[(v*4)+2];
								Vtx.JointWeights1.w = reinterpret_cast<const std::int32_t*>(JointWeightsBuffer1)[(v*4)+3];
								break;
							}
							break;
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
							switch(Accessor.type)
							{
							case TINYGLTF_TYPE_SCALAR:
								Vtx.JointWeights1.x = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer1)[v];
								break;
							case TINYGLTF_TYPE_VEC2:
								Vtx.JointWeights1.x = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer1)[(v*2)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer1)[(v*2)+1];
								break;
							case TINYGLTF_TYPE_VEC3:
								Vtx.JointWeights1.x = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer1)[(v*3)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer1)[(v*3)+1];
								Vtx.JointWeights1.z = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer1)[(v*3)+2];
								break;
							case TINYGLTF_TYPE_VEC4:
								Vtx.JointWeights1.x = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer1)[(v*4)+0];
								Vtx.JointWeights1.y = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer1)[(v*4)+1];
								Vtx.JointWeights1.z = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer1)[(v*4)+2];
								Vtx.JointWeights1.w = reinterpret_cast<const std::uint32_t*>(JointWeightsBuffer1)[(v*4)+3];
								break;
							}
							break;
						}
					}
				}

				std::copy(LocalVertexBuffer.begin(), LocalVertexBuffer.end(), std::back_inserter(HostVertexBuffer));
			}

			{
				const tinygltf::Accessor &  Accessor   = M_Model.accessors[GlTFPrimitive.indices];
				const tinygltf::BufferView& BufferView = M_Model.bufferViews[Accessor.bufferView];
				const tinygltf::Buffer&     Buffer     = M_Model.buffers[BufferView.buffer];

				IndexCount += static_cast<std::uint32_t>(Accessor.count);

				switch (Accessor.componentType)
				{
				case TINYGLTF_PARAMETER_TYPE_INT:
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
					const std::uint32_t* buf = reinterpret_cast<const std::uint32_t*>(&Buffer.data[Accessor.byteOffset + BufferView.byteOffset]);
					for (size_t index = 0; index < Accessor.count; index++)
					{
						HostIndexBuffer.push_back(buf[index] + VertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_SHORT:
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
					const std::uint16_t* buf = reinterpret_cast<const std::uint16_t*>(&Buffer.data[Accessor.byteOffset + BufferView.byteOffset]);
					for (std::size_t index = 0; index < Accessor.count; index++)
					{
						HostIndexBuffer.push_back(buf[index] + VertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_BYTE:
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
					const std::uint8_t* buf = reinterpret_cast<const std::uint8_t*>(&Buffer.data[Accessor.byteOffset + BufferView.byteOffset]);
					for (std::size_t index = 0; index < Accessor.count; index++)
					{
						HostIndexBuffer.push_back(buf[index] + VertexStart);
					}
					break;
				}
				}
			}

			Primitive primitive{};
			primitive.FirstIndex    = FirstIndex;
			primitive.IndexCount    = IndexCount;
			Node->Mesh.Primitives.push_back(primitive);
		}
	}

	if (NodeParent)
	{
		NodeParent->Children.push_back(Node);
	}
	else
	{
		M_Nodes.push_back(Node);
	}
}

void VkGltfModel::LoadSkins()
{
	M_Skins.resize(M_Model.skins.size());

	for (std::size_t i = 0; i < M_Model.skins.size(); i++)
	{
		tinygltf::Skin glTFSkin = M_Model.skins[i];

		M_Skins[i].Name = glTFSkin.name;
		M_Skins[i].SkeletonRoot = NodeFromIndex(glTFSkin.skeleton);


		for (int jointIndex : glTFSkin.joints)
		{
			std::shared_ptr<Node> Node = NodeFromIndex(jointIndex);
			if (Node)
			{
				M_Skins[i].Joints.push_back(Node);
			}
		}

		if (glTFSkin.inverseBindMatrices > -1)
		{
			const tinygltf::Accessor&   Accessor   = M_Model.accessors[glTFSkin.inverseBindMatrices];
			const tinygltf::BufferView& BufferView = M_Model.bufferViews[Accessor.bufferView];
			const tinygltf::Buffer&     Buffer     = M_Model.buffers[BufferView.buffer];
			M_Skins[i].InverseBindMatrices.resize(Accessor.count);
			std::memcpy(M_Skins[i].InverseBindMatrices.data(), &Buffer.data[Accessor.byteOffset + BufferView.byteOffset], Accessor.count * sizeof(DirectX::XMFLOAT4X4));

			M_Skins[i].Ssbo = CreateBuffer(vk::BufferUsageFlagBits::eStorageBuffer, sizeof(DirectX::XMFLOAT4X4) * M_Skins[i].InverseBindMatrices.size(), M_Skins[i].InverseBindMatrices.data());
		}
	}

	const vk::DescriptorPoolSize PoolSize(vk::DescriptorType::eStorageBuffer, M_Skins.size());
	const vk::DescriptorPoolCreateInfo DescriptorPoolCI = vk::DescriptorPoolCreateInfo({}, M_Skins.size(), 1, &PoolSize);
	M_SkinsDescriptorPool = G_Device.createDescriptorPool(DescriptorPoolCI, nullptr, G_DLD);


	for (std::size_t i = 0; i < M_Model.skins.size(); i++) {
		const vk::DescriptorSetAllocateInfo DescriptorSetAI = vk::DescriptorSetAllocateInfo(M_SkinsDescriptorPool, 1, &G_SkinsDescriptorSetLayout);
		M_Skins[i].DescriptorSet = G_Device.allocateDescriptorSets(DescriptorSetAI, G_DLD)[0];

		const vk::DescriptorBufferInfo DescriptorBI = vk::DescriptorBufferInfo(std::get<0>(M_Skins[i].Ssbo), 0, vk::WholeSize);
		const vk::WriteDescriptorSet Write = vk::WriteDescriptorSet(M_Skins[i].DescriptorSet, 0, 0, 1,vk::DescriptorType::eStorageBuffer, nullptr, &DescriptorBI, nullptr);
		G_Device.updateDescriptorSets(Write, nullptr, G_DLD);
	}

}

void VkGltfModel::LoadAnimations()
{
	M_Animations.resize(M_Model.animations.size());

	for (std::size_t i = 0; i < M_Model.animations.size(); i++)
	{
		tinygltf::Animation GltfAnimation = M_Model.animations[i];
		M_Animations[i].Name              = GltfAnimation.name;

		M_Animations[i].Samplers.resize(GltfAnimation.samplers.size());
		for (size_t j = 0; j < GltfAnimation.samplers.size(); j++)
		{
			tinygltf::AnimationSampler GlTFSampler = GltfAnimation.samplers[j];
			AnimationSampler &         DstSampler  = M_Animations[i].Samplers[j];
			DstSampler.Interpolation               = GlTFSampler.interpolation;

			{
				const tinygltf::Accessor&   Accessor   = M_Model.accessors[GlTFSampler.input];
				const tinygltf::BufferView& BufferView = M_Model.bufferViews[Accessor.bufferView];
				const tinygltf::Buffer &    Buffer     = M_Model.buffers[BufferView.buffer];
				const void *                DataPtr    = &Buffer.data[Accessor.byteOffset + BufferView.byteOffset];
				const float *               Buf        = static_cast<const float *>(DataPtr);
				for (std::size_t Index = 0; Index < Accessor.count; Index++)
				{
					DstSampler.Inputs.push_back(Buf[Index]);
				}
				for (auto input : M_Animations[i].Samplers[j].Inputs)
				{
					if (input < M_Animations[i].Start)
					{
						M_Animations[i].Start = input;
					};
					if (input > M_Animations[i].End)
					{
						M_Animations[i].End = input;
					}
				}
			}

			{
				const tinygltf::Accessor&   Accessor   = M_Model.accessors[GlTFSampler.output];
				const tinygltf::BufferView& BufferView = M_Model.bufferViews[Accessor.bufferView];
				const tinygltf::Buffer &    Buffer     = M_Model.buffers[BufferView.buffer];
				const void *                DataPtr    = &Buffer.data[Accessor.byteOffset + BufferView.byteOffset];
				switch (Accessor.type)
				{
				case TINYGLTF_TYPE_SCALAR:
				{
					const float *Buf = static_cast<const float*>(DataPtr);
					for (size_t Index = 0; Index < Accessor.count; Index++)
					{
						DstSampler.OutputsVec4.push_back(DirectX::XMFLOAT4(Buf[Index], 0.0f, 0.0f, 0.0f));
					}
				}
				break;
				case TINYGLTF_TYPE_VEC2:
				{
					const DirectX::XMFLOAT2 *Buf = static_cast<const DirectX::XMFLOAT2*>(DataPtr);
					for (size_t Index = 0; Index < Accessor.count; Index++)
					{
						DstSampler.OutputsVec4.push_back(DirectX::XMFLOAT4(Buf[Index].x, Buf[Index].y, 0.0f, 0.0f));
					}
				}
				break;
				case TINYGLTF_TYPE_VEC3:
				{
					const DirectX::XMFLOAT3 *Buf = static_cast<const DirectX::XMFLOAT3*>(DataPtr);
					for (size_t Index = 0; Index < Accessor.count; Index++)
					{
						DstSampler.OutputsVec4.push_back(DirectX::XMFLOAT4(Buf[Index].x, Buf[Index].y, Buf[Index].z, 0.0f));
					}
				}
				break;
				case TINYGLTF_TYPE_VEC4:
				{
					const DirectX::XMFLOAT4 *Buf = static_cast<const DirectX::XMFLOAT4*>(DataPtr);
					for (size_t Index = 0; Index < Accessor.count; Index++)
					{
						DstSampler.OutputsVec4.push_back(Buf[Index]);
					}
				}
				break;
				}
			}
		}

		M_Animations[i].Channels.resize(GltfAnimation.channels.size());
		for (size_t j = 0; j < GltfAnimation.channels.size(); j++)
		{
			tinygltf::AnimationChannel GltfChannel = GltfAnimation.channels[j];
			AnimationChannel&          DstChannel  = M_Animations[i].Channels[j];
			DstChannel.Path                        = GltfChannel.target_path;
			DstChannel.SamplerIndex                = GltfChannel.sampler;
			DstChannel.Node                        = NodeFromIndex(GltfChannel.target_node);
		}
	}
}

std::shared_ptr<VkGltfModel::Node> VkGltfModel::FindNode(std::shared_ptr<Node> Parent, std::uint32_t Index) const
{
	std::shared_ptr<Node> NodeFound = nullptr;
	if (Parent->Index == Index)
	{
		return Parent;
	}
	for (auto &Child : Parent->Children)
	{
		NodeFound = FindNode(Child, Index);
		if (NodeFound)
		{
			break;
		}
	}
	return NodeFound;
}

std::shared_ptr<VkGltfModel::Node> VkGltfModel::NodeFromIndex(std::uint32_t Index) const
{
	std::shared_ptr<Node> NodeFound = nullptr;
	for (auto &Node : M_Nodes)
	{
		NodeFound = FindNode(Node, Index);
		if (NodeFound)
		{
			break;
		}
	}
	return NodeFound;
}

DirectX::XMMATRIX VkGltfModel::GetNodeMatrix(std::shared_ptr<Node> Node)
{

	DirectX::XMMATRIX                  NodeMatrix = Node->GetLocalMatrix();
	std::shared_ptr<VkGltfModel::Node> CurrentParent = Node->Parent;
	while (CurrentParent)
	{
		NodeMatrix    = DirectX::XMMatrixMultiply(NodeMatrix, CurrentParent->GetLocalMatrix()) ;
		CurrentParent = CurrentParent->Parent;
	}
	return NodeMatrix;
}

void VkGltfModel::UpdateJoints(std::shared_ptr<Node> Node)
{
	if (Node->Skin > -1)
	{
		const DirectX::XMMATRIX InverseTransform = DirectX::XMMatrixInverse(nullptr, GetNodeMatrix(Node));
		const Skin&             NodeSkin         = M_Skins[Node->Skin];
		const std::size_t       NumJoints        = NodeSkin.Joints.size();

		std::vector<DirectX::XMFLOAT4X4> JointMatrices(NumJoints);
		for (std::size_t i = 0; i < NumJoints; i++)
		{
			DirectX::XMMATRIX JointMatrix = DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&NodeSkin.InverseBindMatrices[i]), GetNodeMatrix(NodeSkin.Joints[i]));
			JointMatrix = DirectX::XMMatrixMultiply(JointMatrix, InverseTransform);
			DirectX::XMStoreFloat4x4(&JointMatrices[i], JointMatrix);
		}

		const vk::DeviceSize ByteSize = JointMatrices.size() * sizeof(DirectX::XMFLOAT4X4);
		DirectX::XMFLOAT4X4* Mapped = reinterpret_cast<DirectX::XMFLOAT4X4*>(G_Device.mapMemory(std::get<1>(NodeSkin.Ssbo), 0, ByteSize, vk::MemoryMapFlags(), G_DLD));
		std::memcpy(Mapped, JointMatrices.data(), ByteSize);
		G_Device.unmapMemory(std::get<1>(NodeSkin.Ssbo), G_DLD);
	}

	for (auto &Child : Node->Children)
	{
		UpdateJoints(Child);
	}
}

void VkGltfModel::UpdateAnimation(float DeltaTime)
{
	Animation &Anim = M_Animations[0];
	Anim.CurrentTime += DeltaTime;
	while (Anim.CurrentTime > Anim.End)
	{
		Anim.CurrentTime -= (Anim.End - Anim.Start);
	}

	for (auto &Channel : Anim.Channels)
	{
		AnimationSampler &Sampler = Anim.Samplers[Channel.SamplerIndex];

//		if (Sampler.Interpolation != "LINEAR")
//		{
//			std::cout << "This sample only supports linear interpolations\n";
//			continue;
//		}

		for (std::size_t i = 0; i < Sampler.Inputs.size() - 1; i++)
		{
			if ((Anim.CurrentTime >= Sampler.Inputs[i]) && (Anim.CurrentTime <= Sampler.Inputs[i + 1]))
			{
				const float a = (Anim.CurrentTime - Sampler.Inputs[i]) / (Sampler.Inputs[i + 1] - Sampler.Inputs[i]);
				if (Channel.Path == "translation")
				{
					const DirectX::XMVECTOR TranslationVec = DirectX::XMVectorLerp(DirectX::XMLoadFloat4(&Sampler.OutputsVec4[i]), DirectX::XMLoadFloat4(&Sampler.OutputsVec4[i + 1]), a);
					DirectX::XMStoreFloat3(&Channel.Node->Translation, TranslationVec);

				}
				if (Channel.Path == "rotation")
				{
					const DirectX::XMVECTOR q1 = DirectX::XMVectorSet(Sampler.OutputsVec4[i].x, Sampler.OutputsVec4[i].y, Sampler.OutputsVec4[i].z, Sampler.OutputsVec4[i].w);
					const DirectX::XMVECTOR q2 = DirectX::XMVectorSet(Sampler.OutputsVec4[i+1].x, Sampler.OutputsVec4[i+1].y, Sampler.OutputsVec4[i+1].z, Sampler.OutputsVec4[i+1].w);

					const DirectX::XMVECTOR RotationVec = DirectX::XMVector4Normalize(DirectX::XMQuaternionSlerp(q1, q2, a));
					DirectX::XMStoreFloat4(&Channel.Node->Rotation, RotationVec);
				}
				if (Channel.Path == "scale")
				{
					const DirectX::XMVECTOR ScaleVec = DirectX::XMVectorLerp(DirectX::XMLoadFloat4(&Sampler.OutputsVec4[i]), DirectX::XMLoadFloat4(&Sampler.OutputsVec4[i + 1]), a);
					DirectX::XMStoreFloat3(&Channel.Node->Scale, ScaleVec);
				}

				break;
			}
		}
	}

	for (auto &Node : M_Nodes)
	{
		UpdateJoints(Node);
	}
}


void VkGltfModel::Shutdown()
{
	if (G_Device) {

		if (M_SkinsDescriptorPool) {
			G_Device.destroyDescriptorPool(M_SkinsDescriptorPool, nullptr, G_DLD);
			M_SkinsDescriptorPool = nullptr;
		}

		for (std::size_t i = 0; i < M_Skins.size(); i++) {
			if (std::get<1>(M_Skins[i].Ssbo)) {
				G_Device.freeMemory(std::get<1>(M_Skins[i].Ssbo), nullptr, G_DLD);
				std::get<1>(M_Skins[i].Ssbo) = nullptr;
			}
			if (std::get<0>(M_Skins[i].Ssbo)) {
				G_Device.destroyBuffer(std::get<0>(M_Skins[i].Ssbo), nullptr, G_DLD);
				std::get<0>(M_Skins[i].Ssbo) = nullptr;
			}
		}

		if (std::get<1>(M_IndexBufferTuple)) {
			G_Device.freeMemory(std::get<1>(M_IndexBufferTuple), nullptr, G_DLD);
			std::get<1>(M_IndexBufferTuple) = nullptr;
		}
		if (std::get<1>(M_VertexBufferTuple)) {
			G_Device.freeMemory(std::get<1>(M_VertexBufferTuple), nullptr, G_DLD);
			std::get<1>(M_VertexBufferTuple) = nullptr;
		}
		if (std::get<0>(M_IndexBufferTuple)) {
			G_Device.destroyBuffer(std::get<0>(M_IndexBufferTuple), nullptr, G_DLD);
			std::get<0>(M_IndexBufferTuple) = nullptr;
		}
		if (std::get<0>(M_VertexBufferTuple)) {
			G_Device.destroyBuffer(std::get<0>(M_VertexBufferTuple), nullptr, G_DLD);
			std::get<0>(M_VertexBufferTuple) = nullptr;
		}
	}
}








