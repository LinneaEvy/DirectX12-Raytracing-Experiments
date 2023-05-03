
#pragma once

// DirectX 12 specific headers.
#include "d3dx12.h"

#include <dxgi1_6.h>

#include <d3dcompiler.h>
#include "RaytracingSceneDefines.h"
#include <DirectXMath.h>



//memory management

#include <wrl.h>
#include <memory>

// STL Headers

#include <algorithm>
#include <cassert>
#include <chrono>


// Helper functions

#include "ChiliException.h"


#include <dxcapi.h>
#include <vector>


#define SizeOfInUint32(obj) ((sizeof(obj) - 1) / sizeof(UINT32) + 1)


#define MAX_RAY_RECURSION_DEPTH 3  



const wchar_t* c_hitGroupNames_TriangleGeometry[] =
{
    L"MyHitGroup_Triangle", L"MyHitGroup_Triangle_ShadowRay"
};
const wchar_t* c_hitGroupNames_AABBGeometry[][RayType::Count] =
{
    { L"MyHitGroup_AABB_AnalyticPrimitive", L"MyHitGroup_AABB_AnalyticPrimitive_ShadowRay" },
    { L"MyHitGroup_AABB_VolumetricPrimitive", L"MyHitGroup_AABB_VolumetricPrimitive_ShadowRay" },
    { L"MyHitGroup_AABB_SignedDistancePrimitive", L"MyHitGroup_AABB_SignedDistancePrimitive_ShadowRay" },
};
const wchar_t* c_raygenShaderName = L"MyRaygenShader";
const wchar_t* c_intersectionShaderNames[] =
{
    L"MyIntersectionShader_AnalyticPrimitive",
    L"MyIntersectionShader_VolumetricPrimitive",
    L"MyIntersectionShader_SignedDistancePrimitive",
};


// Attributes per primitive type.
struct PrimitiveConstantBuffer
{
    XMFLOAT4 albedo;
    float reflectanceCoef;
    float diffuseCoef;
    float specularCoef;
    float specularPower;
    float stepScale;                      // Step scale for ray marching of signed distance primitives. 
                                          // - Some object transformations don't preserve the distances and 
                                          //   thus require shorter steps.
    XMFLOAT3 padding;
};

// Attributes per primitive instance.
struct PrimitiveInstanceConstantBuffer
{
    UINT instanceIndex;
    UINT primitiveType; // Procedural primitive type
};

// Dynamic attributes per primitive instance.
struct PrimitiveInstancePerFrameBuffer
{
    XMMATRIX localSpaceToBottomLevelAS;   // Matrix from local primitive space to bottom-level object space.
    XMMATRIX bottomLevelASToLocalSpace;   // Matrix from bottom-level object space to local primitive space.
};




using namespace DirectX;
using namespace Microsoft::WRL;

