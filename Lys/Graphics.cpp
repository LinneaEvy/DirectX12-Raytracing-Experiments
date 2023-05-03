#pragma once
#include "Graphics.h"
#include "dxerr.h"
#include "GraphicsError.h"
#include <tchar.h>
//#include <Winuser.h>
//#include "DXRHelper.h"

#include "CompiledShaders\Raytracing.hlsl.h"
#include "TopLevelASGenerator.h"
#include "BottomLevelASGenerator.h"


#define NAME_D3D12_OBJECT(x) SetName((x).Get(), L#x)
//#include "CompiledShaders\Raytracing.hlsl.h"FRANK
// Graphics exception stuff
Graphics::HrException::HrException(int line, const char* file, HRESULT hr, std::vector<std::string> infoMsgs) noexcept
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

const char* Graphics::HrException::what() const noexcept
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

const char* Graphics::HrException::GetType() const noexcept
{
    return "Chili Graphics Exception";
}

HRESULT Graphics::HrException::GetErrorCode() const noexcept
{
    return hr;
}


std::string Graphics::HrException::GetErrorString() const noexcept
{
    return ToNarrow(DXGetErrorString(hr));
}


std::string Graphics::HrException::GetErrorDescription() const noexcept
{
    char buf[512];
    DXGetErrorDescriptionA(hr, buf, sizeof(buf));
    return buf;
}

std::string Graphics::HrException::GetErrorInfo() const noexcept
{
    return info;
}


const char* Graphics::DeviceRemovedException::GetType() const noexcept
{
    return "Chili Graphics Exception [Device Removed] (DXGI_ERROR_DEVICE_REMOVED)";
}

Graphics::InfoException::InfoException(int line, const char* file, std::vector<std::string> infoMsgs) noexcept
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

const char* Graphics::InfoException::what() const noexcept
{
    std::ostringstream oss;
    oss << GetType() << std::endl
        << "\n[Error Info]\n" << GetErrorInfo() << std::endl << std::endl;
    oss << GetOriginString();
    whatBuffer = oss.str();
    return whatBuffer.c_str();
}

const char* Graphics::InfoException::GetType() const noexcept
{
    return "Chili Graphics Info Exception";
}

std::string Graphics::InfoException::GetErrorInfo() const noexcept
{
    return info;
}



Graphics::Graphics(HWND hWnd, UINT width, UINT height, std::wstring name)
{
    
    EnableDebugLayer();
    g_TearingSupported = CheckTearingSupport();
    g_hWnd = hWnd;
    g_ClientWidth = width;
    g_ClientHeight = height;
    loadPipeline();
}

Graphics::~Graphics()
{
    Flush(g_CommandQueue, g_Fence, g_FenceValue, g_FenceEvent);
    ::CloseHandle(g_FenceEvent);
}

void Graphics::EnableDebugLayer()
{
    #if defined(_DEBUG)
        ComPtr<ID3D12Debug> debugInterface;
        FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))) >> chk;
        debugInterface->EnableDebugLayer();
    #endif
}

ComPtr<IDXGIAdapter1> Graphics::GetAdapter(bool useWarp)
{
    ComPtr<IDXGIFactory4> dxgiFactory;
    UINT createFactoryFlags = 0;
    #if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
    #endif

    CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)) >> chk;

    ComPtr<IDXGIAdapter1> dxgiAdapter1;
    //ComPtr<IDXGIAdapter4> dxgiAdapter4;
    if (useWarp)
    {
        dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)) >> chk;
        //dxgiAdapter1.As(&dxgiAdapter4) >> chk;
    }
    else
    {
        SIZE_T maxDedicatedVideoMemory = 0;
        for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
        {
            DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
            dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

            // Check to see if the adapter can create a D3D12 device without actually 
            // creating it. The adapter with the largest dedicated video memory
            // is favored.
            if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
                SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
                    D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
                dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
            {
                maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
                //dxgiAdapter1.As(&dxgiAdapter4) >> chk;
            }
        }
    }

    return dxgiAdapter1;
}

ComPtr<ID3D12Device5> Graphics::CreateDevice(ComPtr<IDXGIAdapter1> adapter)
{
    ComPtr<ID3D12Device5> d3d12Device5;
    D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&d3d12Device5)) >> chk;
    // Enable debug messages in debug mode.
