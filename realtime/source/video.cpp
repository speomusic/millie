//#include "merc/video.h"
//#include "merc/windows-utils.h"
//#include <filesystem>
//#include <windows.h>
//
//namespace merc
//{
//    struct RealtimeVideoImplementation
//    {
//        RealtimeVideoImplementation(std::shared_ptr<Chain> chain);
//        ~RealtimeVideoImplementation();
//        void setup(HWND hWnd);
//        void draw(HWND hWnd);
//        void waitForPreviousFrame();
//        void stop();
//        operator bool() const { return active; }
//    private:
//        std::shared_ptr<Chain> chain;
//        HWND hwnd;
//        bool active{ true };
//    };
//
//    static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//    {
//        if (message == WM_CREATE)
//        {
//            auto createStruct{ reinterpret_cast<LPCREATESTRUCT>(lParam) };
//            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
//        }
//    
//        if (auto realtimeVideoImplementation{ reinterpret_cast<RealtimeVideoImplementation*>(GetWindowLongPtr(hWnd, GWLP_USERDATA)) })
//        {
//            switch (message)
//            {
//            case WM_CREATE:
//                realtimeVideoImplementation->setup(hWnd);
//                break;
//            case WM_PAINT:
//                // dummy paint, d12 render loop runs independent of these messages
//                // https://stackoverflow.com/a/66062403
//                PAINTSTRUCT ps;
//                BeginPaint(hWnd, &ps);
//                EndPaint(hWnd, &ps);
//                break;
//            case WM_NCCALCSIZE:
//                // hide default window frame
//                if (wParam == TRUE) return 0;
//                break;
//            case WM_NCHITTEST:
//                // every left click moves the window
//                return HTCAPTION;
//            case WM_KEYDOWN:
//                switch (wParam)
//                {
//                case 27: // esc
//                    DestroyWindow(hWnd);
//                    break;
//                }
//                break;
//            case WM_DESTROY:
//                realtimeVideoImplementation->stop();
//                break;
//            }
//        }
//
//        return DefWindowProc(hWnd, message, wParam, lParam);
//    }
//
//    static LPCWSTR registerClass(HINSTANCE hInstance)
//    {
//        LPCWSTR ret{ 0 };
//        WNDCLASS windowClass
//        {
//            .style = CS_HREDRAW | CS_VREDRAW,
//            .lpfnWndProc = WindowProc,
//            .hInstance = hInstance,
//            .hCursor = LoadCursor(NULL, IDC_ARROW),
//            .hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH),
//            .lpszClassName = L"mercRealtimeVideoImplementationClass"
//        };
//        return MAKEINTATOM(RegisterClass(&windowClass));
//    }
//
//    RealtimeVideoImplementation::RealtimeVideoImplementation(std::shared_ptr<Chain> chain)
//        : chain{ std::move(chain) }
//        , hwnd
//        {
//            CreateWindowEx(
//                0,
//                registerClass(getCurrentModule()),
//                nullptr,
//                WS_BORDER,
//                CW_USEDEFAULT,
//                CW_USEDEFAULT,
//                800,
//                600,
//                nullptr, 
//                nullptr,
//                getCurrentModule(),
//                this)
//        }
//    {
//        ShowWindow(hwnd, SW_NORMAL);
//        // Hide windows frame: https://learn.microsoft.com/en-us/windows/win32/dwm/customframe
//        RECT r; GetWindowRect(hwnd, &r);
//        SetWindowPos(hwnd, nullptr, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_FRAMECHANGED);
//    }
//
//    RealtimeVideoImplementation::~RealtimeVideoImplementation()
//    {
//        if (active)
//            DestroyWindow(hwnd);
//    }
//
//    RealtimeVideo::RealtimeVideo(std::shared_ptr<Chain> chain)
//        : implementation{ std::make_unique<RealtimeVideoImplementation>(std::move(chain)) }
//    {}
//    RealtimeVideo::~RealtimeVideo() {}
//    RealtimeVideo::operator bool() const { return *implementation; }
//
//    void RealtimeVideoImplementation::setup(HWND hWnd)
//    {
//        ComPtr<IDXGIFactory4> factory;
//#if defined(_DEBUG)
//        ComPtr<ID3D12Debug> debugController;
//        ComPtr<ID3D12Debug1> debugController1;
//        THROW_IF_FAILED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
//        debugController->EnableDebugLayer();
//        THROW_IF_FAILED(debugController->QueryInterface(IID_PPV_ARGS(&debugController1)));
//        debugController1->SetEnableGPUBasedValidation(true);
//        THROW_IF_FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)));
//#else
//        THROW_IF_FAILED((CreateDXGIFactory2(0, IID_PPV_ARGS(&factory))));
//#endif
//        // uses primary hardware adapter
//        THROW_IF_FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d12.device)));
//
//        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
//        THROW_IF_FAILED(d12.device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&d12.commandQueue)));
//
//        DXGI_SWAP_CHAIN_DESC1 swapChainDesc
//        {
//            .Format = d12.renderTargetFormat,
//            .SampleDesc = { .Count = 1 },
//            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
//            .BufferCount = d12.swapChainBufferCount,
//            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
//        };
//        ComPtr<IDXGISwapChain1> swapChain1;
//        THROW_IF_FAILED(factory->CreateSwapChainForHwnd(
//            d12.commandQueue.Get(),
//            hWnd,
//            &swapChainDesc,
//            nullptr,
//            nullptr,
//            &swapChain1
//        ));
//        THROW_IF_FAILED(swapChain1.As(&d12.swapChain));
//        d12.backBufferIndex = d12.swapChain->GetCurrentBackBufferIndex();
//
//        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc // Render Target View
//        {
//            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
//            .NumDescriptors = d12.swapChainBufferCount + 1,
//        };
//        THROW_IF_FAILED(d12.device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&d12.rtvHeap)));
//        d12.rtvDescriptorSize = d12.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
//        auto rtvHandle{ d12.rtvHeap->GetCPUDescriptorHandleForHeapStart() };
//        for (UINT n = 0; n < d12.swapChainBufferCount; n++)
//        {
//            THROW_IF_FAILED(d12.swapChain->GetBuffer(n, IID_PPV_ARGS(&d12.renderTargets[n])));
//            d12.device->CreateRenderTargetView(d12.renderTargets[n].Get(), nullptr, rtvHandle);
//            rtvHandle.ptr += d12.rtvDescriptorSize;
//        }
//
//        D3D12_RESOURCE_DESC msaaRenderTargetDesc
//        {
//            .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
//            .Width = d12.renderTargets[0]->GetDesc().Width,
//            .Height = d12.renderTargets[0]->GetDesc().Height,
//            .DepthOrArraySize = 1,
//            .MipLevels = 1,
//            .Format = d12.renderTargetFormat,
//            .SampleDesc = { .Count = d12.msaaSampleCount },
//            .Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
//        };
//        D3D12_HEAP_PROPERTIES msaaRenderTargetHeapProperties
//        {
//            .Type = D3D12_HEAP_TYPE_DEFAULT
//        };
//        D3D12_CLEAR_VALUE msaaRtOptimalClearValue
//        {
//            .Format = d12.renderTargetFormat,
//        };
//        memcpy(&msaaRtOptimalClearValue.Color, &d12.clearColor, sizeof(d12.clearColor));
//        THROW_IF_FAILED(d12.device->CreateCommittedResource(
//            &msaaRenderTargetHeapProperties,
//            D3D12_HEAP_FLAG_NONE,
//            &msaaRenderTargetDesc,
//            D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
//            &msaaRtOptimalClearValue,
//            IID_PPV_ARGS(&d12.msaaRenderTarget)
//        ));
//
//        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc
//        {
//            .Format = d12.renderTargetFormat,
//            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS
//        };
//        d12.device->CreateRenderTargetView(d12.msaaRenderTarget.Get(), &rtvDesc, rtvHandle);
//
//        THROW_IF_FAILED(d12.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&d12.commandAllocator)));
//
//        THROW_IF_FAILED(d12.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, d12.commandAllocator.Get(), nullptr, IID_PPV_ARGS(&d12.commandList)));
//        THROW_IF_FAILED(d12.commandList->Close());
//
//        THROW_IF_FAILED(d12.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d12.fence)));
//        d12.fenceValue = 1;
//        d12.fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
//        if (d12.fenceEvent == nullptr) THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
//
//        chain->setup(*d12.device.Get(), d12.renderTargetFormat, d12.msaaSampleCount);
//
//        // Wait for the command list to execute; we are reusing the same command
//        // list in our main loop but for now, we just want to wait for setup to
//        // complete before continuing.
//        waitForPreviousFrame();
//    }
//
//    void RealtimeVideoImplementation::waitForPreviousFrame()
//    {
//        // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
//        // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
//        // sample illustrates how to use fences for efficient resource usage and to
//        // maximize GPU utilization.
//        THROW_IF_FAILED(d12.commandQueue->Signal(d12.fence.Get(), d12.fenceValue++));
//        if (d12.fence->GetCompletedValue() < d12.fenceValue - 1)
//        {
//            THROW_IF_FAILED(d12.fence->SetEventOnCompletion(d12.fenceValue - 1, d12.fenceEvent));
//            WaitForSingleObject(d12.fenceEvent, INFINITE);
//        }
//        d12.backBufferIndex = d12.swapChain->GetCurrentBackBufferIndex();
//    }
//
//    void RealtimeVideoImplementation::draw(HWND hWnd)
//    {
//        // Command list allocators can only be reset when the associated
//        // command lists have finished execution on the GPU; apps should use
//        // fences to determine GPU execution progress.
//        THROW_IF_FAILED(d12.commandAllocator->Reset());
//        // However, when ExecuteCommandList() is called on a particular command
//        // list, that command list can then be reset at any time and must be before
//        // re-recording.
//        THROW_IF_FAILED(d12.commandList->Reset(d12.commandAllocator.Get(), nullptr));
//
//        RECT clientRect;
//        GetClientRect(hWnd, &clientRect);
//        D3D12_VIEWPORT viewport{ 0.0f, 0.0f, (float)clientRect.right, (float)clientRect.bottom, D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
//        d12.commandList->RSSetViewports(1, &viewport);
//        d12.commandList->RSSetScissorRects(1, &clientRect);
//        D3D12_RESOURCE_BARRIER renderTargetReadyResourceBarrier
//        {
//            .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
//            .Transition =
//            {
//                .pResource = d12.msaaRenderTarget.Get(),
//                .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
//                .StateBefore = D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
//                .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET
//        }
//        };
//        d12.commandList->ResourceBarrier(1, &renderTargetReadyResourceBarrier);
//        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle{ d12.rtvHeap->GetCPUDescriptorHandleForHeapStart() };
//        rtvHandle.ptr += d12.swapChainBufferCount * d12.rtvDescriptorSize;
//        d12.commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
//
//        d12.commandList->ClearRenderTargetView(rtvHandle, d12.clearColor, 0, nullptr);
//
//        chain->draw(*d12.commandList.Get());
//
//        D3D12_RESOURCE_BARRIER renderTargetResolveBarriers[2]
//        {
//            {
//                .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
//                .Transition =
//            {
//                .pResource = d12.msaaRenderTarget.Get(),
//                .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
//                .StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
//                .StateAfter = D3D12_RESOURCE_STATE_RESOLVE_SOURCE
//        }
//            },
//            {
//                .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
//                .Transition =
//            {
//                .pResource = d12.renderTargets[d12.backBufferIndex].Get(),
//                .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
//                .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
//                .StateAfter = D3D12_RESOURCE_STATE_RESOLVE_DEST
//        }
//            }
//        };
//        d12.commandList->ResourceBarrier(2, renderTargetResolveBarriers);
//        d12.commandList->ResolveSubresource(d12.renderTargets[d12.backBufferIndex].Get(), 0, d12.msaaRenderTarget.Get(), 0, d12.renderTargetFormat);
//
//        D3D12_RESOURCE_BARRIER renderTargetPresentResourceBarrier
//        {
//            .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
//            .Transition =
//            {
//                .pResource = d12.renderTargets[d12.backBufferIndex].Get(),
//                .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
//                .StateBefore = D3D12_RESOURCE_STATE_RESOLVE_DEST,
//                .StateAfter = D3D12_RESOURCE_STATE_PRESENT
//        }
//        };
//        d12.commandList->ResourceBarrier(1, &renderTargetPresentResourceBarrier);
//
//        THROW_IF_FAILED(d12.commandList->Close());
//        ID3D12CommandList* commandListPointer{ d12.commandList.Get() };
//        d12.commandQueue->ExecuteCommandLists(1, &commandListPointer);
//        THROW_IF_FAILED(d12.swapChain->Present(1, 0));
//
//        waitForPreviousFrame();
//    }
//
//    void RealtimeVideoImplementation::stop()
//    {
//        // ensure GPU command queue is empty before destruction
//        waitForPreviousFrame();
//        CloseHandle(d12.fenceEvent);
//        active = false;
//    }
//}
