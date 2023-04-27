
// DirectX 12 specific headers.

#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
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

    Graphics(UINT width, UINT height, std::wstring name);

    void OnInit();
    void OnUpdate();
    void OnRender();
    void OnDestroy();

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

    ComPtr<ID3D12Device2> g_Device;
    ComPtr<ID3D12CommandQueue> g_CommandQueue;
    ComPtr<IDXGISwapChain4> g_SwapChain;
    ComPtr<ID3D12Resource> g_BackBuffers[g_NumFrames];
    ComPtr<ID3D12GraphicsCommandList> g_CommandList;
    ComPtr<ID3D12CommandAllocator> g_CommandAllocators[g_NumFrames];
    ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
    UINT g_RTVDescriptorSize;
    UINT g_CurrentBackBufferIndex;


    // Advanced Pipeline objects.
    /*D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    UINT m_rtvDescriptorSize;

    // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    // Synchronization objects.*/

    // Synchronization objects


    ComPtr<ID3D12Fence> g_Fence;
    uint64_t g_FenceValue = 0;
    uint64_t g_FrameFenceValues[g_NumFrames] = {};
    HANDLE g_FenceEvent;


    // By default, enable V-Sync.
    // Can be toggled with the V key.
    bool g_VSync = true;
    bool g_TearingSupported = false;// By default, use windowed mode.// Can be toggled with the Alt+Enter or F11
    bool g_Fullscreen = false;



    void LoadPipeline();
    void LoadAssets();
    void PopulateCommandList();
    void WaitForPreviousFrame();
};