#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;
    if (SUCCEEDED(d3d12Device5.As(&pInfoQueue)))
    {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        // Suppress whole categories of messages
        //D3D12_MESSAGE_CATEGORY Categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY Severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID DenyIds[] = {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
        };

        D3D12_INFO_QUEUE_FILTER NewFilter = {};
        //NewFilter.DenyList.NumCategories = _countof(Categories);
        //NewFilter.DenyList.pCategoryList = Categories;
        NewFilter.DenyList.NumSeverities = _countof(Severities);
        NewFilter.DenyList.pSeverityList = Severities;
        NewFilter.DenyList.NumIDs = _countof(DenyIds);
        NewFilter.DenyList.pIDList = DenyIds;

        pInfoQueue->PushStorageFilter(&NewFilter) >> chk;
    }
#endif

    return d3d12Device5;
}

ComPtr<ID3D12CommandQueue> Graphics::CreateCommandQueue(ComPtr<ID3D12Device5> device, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandQueue> d3d12CommandQueue;

    D3D12_COMMAND_QUEUE_DESC desc = {};
    desc.Type = type;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    desc.NodeMask = 0;

    device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)) >> chk;

    return d3d12CommandQueue;
}

bool Graphics::CheckTearingSupport()
{
    BOOL allowTearing = FALSE;

    // Rather than create the DXGI 1.5 factory interface directly, we create the
    // DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
    // graphics debugging tools which will not support the 1.5 factory interface 
    // until a future update.
    ComPtr<IDXGIFactory4> factory4;
    if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
    {
        ComPtr<IDXGIFactory5> factory5;
        if (SUCCEEDED(factory4.As(&factory5)))
        {
            if (FAILED(factory5->CheckFeatureSupport(
                DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                &allowTearing, sizeof(allowTearing))))
            {
                allowTearing = FALSE;
            }
        }
    }

    return allowTearing == TRUE;
}

ComPtr<IDXGISwapChain4> Graphics::CreateSwapChain(HWND hWnd,
    ComPtr<ID3D12CommandQueue> commandQueue,
    uint32_t width, uint32_t height, uint32_t bufferCount)
{
    ComPtr<IDXGISwapChain4> dxgiSwapChain4;
    ComPtr<IDXGIFactory4> dxgiFactory4;
    UINT createFactoryFlags = 0;
#if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

    CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)) >> chk;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = bufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    // It is recommended to always allow tearing if tearing support is available.
    swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    ComPtr<IDXGISwapChain1> swapChain1;
    dxgiFactory4->CreateSwapChainForHwnd(
        commandQueue.Get(),
        hWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain1) >> chk;

    // Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
    // will be handled manually.
    dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER) >> chk;
    swapChain1.As(&dxgiSwapChain4) >> chk;

    return dxgiSwapChain4;
}
    

ComPtr<ID3D12DescriptorHeap> Graphics::CreateDescriptorHeap(ComPtr<ID3D12Device5> device,
    D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
    ComPtr<ID3D12DescriptorHeap> descriptorHeap;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = numDescriptors;
    desc.Type = type;

    device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeap)) >> chk;

    return descriptorHeap;
}

void Graphics::UpdateRenderTargetViews(ComPtr<ID3D12Device5> device,
    ComPtr<IDXGISwapChain4> swapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap)
{
    auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

    for (int i = 0; i < g_NumFrames; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)) >> chk;

        device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

        g_BackBuffers[i] = backBuffer;

        rtvHandle.Offset(rtvDescriptorSize);
    }
}

ComPtr<ID3D12CommandAllocator> Graphics::CreateCommandAllocator(ComPtr<ID3D12Device5> device,
    D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)) >> chk;

    return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList5> Graphics::CreateCommandList(ComPtr<ID3D12Device5> device,
    ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type)
{
    ComPtr<ID3D12GraphicsCommandList5> commandList;
    device->CreateCommandList(0, type, commandAllocator.Get(), g_pipelineState.Get(), IID_PPV_ARGS(&commandList)) >> chk;

    //commandList->Close() >> chk;

    return commandList;
}

ComPtr<ID3D12Fence> Graphics::CreateFence(ComPtr<ID3D12Device5> device)
{
    ComPtr<ID3D12Fence> fence;

    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)) >> chk;

    return fence;
}

HANDLE Graphics::CreateEventHandle()
{
    HANDLE fenceEvent;

    fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(fenceEvent && "Failed to create fence event.");

    return fenceEvent;
}

uint64_t Graphics::Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence,
    uint64_t& fenceValue)
{
    uint64_t fenceValueForSignal = ++fenceValue;
    commandQueue->Signal(fence.Get(), fenceValueForSignal) >> chk;

    return fenceValueForSignal;
}

void Graphics::WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent,
    std::chrono::milliseconds duration)
{
    if (fence->GetCompletedValue() < fenceValue)
    {
        fence->SetEventOnCompletion(fenceValue, fenceEvent) >> chk;
        ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
    }
}

