
#pragma once

#include <unordered_map>
#include "d3dx12.h"

#include <dxgi1_6.h>

#include <d3dcompiler.h>

#include <DirectXMath.h>
#include "RaytracingSceneDefines.h"

#include <wrl.h>
#include <memory>

// STL Headers

#include <algorithm>
#include <cassert>
#include <chrono>


// Helper functions

#include "ChiliException.h"

// Ray types traced in this sample.


//memory management




#include <dxcapi.h>
#include <vector>
//#include "StepTimer.h"
#include "PerformanceTimers.h"
#include "TopLevelASGenerator.h"

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

    class GpuUploadBuffer
    {
    public:
        ComPtr<ID3D12Resource> GetResource() { return m_resource; }

    protected:
        ComPtr<ID3D12Resource> m_resource;

        GpuUploadBuffer() {}
        ~GpuUploadBuffer()
        {
            if (m_resource.Get())
            {
                m_resource->Unmap(0, nullptr);
            }
        }

        void Allocate(ID3D12Device* device, UINT bufferSize, LPCWSTR resourceName = nullptr);
        

        uint8_t* MapCpuWriteOnly();
    };
    // DirectX 12 specific headers.
    template <class T>
    class ConstantBuffer : public GpuUploadBuffer
    {
        uint8_t* m_mappedConstantData;
        UINT m_alignedInstanceSize;
        UINT m_numInstances;

    public:
        ConstantBuffer() : m_alignedInstanceSize(0), m_numInstances(0), m_mappedConstantData(nullptr) {}

        void Create(ID3D12Device* device, UINT numInstances = 1, LPCWSTR resourceName = nullptr)
        {
            m_numInstances = numInstances;
            m_alignedInstanceSize = Align(sizeof(T), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
            UINT bufferSize = numInstances * m_alignedInstanceSize;
            Allocate(device, bufferSize, resourceName);
            m_mappedConstantData = MapCpuWriteOnly();
        }

        void CopyStagingToGpu(UINT instanceIndex = 0)
        {
            memcpy(m_mappedConstantData + instanceIndex * m_alignedInstanceSize, &staging, sizeof(T));
        }

        // Accessors
        T staging;
        T* operator->() { return &staging; }
        UINT NumInstances() { return m_numInstances; }
        D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress(UINT instanceIndex = 0)
        {
            return m_resource->GetGPUVirtualAddress() + instanceIndex * m_alignedInstanceSize;
        }
    };


    // Helper class to create and update a structured buffer.
    // Usage: 
    //    StructuredBuffer<...> sb;
    //    sb.Create(...);
    //    sb[index].var = ... ; 
    //    sb.CopyStagingToGPU(...);
    //    Set...View(..., sb.GputVirtualAddress());
    template <class T>
    class StructuredBuffer : public GpuUploadBuffer
    {
        T* m_mappedBuffers;
        std::vector<T> m_staging;
        UINT m_numInstances;

    public:
        // Performance tip: Align structures on sizeof(float4) boundary.
        // Ref: https://developer.nvidia.com/content/understanding-structured-buffer-performance
        static_assert(sizeof(T) % 16 == 0, L"Align structure buffers on 16 byte boundary for performance reasons.");

        StructuredBuffer() : m_mappedBuffers(nullptr), m_numInstances(0) {}

        void Create(ID3D12Device* device, UINT numElements, UINT numInstances = 1, LPCWSTR resourceName = nullptr)
        {
            m_staging.resize(numElements);
            UINT bufferSize = numInstances * numElements * sizeof(T);
            Allocate(device, bufferSize, resourceName);
            m_mappedBuffers = reinterpret_cast<T*>(MapCpuWriteOnly());
        }

        void CopyStagingToGpu(UINT instanceIndex = 0)
        {
            memcpy(m_mappedBuffers + instanceIndex * NumElementsPerInstance(), &m_staging[0], InstanceSize());
        }

        // Accessors
        T& operator[](UINT elementIndex) { return m_staging[elementIndex]; }
        size_t NumElementsPerInstance() { return m_staging.size(); }
        UINT NumInstances() { return m_staging.size(); }
        size_t InstanceSize() { return NumElementsPerInstance() * sizeof(T); }
        D3D12_GPU_VIRTUAL_ADDRESS GpuVirtualAddress(UINT instanceIndex = 0)
        {
            return m_resource->GetGPUVirtualAddress() + instanceIndex * InstanceSize();
        }
    };
    class ShaderRecord
    {
    public:
        ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSize) :
            shaderIdentifier(pShaderIdentifier, shaderIdentifierSize)
        {
        }

        ShaderRecord(void* pShaderIdentifier, UINT shaderIdentifierSize, void* pLocalRootArguments, UINT localRootArgumentsSize) :
            shaderIdentifier(pShaderIdentifier, shaderIdentifierSize),
            localRootArguments(pLocalRootArguments, localRootArgumentsSize)
        {
        }

        void CopyTo(void* dest) const
        {
            uint8_t* byteDest = static_cast<uint8_t*>(dest);
            memcpy(byteDest, shaderIdentifier.ptr, shaderIdentifier.size);
            if (localRootArguments.ptr)
            {
                memcpy(byteDest + shaderIdentifier.size, localRootArguments.ptr, localRootArguments.size);
            }
        }

        struct PointerWithSize {
            void* ptr;
            UINT size;

            PointerWithSize() : ptr(nullptr), size(0) {}
            PointerWithSize(void* _ptr, UINT _size) : ptr(_ptr), size(_size) {};
        };
        PointerWithSize shaderIdentifier;
        PointerWithSize localRootArguments;
    };
    class ShaderTable : public GpuUploadBuffer
    {
        uint8_t* m_mappedShaderRecords;
        UINT m_shaderRecordSize;

        // Debug support
        std::wstring m_name;
        std::vector<ShaderRecord> m_shaderRecords;

        ShaderTable() {}
    public:
        inline UINT Align(UINT size, UINT alignment)
        {
            return (size + (alignment - 1)) & ~(alignment - 1);
        }
        ShaderTable(ID3D12Device* device, UINT numShaderRecords, UINT shaderRecordSize, LPCWSTR resourceName = nullptr)
            : m_name(resourceName)
        {
            m_shaderRecordSize = Align(shaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
            m_shaderRecords.reserve(numShaderRecords);
            UINT bufferSize = numShaderRecords * m_shaderRecordSize;
            Allocate(device, bufferSize, resourceName);
            m_mappedShaderRecords = MapCpuWriteOnly();
        }

        void push_back(const ShaderRecord& shaderRecord)
        {
            //ThrowIfFalse(m_shaderRecords.size() < m_shaderRecords.capacity());
            m_shaderRecords.push_back(shaderRecord);
            shaderRecord.CopyTo(m_mappedShaderRecords);
            m_mappedShaderRecords += m_shaderRecordSize;
        }

        UINT GetShaderRecordSize() { return m_shaderRecordSize; }

        // Pretty-print the shader records.
        void DebugPrint(std::unordered_map<void*, std::wstring> shaderIdToStringMap)
        {
            std::wstringstream wstr;
            wstr << L"|--------------------------------------------------------------------\n";
            wstr << L"|Shader table - " << m_name.c_str() << L": "
                << m_shaderRecordSize << L" | "
                << m_shaderRecords.size() * m_shaderRecordSize << L" bytes\n";

            for (UINT i = 0; i < m_shaderRecords.size(); i++)
            {
                wstr << L"| [" << i << L"]: ";
                wstr << shaderIdToStringMap[m_shaderRecords[i].shaderIdentifier.ptr] << L", ";
                wstr << m_shaderRecords[i].shaderIdentifier.size << L" + " << m_shaderRecords[i].localRootArguments.size << L" bytes \n";
            }
            wstr << L"|--------------------------------------------------------------------\n";
            wstr << L"\n";
            OutputDebugStringW(wstr.str().c_str());
        }
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
    void CreateConstantBuffers();
    void CreateAABBPrimitiveAttributesBuffers();
    void BuildShaderTables();
    UINT AllocateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE* cpuDescriptor, UINT descriptorIndexToUse);
    void CreateRaytracingOutputResource();
    //};


    Graphics(HWND hWnd, UINT width, UINT height, std::wstring name);
    Graphics(const Graphics&) = delete;
    Graphics& operator=(const Graphics&) = delete;
    ~Graphics();

#undef max
#undef min

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
    
    UINT g_descriptorsAllocated;
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

    ConstantBuffer<SceneConstantBuffer> g_sceneCB;
    StructuredBuffer<PrimitiveInstancePerFrameBuffer> g_aabbPrimitiveAttributeBuffer;
    std::vector<D3D12_RAYTRACING_AABB> g_aabbs;

    ComPtr<ID3D12Resource> g_missShaderTable;
    UINT g_missShaderTableStrideInBytes;
    ComPtr<ID3D12Resource> g_hitGroupShaderTable;
    UINT g_hitGroupShaderTableStrideInBytes;
    ComPtr<ID3D12Resource> g_rayGenShaderTable;

    // Root constants
    PrimitiveConstantBuffer m_planeMaterialCB;
    PrimitiveConstantBuffer m_aabbMaterialCB[IntersectionShaderType::TotalPrimitiveCount];

    // Raytracing output
    ComPtr<ID3D12Resource> m_raytracingOutput;
    D3D12_GPU_DESCRIPTOR_HANDLE m_raytracingOutputResourceUAVGpuDescriptor;
    UINT m_raytracingOutputResourceUAVDescriptorHeapIndex;

    DXGI_FORMAT BackBufferFormat;
    static const wchar_t* c_missShaderNames[RayType::Count];


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