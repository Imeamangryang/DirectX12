#include "Render/Renderer.h"

namespace Engine
{
    Renderer::Renderer() :
        m_dxgiFactory(nullptr),
        m_swapChain(nullptr),
        m_device(nullptr),
        m_renderTargets{ nullptr,nullptr },
        m_commandAllocator(),
        m_commandQueue(nullptr),
        m_rootSignature(nullptr),
        m_rtvHeap(nullptr),
        m_pipelineState(nullptr),
        m_commandList(nullptr),
        m_vertexBuffer(nullptr),
        m_vertexBufferView(),
        m_indexBuffer(nullptr),
        m_indexBufferView(),
        m_rtvDescriptorSize(0u),
        m_frameIndex(0u),
        m_fenceEvent(),
        m_fence(),
        m_fenceValues(),
        vertexShader(nullptr),
        pixelShader(nullptr),
        signature(nullptr),
        error(nullptr)
    {}

    // Initialize 함수 (Graphics Pipeline 초기화)
    HRESULT Renderer::Initialize(_In_ HWND hWnd)
    {
        HRESULT hr = S_OK;
        hr = initializePipeLine(hWnd);
        if (FAILED(hr))
        {
            return hr;
        }

        hr = initializeAssets();
        if (FAILED(hr))
        {
            return hr;
        }

        return S_OK;
    }

    // Render 함수 (Command Queue를 작성하고 실행함)
    void Renderer::Render(_In_ FLOAT deltaTime)
    {
        populateCommandList();  // command list 기록

        ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists); //command queue에 담긴 command list들 실행명령(비동기)

        m_swapChain->Present(1, 0); //백버퍼 교체

        moveToNextFrame();
    }

    // Graphics Pipeline Initialize 함수
    HRESULT Renderer::initializePipeLine(_In_ HWND hWnd)
    {
        /*
        1. DXGI Factory 생성 - 완료
        2. Device 생성 - 완료
        3. Command Queue & CommandList 생성 - 완료
        4. Swap chain 생성 - 완료
        5. FenceObject 생성 - 완료
        6. RenderTarget과 Denpth-Stencil Buffer 생성 - 완료
        */
        HRESULT hr = S_OK;
        UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG) //디버그 빌드시
        {
            ComPtr<ID3D12Debug> debugController(nullptr);
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
            {
                debugController->EnableDebugLayer();
                dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
            }
        }