void Graphics::Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence,
    uint64_t& fenceValue, HANDLE fenceEvent)
{
    uint64_t fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
    WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
}

void Graphics::Update()
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

void Graphics::Render()
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
        g_SwapChain->Present(syncInterval, presentFlags) >> chk;

        g_FrameFenceValues[g_CurrentBackBufferIndex] = Signal(g_CommandQueue, g_Fence, g_FenceValue);
        g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

        WaitForFenceValue(g_Fence, g_FrameFenceValues[g_CurrentBackBufferIndex], g_FenceEvent);
    }
}

void Graphics::Resize(uint32_t width, uint32_t height)
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

void Graphics::SetFullscreen(bool fullscreen)
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

void Graphics::loadPipeline() {




    // Initialize the global window rect variable.
    ::GetWindowRect(g_hWnd, &g_WindowRect);
    ComPtr<IDXGIAdapter1> dxgiAdapter1 = GetAdapter(false);

    g_Device = CreateDevice(dxgiAdapter1);

    g_CommandQueue = CreateCommandQueue(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

    g_SwapChain = CreateSwapChain(g_hWnd, g_CommandQueue,
        g_ClientWidth, g_ClientHeight, g_NumFrames);

    g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();

    g_RTVDescriptorHeap = CreateDescriptorHeap(g_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, g_NumFrames);
    g_RTVDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap);
    for (int i = 0; i < g_NumFrames; ++i)
    {
        g_CommandAllocators[i] = CreateCommandAllocator(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
    }
    g_CommandList = CreateCommandList(g_Device,
        g_CommandAllocators[g_CurrentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);



    {
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

    //g_CommandList->Close() >> chk;
    //CheckRaytracingSupport();
    g_Fence = CreateFence(g_Device);
    g_FenceEvent = CreateEventHandle();
    g_CommandList->Close() >> chk;
    g_IsInitialized = true;

    CreateAuxilaryDeviceResources();//FRANK

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
    //BuildGeometry();

    // Check the raytracing capabilities of the device 
    CheckRaytracingSupport(); // Setup the acceleration structures (AS) for raytracing. When setting up 
                              // geometry, each bottom-level AS has its own transform matrix. 
    CreateAccelerationStructures(); // Command lists are created in the recording state, but there is 
                                    // nothing to record yet. The main loop expects it to be closed, so 
                                    // close it now. 

    //CreateConstantBuffers();

    // Create AABB primitive attribute buffers.
    //CreateAABBPrimitiveAttributesBuffers();

    // Build shader tables, which define shaders and their local root arguments.
    //BuildShaderTables();

    // Create an output 2D texture to store the raytracing result to.
    //CreateRaytracingOutputResource();
    
}
void Graphics::CheckRaytracingSupport()
{



    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    g_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)) >> chk;
    if (options5.RaytracingTier < D3D12_RAYTRACING_TIER_1_0) throw std::runtime_error("Raytracing not supported on device");
}




Graphics::AccelerationStructureBuffers
Graphics::CreateBottomLevelAS(std::vector<std::pair<ComPtr<ID3D12Resource>, uint32_t>> vVertexBuffers) {
    nv_helpers_dx12::BottomLevelASGenerator bottomLevelAS; // Adding all vertex buffers and not transforming their position. 
    for (const auto &buffer : vVertexBuffers) { bottomLevelAS.AddVertexBuffer(buffer.first.Get(), 0, buffer.second, sizeof(Vertex), 0, 0); } // The AS build requires some scratch space to store temporary information. 
                                                                                                                                             // The amount of scratch memory is dependent on the scene complexity. 
    UINT64 scratchSizeInBytes = 0; 
                                                                                                                                             // The final AS also needs to be stored in addition to the existing vertex 
                                                                                                                                             // buffers. It size is also dependent on the scene complexity. 
    UINT64 resultSizeInBytes = 0; bottomLevelAS.ComputeASBufferSizes(g_Device.Get(), false, &scratchSizeInBytes, &resultSizeInBytes); // Once the sizes are obtained, the application is responsible for allocating 
                                                                                                                                      // the necessary buffers. Since the entire generation will be done on the GPU, 
                                                                                                                                      // we can directly allocate those on the default heap 
    AccelerationStructureBuffers buffers; 
    buffers.pScratch = nv_helpers_dx12::CreateBuffer( g_Device.Get(), scratchSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON, nv_helpers_dx12::kDefaultHeapProps); 
    buffers.pResult = nv_helpers_dx12::CreateBuffer( g_Device.Get(), resultSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nv_helpers_dx12::kDefaultHeapProps); 
    // Build the acceleration structure. Note that this call integrates a barrier 
    // on the generated AS, so that it can be used to compute a top-level AS right 
    // after this method.
    bottomLevelAS.Generate(g_CommandList.Get(), buffers.pScratch.Get(), buffers.pResult.Get(), false, nullptr); 
    return buffers;
}
void Graphics::CreateTopLevelAS(const std::vector<std::pair<ComPtr<ID3D12Resource>, DirectX::XMMATRIX>>& instances) { // Gather all the instances into the builder helper 
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

void Graphics::CreateAccelerationStructures() {
    // Build the bottom AS from the Triangle vertex buffer 
    AccelerationStructureBuffers bottomLevelBuffers = CreateBottomLevelAS({{g_vertexBuffer.Get(), 3}}); 
    // Just one instance for now 
    m_instances = {{bottomLevelBuffers.pResult, XMMatrixIdentity()}}; CreateTopLevelAS(m_instances); 
    // Flush the command list and wait for it to finish 
    g_CommandList->Close(); ID3D12CommandList *ppCommandLists[] = { g_CommandList.Get()};
    g_CommandQueue->ExecuteCommandLists(1, ppCommandLists);
    g_FenceValue ++;
    g_CommandQueue->Signal(g_Fence.Get(), g_FenceValue);
    g_Fence->SetEventOnCompletion(g_FenceValue, g_FenceEvent);
    WaitForSingleObject(g_FenceEvent, INFINITE);
    // Once the command list is finished executing, reset it to be reused for 
    // rendering 
    g_CommandList->Reset(g_CommandAllocators[0].Get(), g_pipelineState.Get()) >> chk;//I have too many backbuffers FRANK
    // Store the AS buffers. The rest of the buffers will be released once we exit // the function 
    m_bottomLevelAS = bottomLevelBuffers.pResult;
}

void Graphics::CreateRaytracingInterfaces()
{
    //auto device = m_deviceResources->GetD3DDevice();
    //auto commandList = m_deviceResources->GetCommandList();
    //g_device is their device from the main graphics and GI device from external recource
    g_Device->QueryInterface(IID_PPV_ARGS(&g_GIDevice)) >> chk;
    g_CommandList->QueryInterface(IID_PPV_ARGS(&g_CommandList)) >> chk;
}

void Graphics::SerializeAndCreateRaytracingRootSignature(D3D12_ROOT_SIGNATURE_DESC& desc, ComPtr<ID3D12RootSignature>* rootSig)
{
    //auto device = m_deviceResources->GetD3DDevice();
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;

    D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error) >> chk, error ? static_cast<wchar_t*>(error->GetBufferPointer()) : nullptr ;
    g_Device->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&(*rootSig))) >> chk;
}
void Graphics::CreateAuxilaryDeviceResources()
{
    //auto device = g_deviceResources->GetD3DDevice();
    //auto commandQueue = m_deviceResources->GetCommandQueue();

    //for (auto& gpuTimer : m_gpuTimers)
    {
        //gpuTimer.RestoreDevice(g_Device, g_CommandQueue, g_NumFrames);
    }
}