class Graphics
{
public:
    class Exception : public ChiliException
    {
        using ChiliException::ChiliException;//he felt too lazy to write 
    };
    class HrException : public Exception
    {
    public:
        HrException(int line, const char* file, HRESULT hr, std::vector<std::string> infoMsgs = {}) noexcept;
        const char* what() const noexcept override;
        const char* GetType() const noexcept override;
        HRESULT GetErrorCode() const noexcept;
        std::string GetErrorString() const noexcept;
        std::string GetErrorDescription() const noexcept;
        std::string GetErrorInfo() const noexcept;
        inline void ThrowIfFailed(HRESULT hr);
    private:
        HRESULT hr;
        std::string info;
    };
    class InfoException : public Exception
    {
    public:
        InfoException(int line, const char* file, std::vector<std::string> infoMsgs) noexcept;
        const char* what() const noexcept override;
        const char* GetType() const noexcept override;
        std::string GetErrorInfo() const noexcept;
    private:
        std::string info;
    };
    class DeviceRemovedException : public HrException
    {
        using HrException::HrException;
    public:
        const char* GetType() const noexcept override;
    private:
        std::string reason;
    };
    //class RayTracing {
        struct AccelerationStructureBuffers
        {
            ComPtr<ID3D12Resource> pScratch; // Scratch memory for AS builder 
            ComPtr<ID3D12Resource> pResult; // Where the AS is 
            ComPtr<ID3D12Resource> pInstanceDesc; // Hold the matrices of the instances
        };
        ComPtr<ID3D12Resource> m_bottomLevelAS; // Storage for the bottom Level AS
        nv_helpers_dx12::TopLevelASGenerator m_topLevelASGenerator;
        AccelerationStructureBuffers m_topLevelASBuffers;
        std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>> m_instances;
        AccelerationStructureBuffers CreateBottomLevelAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers);
        void CreateTopLevelAS(const std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances);
        void CreateAccelerationStructures();
        void CreateRaytracingInterfaces();
        void SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig);
        void CreateAuxilaryDeviceResources();
        void CreateRootSignatures();
        void CreateHitGroupSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline);
        void CreateDxilLibrarySubobject(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline);
        void CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline);
        void CreateRaytracingPipelineStateObject();
        void CreateDescriptorHeap();
    //};


    Graphics(HWND hWnd, UINT width, UINT height, std::wstring name);
    Graphics(const Graphics&) = delete;
    Graphics& operator=(const Graphics&) = delete;
    ~Graphics();


    void EnableDebugLayer();
    ComPtr<IDXGIAdapter1> GetAdapter(bool useWarp);
    ComPtr<ID3D12Device5> CreateDevice(ComPtr<IDXGIAdapter1> adapter);
    ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device5> device, D3D12_COMMAND_LIST_TYPE type);
    bool CheckTearingSupport();
    ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd,
        ComPtr<ID3D12CommandQueue> commandQueue,
        uint32_t width, uint32_t height, uint32_t bufferCount);
    ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device5> device,
        D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
    void UpdateRenderTargetViews(ComPtr<ID3D12Device5> device,
        ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap);
    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device5> device,
        D3D12_COMMAND_LIST_TYPE type);
    ComPtr<ID3D12GraphicsCommandList5> CreateCommandList(ComPtr<ID3D12Device5> device,
        ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type);
    ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device5> device);
    HANDLE CreateEventHandle();
    uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence,
        uint64_t& fenceValue);
    void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration = std::chrono::milliseconds::max());
    void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence,
        uint64_t& fenceValue, HANDLE fenceEvent);
    void Resize(uint32_t width, uint32_t height);
    void SetFullscreen(bool fullscreen);

    void loadPipeline();

    void Update();
    void Render();

private:

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT4 color;
    };
    //weird stuff
    static const uint8_t g_NumFrames = 3;//num of backbuffers
    bool g_UseWarp = false;// Use WARP adapter
    uint32_t g_ClientWidth = 1280;
    uint32_t g_ClientHeight = 720;
    bool g_IsInitialized = false;// Set to true once the DX12 objects have been initialized.

    //move this to window function
    HWND g_hWnd;// Window rectangle (used to toggle fullscreen state).
    RECT g_WindowRect;

    //Pipeline assets:

    ComPtr<ID3D12Device5> g_Device;
    ComPtr<ID3D12Device> g_GIDevice;
    ComPtr<ID3D12CommandQueue> g_CommandQueue;
    ComPtr<IDXGISwapChain4> g_SwapChain;
    ComPtr<ID3D12Resource> g_BackBuffers[g_NumFrames];
    ComPtr<ID3D12GraphicsCommandList5> g_CommandList;
    ComPtr<ID3D12GraphicsCommandList> g_GICommandList;
    ComPtr<ID3D12CommandAllocator> g_CommandAllocators[g_NumFrames];
    ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
    UINT g_RTVDescriptorSize;
    UINT g_CurrentBackBufferIndex;
    ComPtr<ID3D12PipelineState> g_pipelineState;
    ComPtr<ID3D12Resource> g_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW g_vertexBufferView;
    float g_aspectRatio = 10;
    // Root signatures
    ComPtr<ID3D12RootSignature> g_raytracingGlobalRootSignature;
    ComPtr<ID3D12RootSignature> g_raytracingLocalRootSignature[LocalRootSignature::Type::Count];

    ComPtr<ID3D12Fence> g_Fence;
    uint64_t g_FenceValue = 0;
    uint64_t g_FrameFenceValues[g_NumFrames] = {};
    HANDLE g_FenceEvent;


    // By default, enable V-Sync.
    // Can be toggled with the V key.
    bool g_VSync = true;
    bool g_TearingSupported = false;// By default, use windowed mode.// Can be toggled with the Alt+Enter or F11
    bool g_Fullscreen = false;
    static const wchar_t* c_closestHitShaderNames[GeometryType::Count];
    void CheckRaytracingSupport();

    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList();
    void WaitForPreviousFrame();
};