#endif
        hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory));
        if (FAILED(hr))
        {
            return hr;
        }
        hr = createDevice();
        if (FAILED(hr))
        {
            return hr;
        }
        hr = createCommandQueue();
        if (FAILED(hr))
        {
            return hr;
        }
        hr = createCommandAllocator();
        if (FAILED(hr))
        {
            return hr;
        }
        hr = createSwapChain(hWnd);
        if (FAILED(hr))
        {
            return hr;
        }
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
        hr = CreateRtvAndDsvDescriptorHeaps();
        if (FAILED(hr))
        {
            return hr;
        }

        for (int i = 0; i < FRAME_COUNT; ++i)
        {
            hr = m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence[i]));
            if (FAILED(hr))
            {
                return hr;
            }
            m_fenceValues[i] = 0;
        }

        hr = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator[m_frameIndex].Get(), nullptr, IID_PPV_ARGS(&m_commandList));
        if (FAILED(hr))
        {
            return hr;
        }

        m_fenceEvent = (CreateEvent(nullptr, FALSE, FALSE, nullptr));
        if (m_fenceEvent == nullptr)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (FAILED(hr))
            {
                return hr;
            }
        }

        hr = createRootSignature();
        if (FAILED(hr))
        {
            return hr;
        }

        hr = compiledShaders();
        if (FAILED(hr))
        {
            return hr;
        }

        return hr;
}

    HRESULT Renderer::initializeAssets()
    {
        HRESULT hr = S_OK;
        // Input Layout Object 구성 (IA Stage Input information)
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // 렌더링 파이프라인 설정
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
            .pRootSignature = m_rootSignature.Get(),                            // 사용할 세이더의 Root Signature                      
            .VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get()),                  // VertexShader 코드
            .PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get()),                   // PixelShader 코드
            .DS = nullptr,
            .HS = nullptr,
            .GS = nullptr,
            .StreamOutput = nullptr,
            .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),                    // Blend 옵션
            .SampleMask = UINT_MAX,
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),          // Rasterize 옵션
            .DepthStencilState = {                                              // Depth-Stencil 옵션
                .DepthEnable = FALSE,
                .StencilEnable = FALSE},
            .InputLayout = {                                                    // VertexShader의 Input으로 들어갈 구조체 정보
                .pInputElementDescs = inputElementDescs,
                .NumElements = _countof(inputElementDescs)},
            .IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,    // 정점들을 어떻게 연결할 것인가.
            .NumRenderTargets = 1u,                                             // 몇개의 렌더 타겟을 그릴 것인가.
            .RTVFormats = {DXGI_FORMAT_R8G8B8A8_UNORM },                        // Render Target View Format
            .DSVFormat = DXGI_FORMAT_R32G32B32A32_FLOAT,                        // Depth Stencil View Format
            .SampleDesc = {.Count = 1, .Quality = 0},
            .NodeMask = 0u,
            .CachedPSO = nullptr,
            .Flags = D3D12_PIPELINE_STATE_FLAG_NONE
        };

        // 렌더링 파이프라인 생성
        hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));   // PSO만들기
        if (FAILED(hr))
        {
            return hr;
        }
        // Vertex Buffer 생성
        Vertex VERTICES[] = {
            { {-0.5f,  0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f} },
            {  {0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 0.0f, 1.0f} },
            { {-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f} },
            {  {0.5f,  0.5f, 0.5f}, {0.0f, 1.0f, 1.0f, 1.0f} }
        };

        const UINT vertexBufferSize = sizeof(VERTICES);

        CD3DX12_HEAP_PROPERTIES vbheapProperties = {};
        D3D12_RESOURCE_DESC vertexDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        vbheapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        hr = m_device->CreateCommittedResource(//힙 크기 == 데이터 크기로 힙과, 자원 할당
            &vbheapProperties,               //힙 타입
            D3D12_HEAP_FLAG_NONE,
            &vertexDesc,                 //자원 크기정보
            D3D12_RESOURCE_STATE_GENERIC_READ,                           //접근 정보
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)                                 //CPU메모리에서 접근가능한 ComPtr개체
        );

        //ComPtr<ID3D12Resource> destinationResource;
        //CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
        //hr = m_device->CreateCommittedResource(//힙 크기 == 데이터 크기로 힙과, 자원 할당
        //    &uploadHeapProperties,               //힙 타입
        //    D3D12_HEAP_FLAG_NONE,
        //    &vertexDesc,                 //자원 크기정보
        //    D3D12_RESOURCE_STATE_GENERIC_READ,                           //접근 정보
        //    nullptr,
        //    IID_PPV_ARGS(&destinationResource)                                 //CPU메모리에서 접근가능한 ComPtr개체
        //);

        UINT8* pVertexDataBegin = nullptr;    // gpu메모리에 mapping 될 cpu메모리(virtual memory로 운영체제 통해 접근하는듯)
        CD3DX12_RANGE readRange(0, 0);        // 0~0으로 설정시 CPU메모리로 gpu데이터 읽기 불허 가능, nullptr입력하면 gpu데이터 읽기 가능
        hr = m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));//매핑
        if (FAILED(hr))
        {
            return hr;
        }
        memcpy(pVertexDataBegin, VERTICES, sizeof(VERTICES));//gpu 메모리 전송
        m_vertexBuffer->Unmap(0, nullptr);//매핑 해제

        //m_commandList->CopyBufferRegion(
        //    m_vertexBuffer.Get(),
        //    0,
        //    destinationResource.Get(),
        //    0,
        //    vertexBufferSize
        //);

        //const D3D12_RESOURCE_BARRIER vertexbufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        //    m_vertexBuffer.Get(),
        //    D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        //);
        //m_commandList->ResourceBarrier(1, &vertexbufferBarrier);

        Index INDICES[] =
        {
            0, 1, 2,
            0, 3, 1
        };

        const UINT indexBufferSize = sizeof(INDICES);

        CD3DX12_HEAP_PROPERTIES ibheapProperties = {};
        D3D12_RESOURCE_DESC indexDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
        ibheapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        hr = m_device->CreateCommittedResource(//힙 크기 == 데이터 크기로 힙과, 자원 할당
            &ibheapProperties,               //힙 타입
            D3D12_HEAP_FLAG_NONE,
            &indexDesc,                 //자원 크기정보
            D3D12_RESOURCE_STATE_GENERIC_READ,                           //접근 정보
            nullptr,
            IID_PPV_ARGS(&m_indexBuffer)
        );
        if (FAILED(hr))
        {
            return hr;
        }

        //ComPtr<ID3D12Resource> destinationResource1;
        //hr = m_device->CreateCommittedResource(//힙 크기 == 데이터 크기로 힙과, 자원 할당
        //    &uploadHeapProperties,               //힙 타입
        //    D3D12_HEAP_FLAG_NONE,
        //    &indexDesc,                 //자원 크기정보
        //    D3D12_RESOURCE_STATE_GENERIC_READ,                           //접근 정보
        //    nullptr,
        //    IID_PPV_ARGS(&destinationResource1)
        //);

        UINT8* pindexDataBegin = nullptr;    // gpu메모리에 mapping 될 cpu메모리(virtual memory로 운영체제 통해 접근하는듯)
        hr = m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pindexDataBegin));//매핑
        if (FAILED(hr))
        {
            return hr;
        }
        memcpy(pindexDataBegin, INDICES, sizeof(INDICES));//gpu 메모리 전송
        m_indexBuffer->Unmap(0, nullptr);//매핑 해제

        //m_commandList->CopyBufferRegion(
        //    m_indexBuffer.Get(),
        //    0,
        //    destinationResource1.Get(),
        //    0,
        //    indexBufferSize
        //);

        //const D3D12_RESOURCE_BARRIER indexbufferBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
        //    m_indexBuffer.Get(),
        //    D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
        //);
        //m_commandList->ResourceBarrier(1, &indexbufferBarrier);

        hr = m_commandList->Close();

        //ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
        //m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists); //command queue에 담긴 command list들 실행명령(비동기)
        //
        //hr = m_commandQueue->Signal(m_fence[m_frameIndex].Get(), m_fenceValues[m_frameIndex]);
        //if (FAILED(hr))
        //{
        //    return hr;
        //}

        m_vertexBufferView = {
            .BufferLocation = m_vertexBuffer->GetGPUVirtualAddress(),   //gpu메모리에 대응하는 cpu virtual address겟
            .SizeInBytes = vertexBufferSize,                            //vertex버퍼 총 크기는?
            .StrideInBytes = sizeof(Vertex)                             //각 vertex는 어떻게 띄어 읽어야하는가?
        };

        m_indexBufferView = {
            .BufferLocation = m_indexBuffer->GetGPUVirtualAddress(),
            .SizeInBytes = indexBufferSize,
            .Format = DXGI_FORMAT_R16_UINT
        };
        return hr;
    }

    // Hardware Adapter Get 함수
    HRESULT Renderer::getHardwareAdapter(_In_ IDXGIFactory1* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter)
    {
        *ppAdapter = nullptr;
        ComPtr<IDXGIAdapter1> adapter(nullptr);
        ComPtr<IDXGIFactory6> factory6(nullptr);
        if (FAILED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))// dxgi factory6를 지원하는가?
        {
            return E_FAIL;
        }
        for (//여러 gpu adapter들 순회
            UINT adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(&adapter)));
            ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc = {
                .Description = {},
                .VendorId = 0u,
                .DeviceId = 0u,
                .SubSysId = 0u,
                .Revision = 0u,
                .DedicatedVideoMemory = 0ul,
                .DedicatedSystemMemory = 0ul,
                .SharedSystemMemory = 0ul,
                .AdapterLuid = {
                    .LowPart = 0,
                    .HighPart = 0l
                },
                .Flags = 0u
            };
            adapter->GetDesc1(&desc);//어뎁터 정보 조회
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)//소프트웨어 어댑터는 취급x
            {
                continue;
            }
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                return S_OK;
            }
        }
    }

    HRESULT Renderer::populateCommandList()
    {
        //command list allocator특: gpu동작 끝나야 Reset가능(펜스로 동기화해라)
        HRESULT hr = S_OK;
        hr = m_commandAllocator[m_frameIndex]->Reset();
        if (FAILED(hr))
        {
            return hr;
        }

        //command list 특: ExecuteCommandList실행하면 언제든지 Reset가능
        hr = m_commandList->Reset(m_commandAllocator[m_frameIndex].Get(), m_pipelineState.Get());
        if (FAILED(hr))
        {
            return hr;
        }

        // 백버퍼를 RT으로 쓸것이라고 전달(barrier)
        const D3D12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_renderTargets[m_frameIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
        );
        m_commandList->ResourceBarrier(1, &resourceBarrier);

        // 뷰포트 Set
        CD3DX12_VIEWPORT viewPort(0.0f, 0.0f, 1920.0f, 1080.0f);
        CD3DX12_RECT scissorRect(0, 0, 1920l, 1080l);
        m_commandList->RSSetViewports(1, &viewPort);
        m_commandList->RSSetScissorRects(1, &scissorRect);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        m_commandList->IASetIndexBuffer(&m_indexBufferView);
        m_commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

        // 백버퍼를 Present용으로 쓸것이라고 전달(barrier)
        const D3D12_RESOURCE_BARRIER resourceBarrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
            m_renderTargets[m_frameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
        );
        m_commandList->ResourceBarrier(1, &resourceBarrier1);

        hr = m_commandList->Close();//command list작성 종료
        if (FAILED(hr))
        {
            return hr;
        }
        return S_OK;
    }

    void Renderer::getWindowWidthHeight(_In_ HWND hWnd, _Out_ PUINT pWidth, _Out_ PUINT pHeight)
    {
        *pWidth = 0u;
        *pHeight = 0u;
        RECT rc = {
            .left = 0u,
            .top = 0u,
            .right = 0u,
            .bottom = 0u
        };
        GetClientRect(hWnd, &rc);
        *pWidth = static_cast<UINT>(rc.right - rc.left);
        *pHeight = static_cast<UINT>(rc.bottom - rc.top);
    }

    // D3D device 생성 함수
    HRESULT Renderer::createDevice()
    {
        HRESULT hr = S_OK;
        ComPtr<IDXGIAdapter1> hardwareAdapter(nullptr);
        getHardwareAdapter(m_dxgiFactory.Get(), &hardwareAdapter);
        hr = D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device));
        if (FAILED(hr))
        {
            return hr;
        }
        return hr;
    }

    // Command Queue 생성 함수
    HRESULT Renderer::createCommandQueue()
    {
        HRESULT hr = S_OK;
        D3D12_COMMAND_QUEUE_DESC queueDesc = {
            .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
            .Priority = 0,
            .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
            .NodeMask = 0u
        };
        hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));
        if (FAILED(hr))
        {
            return hr;
        }
        return hr;
    }

    // Swap Chain 생성 함수
    HRESULT Renderer::createSwapChain(_In_ HWND hWnd)//스왑체인 생성
    {
        HRESULT hr = S_OK;
        UINT width = 0u;
        UINT height = 0u;
        getWindowWidthHeight(hWnd, &width, &height);
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
            .Width = width,                                 //가로
            .Height = height,                               //세로
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,           //후면 버퍼 형식(32bit)
            .Stereo = false,                                //풀 스크린인가?
            .SampleDesc = {
                .Count = 1u,
                .Quality = 0u
            },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT, //백버퍼의 cpu 접근 옵션
            .BufferCount = FRAME_COUNT,                     //버퍼 개수
            .Scaling = DXGI_SCALING_NONE,                   //스케일링 지원여부
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,    //스왑방식
            .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,       //alpha값 사용여부
            .Flags = 0u                                     //각종 flag
        };
        ComPtr<IDXGISwapChain1> swapChain(nullptr);
        hr = m_dxgiFactory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),
            hWnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain
        );
        if (FAILED(hr))
        {
            return hr;
        }
        hr = m_dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);//alt + enter로 전체화면하는거 막기
        if (FAILED(hr))
        {
            return hr;
        }
        hr = swapChain.As(&m_swapChain);
        if (FAILED(hr))
        {
            return hr;
        }
    }

    // RTV & DSV Descriptor 생성 함수
    HRESULT Renderer::CreateRtvAndDsvDescriptorHeaps()
    {
        HRESULT hr = S_OK;

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        //m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        //m_srvuavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        //RTV용 descriptor heap 생성
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,     //렌더타겟뷰 descriptor heap
            .NumDescriptors = FRAME_COUNT,              //FRAME개수 만큼
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            .NodeMask = 0u
        };
        hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
        if (FAILED(hr))
        {
            return hr;
        }

        //RTV생성
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());//첫번째 descriptor가져오기(iterator로 사용됨)
        for (UINT n = 0; n < FRAME_COUNT; n++)//프레임 개수만큼
        {
            hr = m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));//스왑체인에서 버퍼 가져오기
            if (FAILED(hr))
            {
                return hr;
            }
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);
        }

        // DSV용 Descriptor heap 생성
        //D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {
        //    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        //    .NumDescriptors = 1,
        //    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        //    .NodeMask = 0u
        //};
        //hr = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
        //if (FAILED(hr))
        //{
        //    return hr;
        //}

        //// Depth-Stencil Buffer와 View 생성
        //D3D12_RESOURCE_DESC depthStencilDesc = {
        //    .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        //    .Alignment = 0,
        //    .Width = m_ClientWidth,
        //    .Height = m_ClientHeight,
        //    .DepthOrArraySize = 1,
        //    .MipLevels = 1,
        //    .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
        //    .SampleDesc = 0u,
        //    .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        //    .Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
        //};

        //D3D12_CLEAR_VALUE optClear;
        //optClear.Format = depthStencilDesc.Format;
        //optClear.DepthStencil.Depth = 1.0f;
        //optClear.DepthStencil.Stencil = 0;

        //hr = m_device->CreateCommittedResource(
        //    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        //    D3D12_HEAP_FLAG_NONE,
        //    &depthStencilDesc,
        //    D3D12_RESOURCE_STATE_COMMON,
        //    &optClear,
        //    IID_PPV_ARGS(&m_DepthStencilBuffer)
        //);
        //if (FAILED(hr))
        //{
        //    return hr;
        //}

        //CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
        //m_device->CreateDepthStencilView(m_DepthStencilBuffer.Get(), nullptr, dsvHandle);

        return hr;
    }

    // Command Allocator 생성 함수 (command list메모리 할당)
    HRESULT Renderer::createCommandAllocator()
    {
        HRESULT hr = S_OK;
        for (UINT i = 0; i < FRAME_COUNT; i++)
        {
            hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator[i]));
            if (FAILED(hr))
            {
                return hr;
            }
        }
        return hr;
    }

    // RootSignature 생성 함수
    HRESULT Renderer::createRootSignature()
    {
        HRESULT hr = S_OK;
        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {
            .NumParameters = 0,
            .pParameters = nullptr,
            .NumStaticSamplers = 0u,
            .pStaticSamplers = nullptr,
            .Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
        };
        
        hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);// 루트 시그니처의 binary화
        if (FAILED(hr))
        {
            return hr;
        }
        hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));//루트 시그니처 생성
        if (FAILED(hr))
        {
            return hr;
        }

        return hr;
    }

    // CompiledShader 함수
    HRESULT Renderer::compiledShaders()
    {
        HRESULT hr = S_OK;

#if defined(_DEBUG)
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        // 셰이더 컴파일
        hr = D3DCompileFromFile(L"Shaders/Shader.fx", nullptr, nullptr, "VS", "vs_5_0", compileFlags, 0, &vertexShader, nullptr);
        if (FAILED(hr))
        {
            return hr;
        }
        hr = D3DCompileFromFile(L"Shaders/Shader.fx", nullptr, nullptr, "PS", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);
        if (FAILED(hr))
        {
            return hr;
        }

        return hr;
    }

    // CPU & GPU 동기화(Fence) 함수
    HRESULT Renderer::moveToNextFrame()
    {
        HRESULT hr = S_OK;
        //// Schedule a Signal command in the queue.
        hr = m_commandQueue->Signal(m_fence[m_frameIndex].Get(), m_fenceValues[m_frameIndex]);

        // Update the back buffer index.
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        // If the next frame is not ready to be rendered yet, wait until it is ready.
        if (m_fence[m_frameIndex]->GetCompletedValue() < m_fenceValues[m_frameIndex])
        {
            hr = m_fence[m_frameIndex]->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
            if (FAILED(hr))
            {
                return hr;
            }
            WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
        }
        // Set the fence value for the next frame.
        ++m_fenceValues[m_frameIndex];
        return hr;
    }
}