void Graphics::CreateRootSignatures()
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
}
const wchar_t* Graphics::c_closestHitShaderNames[] =
{
    L"MyClosestHitShader_Triangle",
    L"MyClosestHitShader_AABB",
};
void Graphics::CreateHitGroupSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline)
{
    // Triangle geometry hit groups
    {
        for (UINT rayType = 0; rayType < RayType::Count; rayType++)
        {
            auto hitGroup = raytracingPipeline->CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
            if (rayType == RayType::Radiance)
            {
                hitGroup->SetClosestHitShaderImport(c_closestHitShaderNames[GeometryType::Triangle]);
            }
            hitGroup->SetHitGroupExport(c_hitGroupNames_TriangleGeometry[rayType]);
            hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
        }
    }

    // AABB geometry hit groups
    {
        // Create hit groups for each intersection shader.
        for (UINT t = 0; t < IntersectionShaderType::Count; t++)
            for (UINT rayType = 0; rayType < RayType::Count; rayType++)
            {
                auto hitGroup = raytracingPipeline->CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                hitGroup->SetIntersectionShaderImport(c_intersectionShaderNames[t]);
                if (rayType == RayType::Radiance)
                {
                    hitGroup->SetClosestHitShaderImport(c_closestHitShaderNames[GeometryType::AABB]);
                }
                hitGroup->SetHitGroupExport(c_hitGroupNames_AABBGeometry[t][rayType]);
                hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE);
            }
    }
}

