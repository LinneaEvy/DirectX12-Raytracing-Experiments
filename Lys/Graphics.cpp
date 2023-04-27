#include "Graphics.h"



void EnableDebugLayer()
{
    #if defined(_DEBUG)
        ComPtr<ID3D12Debug> debugInterface;
        if (FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) {
            throw std::exception();
        }
        debugInterface->EnableDebugLayer();
    #endif
}

ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp)
{
    ComPtr<IDXGIFactory4> dxgiFactory;
    UINT createFactoryFlags = 0;
    #if defined(_DEBUG)
    createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
    #endif

    if (FAILED(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)))) {
        throw std::exception();
    }

    ComPtr<IDXGIAdapter1> dxgiAdapter1;
    ComPtr<IDXGIAdapter4> dxgiAdapter4;
    if (useWarp)
    {
        ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
        ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
    }

}



void loadPipeline() {
    EnableDebugLayer();
    GetAdapter(true);

}