/*#include "Renderer.h"
#pragma once
#include "Graphics.h"
#include "dxerr.h"
#include "GraphicsError.h"
#include <tchar.h>
#include <Winuser.h>

#include "DXRHelper.h"
#include "BottomLevelASGenerator.h"
// Graphics exception stuff
Renderer::HrException::HrException(int line, const char* file, HRESULT hr, std::vector<std::string> infoMsgs) noexcept
    :
    Exception(line, file),
    hr(hr)
{
    // join all info messages with newlines into single string
    for (const auto& m : infoMsgs)
    {
        info += m;
        info.push_back('\n');
    }
    // remove final newline if exists
    if (!info.empty())
    {
        info.pop_back();
    }
}

const char* Renderer::HrException::what() const noexcept
{
    std::ostringstream oss;
    oss << GetType() << std::endl
        << "[Error Code] 0x" << std::hex << std::uppercase << GetErrorCode()
        << std::dec << " (" << (unsigned long)GetErrorCode() << ")" << std::endl
        << "[Error String] " << GetErrorString() << std::endl
        << "[Description] " << GetErrorDescription() << std::endl;
    if (!info.empty())
    {
        oss << "\n[Error Info]\n" << GetErrorInfo() << std::endl << std::endl;
    }
    oss << GetOriginString();
    whatBuffer = oss.str();
    return whatBuffer.c_str();
}

const char* Renderer::HrException::GetType() const noexcept
{
    return "Chili Graphics Exception";
}

HRESULT Renderer::HrException::GetErrorCode() const noexcept
{
    return hr;
}


std::string Renderer::HrException::GetErrorString() const noexcept
{
    return ToNarrow(DXGetErrorString(hr));
}


std::string Renderer::HrException::GetErrorDescription() const noexcept
{
    char buf[512];
    DXGetErrorDescriptionA(hr, buf, sizeof(buf));
    return buf;
}

std::string Renderer::HrException::GetErrorInfo() const noexcept
{
    return info;
}


const char* Renderer::DeviceRemovedException::GetType() const noexcept
{
    return "Chili Graphics Exception [Device Removed] (DXGI_ERROR_DEVICE_REMOVED)";
}

Renderer::InfoException::InfoException(int line, const char* file, std::vector<std::string> infoMsgs) noexcept
    :
    Exception(line, file)
{
    // join all info messages with newlines into single string
    for (const auto& m : infoMsgs)
    {
        info += m;
        info.push_back('\n');
    }
    // remove final newline if exists
    if (!info.empty())
    {
        info.pop_back();
    }
}

const char* Renderer::InfoException::what() const noexcept
{
    std::ostringstream oss;
    oss << GetType() << std::endl
        << "\n[Error Info]\n" << GetErrorInfo() << std::endl << std::endl;
    oss << GetOriginString();
    whatBuffer = oss.str();
    return whatBuffer.c_str();
}

const char* Renderer::InfoException::GetType() const noexcept
{
    return "Chili Graphics Info Exception";
}

std::string Renderer::InfoException::GetErrorInfo() const noexcept
{
    return info;
}



Renderer::Renderer(HWND hWnd, UINT width, UINT height, std::wstring name)
{

    EnableDebugLayer();
    g_TearingSupported = true;
    g_hWnd = hWnd;
    g_ClientWidth = width;
    g_ClientHeight = height;
    loadPipeline();
}

Renderer::~Renderer()
{
}

void Renderer::EnableDebugLayer()
{
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugInterface;
    FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))) >> chk;
    debugInterface->EnableDebugLayer();
#endif
}



void Renderer::Update()
{
    static uint64_t frameCounter = 0;
    static double elapsedSeconds = 0.0;
    static std::chrono::high_resolution_clock clock;
    static auto t0 = clock.now();

    frameCounter++;
    auto t1 = clock.now();
    auto deltaTime = t1 - t0;
    t0 = t1;
    elapsedSeconds += deltaTime.count() * 1e-9;
    if (elapsedSeconds > 1.0)
    {
        TCHAR buffer[500];
        auto fps = frameCounter / elapsedSeconds;
        _stprintf_s(buffer, 500, _T("FPS: %f\n"), fps);
        OutputDebugString(buffer);

        frameCounter = 0;
        elapsedSeconds = 0.0;
    }
}

void Renderer::Render()
{
    auto commandAllocator = g_CommandAllocators[g_CurrentBackBufferIndex];
    auto backBuffer = g_BackBuffers[g_CurrentBackBufferIndex];

    commandAllocator->Reset();
    g_CommandList->Reset(commandAllocator.Get(), nullptr);
    // Clear the render target.
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

        g_CommandList->ResourceBarrier(1, &barrier);
        FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
            g_CurrentBackBufferIndex, g_RTVDescriptorSize);

        g_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
    }
    // Present
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffer.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        g_CommandList->ResourceBarrier(1, &barrier);

        g_CommandList->Close() >> chk;

        ID3D12CommandList* const commandLists[] = {
            g_CommandList.Get()
        };
        g_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

        UINT syncInterval = g_VSync ? 1 : 0;
        UINT presentFlags = g_TearingSupported && !g_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    }
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
    if (g_ClientWidth != width || g_ClientHeight != height)
    {
        // Don't allow 0 size swap chain back buffers.
        g_ClientWidth = std::max(1u, width);
        g_ClientHeight = std::max(1u, height);

        // Flush the GPU queue to make sure the swap chain's back buffers
        // are not being referenced by an in-flight command list.
        Flush(g_CommandQueue, g_Fence, g_FenceValue, g_FenceEvent);
        for (int i = 0; i < g_NumFrames; ++i)
        {
            // Any references to the back buffers must be released
            // before the swap chain can be resized.
            g_BackBuffers[i].Reset();
            g_FrameFenceValues[i] = g_FrameFenceValues[g_CurrentBackBufferIndex];
        }
        DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
        g_SwapChain->GetDesc(&swapChainDesc) >> chk;
        g_SwapChain->ResizeBuffers(g_NumFrames, g_ClientWidth, g_ClientHeight,
            swapChainDesc.BufferDesc.Format, swapChainDesc.Flags) >> chk;

        g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

        UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap);
    }
}

void Renderer::SetFullscreen(bool fullscreen)
{

    fullscreen = !g_Fullscreen;
    if (g_Fullscreen != fullscreen)
    {
        g_Fullscreen = fullscreen;

        if (g_Fullscreen) // Switching to fullscreen.
        {
            // Store the current window dimensions so they can be restored 
            // when switching out of fullscreen state.
            ::GetWindowRect(g_hWnd, &g_WindowRect);
            // Set the window style to a borderless window so the client area fills
           // the entire screen.
            UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

            ::SetWindowLongW(g_hWnd, GWL_STYLE, windowStyle);
            // Query the name of the nearest display device for the window.
      // This is required to set the fullscreen dimensions of the window
      // when using a multi-monitor setup.
            HMONITOR hMonitor = ::MonitorFromWindow(g_hWnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitorInfo = {};
            monitorInfo.cbSize = sizeof(MONITORINFOEX);
            ::GetMonitorInfo(hMonitor, &monitorInfo);
            ::SetWindowPos(g_hWnd, HWND_TOP,
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ::ShowWindow(g_hWnd, SW_MAXIMIZE);
        }
        else
        {
            // Restore all the window decorators.
            ::SetWindowLong(g_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

            ::SetWindowPos(g_hWnd, HWND_NOTOPMOST,
                g_WindowRect.left,
                g_WindowRect.top,
                g_WindowRect.right - g_WindowRect.left,
                g_WindowRect.bottom - g_WindowRect.top,
                SWP_FRAMECHANGED | SWP_NOACTIVATE);

            ::ShowWindow(g_hWnd, SW_NORMAL);
        }
    }
}

void Renderer::loadPipeline() {
    // Initialize the global window rect variable.
    ::GetWindowRect(g_hWnd, &g_WindowRect);
    {
        CreateAuxilaryDeviceResources();

        // Initialize raytracing pipeline.

        // Create raytracing interfaces: raytracing device and commandlist.
        CreateRaytracingInterfaces();

        // Create root signatures for the shaders.
        CreateRootSignatures();

        // Create a raytracing pipeline state object which defines the binding of shaders, state and resources to be used during raytracing.
        CreateRaytracingPipelineStateObject();

        // Create a heap for descriptors.
        CreateDescriptorHeap();

        // Build geometry to be used in the sample.
        BuildGeometry();

        // Build raytracing acceleration structures from the generated geometry.
        BuildAccelerationStructures();

        // Create constant buffers for the geometry and the scene.
        CreateConstantBuffers();

        // Create AABB primitive attribute buffers.
        CreateAABBPrimitiveAttributesBuffers();

        // Build shader tables, which define shaders and their local root arguments.
        BuildShaderTables();

        // Create an output 2D texture to store the raytracing result to.
        CreateRaytracingOutputResource();

        // Define the geometry for a triangle.
        Vertex triangleVertices[] =
        {
            { { 0.0f, 0.25f * g_aspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f, -0.25f * g_aspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f * g_aspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        const UINT vertexBufferSize = sizeof(triangleVertices);

        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.
        auto a = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);//to avoid illegal l value referencing
        auto b = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        g_Device->CreateCommittedResource(
            &a,
            D3D12_HEAP_FLAG_NONE,
            &b,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&g_vertexBuffer)) >> chk;

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);		// We do not intend to read from this resource on the CPU.
        g_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)) >> chk;
        memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
        g_vertexBuffer->Unmap(0, nullptr);

        // Initialize the vertex buffer view.
        g_vertexBufferView.BufferLocation = g_vertexBuffer->GetGPUVirtualAddress();
        g_vertexBufferView.StrideInBytes = sizeof(Vertex);
        g_vertexBufferView.SizeInBytes = vertexBufferSize;
    }

    CheckRaytracingSupport(); // Setup the acceleration structures (AS) for raytracing. When setting up 
                              // geometry, each bottom-level AS has its own transform matrix. 
    CreateAccelerationStructures(); // Command lists are created in the recording state, but there is 
                                    // nothing to record yet. The main loop expects it to be closed, so 
                                    // close it now. 
    g_CommandList->Close() >> chk;
    g_IsInitialized = true;
}

void Renderer::CreateAuxilaryDeviceResources()
{
    auto device = g_deviceResources->GetD3DDevice();
    auto commandQueue = g_deviceResources->GetCommandQueue();

    for (auto& gpuTimer : m_gpuTimers)
    {
        gpuTimer.RestoreDevice(device, commandQueue, g_NumFrames);
    }
}
void Renderer::CheckRaytracingSupport()
{



    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    g_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)) >> chk;
    if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0) throw std::runtime_error("Raytracing not supported on device");
}




Renderer::AccelerationStructureBuffers
Renderer::CreateBottomLevelAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers) {
    nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS; // Adding all vertex buffers and not transforming their position. 
    for (const auto& buffer : vVertexBuffers) { bottomLevelAS.AddVertexBuffer(buffer.first.Get(), 0, buffer.second, sizeof(Vertex), 0, 0); } // The AS build requires some scratch space to store temporary information. 
                                                                                                                                             // The amount of scratch memory is dependent on the scene complexity. 
    UINT64 scratchSizeInBytes = 0;
    // The final AS also needs to be stored in addition to the existing vertex 
    // buffers. It size is also dependent on the scene complexity. 
    UINT64 resultSizeInBytes = 0; bottomLevelAS.ComputeASBufferSizes(g_Device.Get(), false, &scratchSizeInBytes, &resultSizeInBytes); // Once the sizes are obtained, the application is responsible for allocating 
                                                                                                                                      // the necessary buffers. Since the entire generation will be done on the GPU, 
                                                                                                                                      // we can directly allocate those on the default heap 
    AccelerationStructureBuffers buffers;
    buffers.pScratch = nv_helpers_dx12::CreateBuffer(g_Device.Get(), scratchSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, nv_helpers_dx12::kDefaultHeapProps);
    buffers.pResult = nv_helpers_dx12::CreateBuffer(g_Device.Get(), resultSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nv_helpers_dx12::kDefaultHeapProps);
    // Build the acceleration structure. Note that this call integrates a barrier 
    // on the generated AS, so that it can be used to compute a top-level AS right 
    // after this method.
    bottomLevelAS.Generate(g_CommandList.Get(), buffers.pScratch.Get(), buffers.pResult.Get(), false, nullptr);
    return buffers;
}
void Renderer::CreateTopLevelAS(const std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances) { // Gather all the instances into the builder helper 
    for (size_t i = 0; i < instances.size(); i++) { m_topLevelASGenerator.AddInstance(instances[i].first.Get(), instances[i].second, static_cast<UINT>(i), static_cast<UINT>(0)); }
    // As for the bottom-level AS, the building the AS requires some scratch space 
    // to store temporary data in addition to the actual AS. In the case of the 
    // top-level AS, the instance descriptors also need to be stored in GPU 
    // memory. This call outputs the memory requirements for each (scratch, 
    // results, instance descriptors) so that the application can allocate the 
    // corresponding memory 
    UINT64 scratchSize, resultSize, instanceDescsSize; m_topLevelASGenerator.ComputeASBufferSizes(g_Device.Get(), true, &scratchSize, &resultSize, &instanceDescsSize);
    // Create the scratch and result buffers. Since the build is all done on GPU, 
    // those can be allocated on the default heap 
    m_topLevelASBuffers.pScratch = nv_helpers_dx12::CreateBuffer(g_Device.Get(), scratchSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nv_helpers_dx12::kDefaultHeapProps);
    m_topLevelASBuffers.pResult = nv_helpers_dx12::CreateBuffer(g_Device.Get(), resultSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nv_helpers_dx12::kDefaultHeapProps);
    // The buffer describing the instances: ID, shader binding information, 
    // matrices ... Those will be copied into the buffer by the helper through 
    // mapping, so the buffer has to be allocated on the upload heap. 
    m_topLevelASBuffers.pInstanceDesc = nv_helpers_dx12::CreateBuffer(g_Device.Get(), instanceDescsSize, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, nv_helpers_dx12::kUploadHeapProps);
    // After all the buffers are allocated, or if only an update is required, we 
    // // can build the acceleration structure. Note that in the case of the update 
    // we also pass the existing AS as the 'previous' AS, so that it can be 
    // refitted in place. 
    m_topLevelASGenerator.Generate(g_CommandList.Get(), m_topLevelASBuffers.pScratch.Get(), m_topLevelASBuffers.pResult.Get(), m_topLevelASBuffers.pInstanceDesc.Get());
}

void Renderer::CreateAccelerationStructures() {
    // Build the bottom AS from the Triangle vertex buffer 
    AccelerationStructureBuffers bottomLevelBuffers = CreateBottomLevelAS({ {g_vertexBuffer.Get(), 3} });
    // Just one instance for now 
    m_instances = { {bottomLevelBuffers.pResult, XMMatrixIdentity()} }; CreateTopLevelAS(m_instances);
    // Flush the command list and wait for it to finish 
    g_CommandList->Close(); ID3D12CommandList* ppCommandLists[] = { g_CommandList.Get() };
    g_CommandQueue->ExecuteCommandLists(1, ppCommandLists);
    g_FenceValue++;
    g_CommandQueue->Signal(g_Fence.Get(), g_FenceValue);
    g_Fence->SetEventOnCompletion(g_FenceValue, g_FenceEvent);
    WaitForSingleObject(g_FenceEvent, INFINITE);
    // Once the command list is finished executing, reset it to be reused for 
    // rendering 
    g_CommandList->Reset(g_CommandAllocators[0].Get(), g_pipelineState.Get()) >> chk;//I have too many backbuffers FRANK
    // Store the AS buffers. The rest of the buffers will be released once we exit // the function 
    m_bottomLevelAS = bottomLevelBuffers.pResult;
}

void Renderer::CreateRaytracingInterfaces()
{
    //auto device = m_deviceResources->GetD3DDevice();
    //auto commandList = m_deviceResources->GetCommandList();

    err->QueryInterface(IID_PPV_ARGS(&g_Device) >> chk, L"Couldn't get DirectX Raytracing interface for the device.\n");
    errcommandlist->QueryInterface(IID_PPV_ARGS(&g_CommandList) >> chk, L"Couldn't get DirectX Raytracing interface for the command list.\n");
}

void Renderer::SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig)
{
    //auto device = m_deviceResources->GetD3DDevice();
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;

    D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error) >> chk, error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr;
    err->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(*rootSig))) >> chk;
}

void Renderer::CreateRootSignatures()
{
    //auto device = m_deviceResources->GetD3DDevice();

    // Global Root Signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    {
        CD3DX12_DESCRIPTOR_RANGE ranges[2]; // Perfomance TIP: Order from most frequent to least frequent.
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);  // 1 output texture
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1);  // 2 static index and vertex buffers.

        CD3DX12_ROOT_PARAMETER rootParameters[GlobalRootSignature::Slot::Count];
        rootParameters[GlobalRootSignature::Slot::OutputView].InitAsDescriptorTable(1, &ranges[0]);
        rootParameters[GlobalRootSignature::Slot::AccelerationStructure].InitAsShaderResourceView(0);
        rootParameters[GlobalRootSignature::Slot::SceneConstant].InitAsConstantBufferView(0);
        rootParameters[GlobalRootSignature::Slot::AABBattributeBuffer].InitAsShaderResourceView(3);
        rootParameters[GlobalRootSignature::Slot::VertexBuffers].InitAsDescriptorTable(1, &ranges[1]);
        CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
        SerializeAndCreateRaytracingRootSignature(globalRootSignatureDesc, &g_raytracingGlobalRootSignature);
    }

    // Local Root Signature
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    {
        // Triangle geometry
        {
            namespace RootSignatureSlots = LocalRootSignature::Triangle::Slot;
            CD3DX12_ROOT_PARAMETER rootParameters[RootSignatureSlots::Count];
            rootParameters[RootSignatureSlots::MaterialConstant].InitAsConstants(SizeOfInUint32(PrimitiveConstantBuffer), 1);

            CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
            localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
            SerializeAndCreateRaytracingRootSignature(localRootSignatureDesc, &g_raytracingLocalRootSignature[LocalRootSignature::Type::Triangle]);
        }

        // AABB geometry
        {
            namespace RootSignatureSlots = LocalRootSignature::AABB::Slot;
            CD3DX12_ROOT_PARAMETER rootParameters[RootSignatureSlots::Count];
            rootParameters[RootSignatureSlots::MaterialConstant].InitAsConstants(SizeOfInUint32(PrimitiveConstantBuffer), 1);
            rootParameters[RootSignatureSlots::GeometryIndex].InitAsConstants(SizeOfInUint32(PrimitiveInstanceConstantBuffer), 2);

            CD3DX12_ROOT_SIGNATURE_DESC localRootSignatureDesc(ARRAYSIZE(rootParameters), rootParameters);
            localRootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
            SerializeAndCreateRaytracingRootSignature(localRootSignatureDesc, &g_raytracingLocalRootSignature[LocalRootSignature::Type::AABB]);
        }
    }
}*/