void Graphics::CreateDxilLibrarySubobject(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline)
{
    auto lib = raytracingPipeline->CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
    //D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE((void*)g_pRaytracing, ARRAYSIZE(g_pRaytracing));FRANK
    //lib->SetDXILLibrary(&libdxil);FRANK
    // Use default shader exports for a DXIL library/collection subobject ~ surface all shaders.
}

// Local root signature and shader association
// This is a root signature that enables a shader to have unique arguments that come from shader tables.
void Graphics::CreateLocalRootSignatureSubobjects(CD3DX12_STATE_OBJECT_DESC* raytracingPipeline)
{
    // Ray gen and miss shaders in this sample are not using a local root signature and thus one is not associated with them.

    // Hit groups
    // Triangle geometry
    {
        auto localRootSignature = raytracingPipeline->CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
        localRootSignature->SetRootSignature(g_raytracingLocalRootSignature[LocalRootSignature::Type::Triangle].Get());
        // Shader association
        auto rootSignatureAssociation = raytracingPipeline->CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
        rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
        rootSignatureAssociation->AddExports(c_hitGroupNames_TriangleGeometry);
    }

    // AABB geometry
    {
        auto localRootSignature = raytracingPipeline->CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
        localRootSignature->SetRootSignature(g_raytracingLocalRootSignature[LocalRootSignature::Type::AABB].Get());
        // Shader association
        auto rootSignatureAssociation = raytracingPipeline->CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
        rootSignatureAssociation->SetSubobjectToAssociate(*localRootSignature);
        for (auto& hitGroupsForIntersectionShaderType : c_hitGroupNames_AABBGeometry)
        {
            //rootSignatureAssociation->AddExports(hitGroupsForIntersectionShaderType);FRANK
        }
    }
}

void Graphics::CreateRaytracingPipelineStateObject()
{
    // Create 18 subobjects that combine into a RTPSO:
    // Subobjects need to be associated with DXIL exports (i.e. shaders) either by way of default or explicit associations.
    // Default association applies to every exported shader entrypoint that doesn't have any of the same type of subobject associated with it.
    // This simple sample utilizes default shader association except for local root signature subobject
    // which has an explicit association specified purely for demonstration purposes.
    // 1 - DXIL library
    // 8 - Hit group types - 4 geometries (1 triangle, 3 aabb) x 2 ray types (ray, shadowRay)
    // 1 - Shader config
    // 6 - 3 x Local root signature and association
    // 1 - Global root signature
    // 1 - Pipeline config
    CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };

    // DXIL library
    CreateDxilLibrarySubobject(&raytracingPipeline);

    // Hit groups
    CreateHitGroupSubobjects(&raytracingPipeline);

    // Shader config
    // Defines the maximum sizes in bytes for the ray rayPayload and attribute structure.
    auto shaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    UINT payloadSize = sizeof(RayPayload) > sizeof(ShadowRayPayload) ? sizeof(RayPayload) : sizeof(ShadowRayPayload);
    UINT attributeSize = sizeof(struct ProceduralPrimitiveAttributes);
    shaderConfig->Config(payloadSize, attributeSize);

    // Local root signature and shader association
    // This is a root signature that enables a shader to have unique arguments that come from shader tables.
    CreateLocalRootSignatureSubobjects(&raytracingPipeline);

    // Global root signature
    // This is a root signature that is shared across all raytracing shaders invoked during a DispatchRays() call.
    auto globalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    globalRootSignature->SetRootSignature(g_raytracingGlobalRootSignature.Get());

    // Pipeline config
    // Defines the maximum TraceRay() recursion depth.
    auto pipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    // PERFOMANCE TIP: Set max recursion depth as low as needed
    // as drivers may apply optimization strategies for low recursion depths.
    UINT maxRecursionDepth = MAX_RAY_RECURSION_DEPTH;
    pipelineConfig->Config(maxRecursionDepth);

    //PrintStateObjectDesc(raytracingPipeline);

    // Create the state object.
    g_Device->CreateStateObject(raytracingPipeline, IID_PPV_ARGS(&g_pipelineState)) >> chk;
}
void Graphics::CreateDescriptorHeap()
{
    //auto device = m_deviceResources->GetD3DDevice();

    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
    // Allocate a heap for 6 descriptors:
    // 2 - vertex and index  buffer SRVs
    // 1 - raytracing output texture SRV
    descriptorHeapDesc.NumDescriptors = 3;
    descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    descriptorHeapDesc.NodeMask = 0;
    g_Device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&g_RTVDescriptorHeap));
    //NAME_D3D12_OBJECT(g_RTVDescriptorHeap);FRANK

    g_RTVDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}