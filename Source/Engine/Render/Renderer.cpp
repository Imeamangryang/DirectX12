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

    // Initialize �Լ� (Graphics Pipeline �ʱ�ȭ)
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

    // Render �Լ� (Command Queue�� �ۼ��ϰ� ������)
    void Renderer::Render(_In_ FLOAT deltaTime)
    {
        populateCommandList();  // command list ���

        ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists); //command queue�� ��� command list�� ������(�񵿱�)

        m_swapChain->Present(1, 0); //����� ��ü

        moveToNextFrame();
    }

    // Graphics Pipeline Initialize �Լ�
    HRESULT Renderer::initializePipeLine(_In_ HWND hWnd)
    {
        /*
        1. DXGI Factory ���� - �Ϸ�
        2. Device ���� - �Ϸ�
        3. Command Queue & CommandList ���� - �Ϸ�
        4. Swap chain ���� - �Ϸ�
        5. FenceObject ���� - �Ϸ�
        6. RenderTarget�� Denpth-Stencil Buffer ���� - �Ϸ�
        */
        HRESULT hr = S_OK;
        UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG) //����� �����
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
        // Input Layout Object ���� (IA Stage Input information)
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // ������ ���������� ����
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
            .pRootSignature = m_rootSignature.Get(),                            // ����� ���̴��� Root Signature                      
            .VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get()),                  // VertexShader �ڵ�
            .PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get()),                   // PixelShader �ڵ�
            .DS = nullptr,
            .HS = nullptr,
            .GS = nullptr,
            .StreamOutput = nullptr,
            .BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT),                    // Blend �ɼ�
            .SampleMask = UINT_MAX,
            .RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),          // Rasterize �ɼ�
            .DepthStencilState = {                                              // Depth-Stencil �ɼ�
                .DepthEnable = FALSE,
                .StencilEnable = FALSE},
            .InputLayout = {                                                    // VertexShader�� Input���� �� ����ü ����
                .pInputElementDescs = inputElementDescs,
                .NumElements = _countof(inputElementDescs)},
            .IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
            .PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,    // �������� ��� ������ ���ΰ�.
            .NumRenderTargets = 1u,                                             // ��� ���� Ÿ���� �׸� ���ΰ�.
            .RTVFormats = {DXGI_FORMAT_R8G8B8A8_UNORM },                        // Render Target View Format
            .DSVFormat = DXGI_FORMAT_R32G32B32A32_FLOAT,                        // Depth Stencil View Format
            .SampleDesc = {.Count = 1, .Quality = 0},
            .NodeMask = 0u,
            .CachedPSO = nullptr,
            .Flags = D3D12_PIPELINE_STATE_FLAG_NONE
        };

        // ������ ���������� ����
        hr = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));   // PSO�����
        if (FAILED(hr))
        {
            return hr;
        }
        // Vertex Buffer ����
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
        hr = m_device->CreateCommittedResource(//�� ũ�� == ������ ũ��� ����, �ڿ� �Ҵ�
            &vbheapProperties,               //�� Ÿ��
            D3D12_HEAP_FLAG_NONE,
            &vertexDesc,                 //�ڿ� ũ������
            D3D12_RESOURCE_STATE_GENERIC_READ,                           //���� ����
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)                                 //CPU�޸𸮿��� ���ٰ����� ComPtr��ü
        );

        //ComPtr<ID3D12Resource> destinationResource;
        //CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
        //hr = m_device->CreateCommittedResource(//�� ũ�� == ������ ũ��� ����, �ڿ� �Ҵ�
        //    &uploadHeapProperties,               //�� Ÿ��
        //    D3D12_HEAP_FLAG_NONE,
        //    &vertexDesc,                 //�ڿ� ũ������
        //    D3D12_RESOURCE_STATE_GENERIC_READ,                           //���� ����
        //    nullptr,
        //    IID_PPV_ARGS(&destinationResource)                                 //CPU�޸𸮿��� ���ٰ����� ComPtr��ü
        //);

        UINT8* pVertexDataBegin = nullptr;    // gpu�޸𸮿� mapping �� cpu�޸�(virtual memory�� �ü�� ���� �����ϴµ�)
        CD3DX12_RANGE readRange(0, 0);        // 0~0���� ������ CPU�޸𸮷� gpu������ �б� ���� ����, nullptr�Է��ϸ� gpu������ �б� ����
        hr = m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));//����
        if (FAILED(hr))
        {
            return hr;
        }
        memcpy(pVertexDataBegin, VERTICES, sizeof(VERTICES));//gpu �޸� ����
        m_vertexBuffer->Unmap(0, nullptr);//���� ����

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
        hr = m_device->CreateCommittedResource(//�� ũ�� == ������ ũ��� ����, �ڿ� �Ҵ�
            &ibheapProperties,               //�� Ÿ��
            D3D12_HEAP_FLAG_NONE,
            &indexDesc,                 //�ڿ� ũ������
            D3D12_RESOURCE_STATE_GENERIC_READ,                           //���� ����
            nullptr,
            IID_PPV_ARGS(&m_indexBuffer)
        );
        if (FAILED(hr))
        {
            return hr;
        }

        //ComPtr<ID3D12Resource> destinationResource1;
        //hr = m_device->CreateCommittedResource(//�� ũ�� == ������ ũ��� ����, �ڿ� �Ҵ�
        //    &uploadHeapProperties,               //�� Ÿ��
        //    D3D12_HEAP_FLAG_NONE,
        //    &indexDesc,                 //�ڿ� ũ������
        //    D3D12_RESOURCE_STATE_GENERIC_READ,                           //���� ����
        //    nullptr,
        //    IID_PPV_ARGS(&destinationResource1)
        //);

        UINT8* pindexDataBegin = nullptr;    // gpu�޸𸮿� mapping �� cpu�޸�(virtual memory�� �ü�� ���� �����ϴµ�)
        hr = m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pindexDataBegin));//����
        if (FAILED(hr))
        {
            return hr;
        }
        memcpy(pindexDataBegin, INDICES, sizeof(INDICES));//gpu �޸� ����
        m_indexBuffer->Unmap(0, nullptr);//���� ����

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
        //m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists); //command queue�� ��� command list�� ������(�񵿱�)
        //
        //hr = m_commandQueue->Signal(m_fence[m_frameIndex].Get(), m_fenceValues[m_frameIndex]);
        //if (FAILED(hr))
        //{
        //    return hr;
        //}

        m_vertexBufferView = {
            .BufferLocation = m_vertexBuffer->GetGPUVirtualAddress(),   //gpu�޸𸮿� �����ϴ� cpu virtual address��
            .SizeInBytes = vertexBufferSize,                            //vertex���� �� ũ���?
            .StrideInBytes = sizeof(Vertex)                             //�� vertex�� ��� ��� �о���ϴ°�?
        };

        m_indexBufferView = {
            .BufferLocation = m_indexBuffer->GetGPUVirtualAddress(),
            .SizeInBytes = indexBufferSize,
            .Format = DXGI_FORMAT_R16_UINT
        };
        return hr;
    }

    // Hardware Adapter Get �Լ�
    HRESULT Renderer::getHardwareAdapter(_In_ IDXGIFactory1* pFactory, _Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter)
    {
        *ppAdapter = nullptr;
        ComPtr<IDXGIAdapter1> adapter(nullptr);
        ComPtr<IDXGIFactory6> factory6(nullptr);
        if (FAILED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))// dxgi factory6�� �����ϴ°�?
        {
            return E_FAIL;
        }
        for (//���� gpu adapter�� ��ȸ
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
            adapter->GetDesc1(&desc);//��� ���� ��ȸ
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)//����Ʈ���� ����ʹ� ���x
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
        //command list allocatorƯ: gpu���� ������ Reset����(�潺�� ����ȭ�ض�)
        HRESULT hr = S_OK;
        hr = m_commandAllocator[m_frameIndex]->Reset();
        if (FAILED(hr))
        {
            return hr;
        }

        //command list Ư: ExecuteCommandList�����ϸ� �������� Reset����
        hr = m_commandList->Reset(m_commandAllocator[m_frameIndex].Get(), m_pipelineState.Get());
        if (FAILED(hr))
        {
            return hr;
        }

        // ����۸� RT���� �����̶�� ����(barrier)
        const D3D12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_renderTargets[m_frameIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET
        );
        m_commandList->ResourceBarrier(1, &resourceBarrier);

        // ����Ʈ Set
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

        // ����۸� Present������ �����̶�� ����(barrier)
        const D3D12_RESOURCE_BARRIER resourceBarrier1 = CD3DX12_RESOURCE_BARRIER::Transition(
            m_renderTargets[m_frameIndex].Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT
        );
        m_commandList->ResourceBarrier(1, &resourceBarrier1);

        hr = m_commandList->Close();//command list�ۼ� ����
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

    // D3D device ���� �Լ�
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

    // Command Queue ���� �Լ�
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

    // Swap Chain ���� �Լ�
    HRESULT Renderer::createSwapChain(_In_ HWND hWnd)//����ü�� ����
    {
        HRESULT hr = S_OK;
        UINT width = 0u;
        UINT height = 0u;
        getWindowWidthHeight(hWnd, &width, &height);
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
            .Width = width,                                 //����
            .Height = height,                               //����
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,           //�ĸ� ���� ����(32bit)
            .Stereo = false,                                //Ǯ ��ũ���ΰ�?
            .SampleDesc = {
                .Count = 1u,
                .Quality = 0u
            },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT, //������� cpu ���� �ɼ�
            .BufferCount = FRAME_COUNT,                     //���� ����
            .Scaling = DXGI_SCALING_NONE,                   //�����ϸ� ��������
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,    //���ҹ��
            .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,       //alpha�� ��뿩��
            .Flags = 0u                                     //���� flag
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
        hr = m_dxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER);//alt + enter�� ��üȭ���ϴ°� ����
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

    // RTV & DSV Descriptor ���� �Լ�
    HRESULT Renderer::CreateRtvAndDsvDescriptorHeaps()
    {
        HRESULT hr = S_OK;

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        //m_dsvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        //m_srvuavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        //RTV�� descriptor heap ����
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
            .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,     //����Ÿ�ٺ� descriptor heap
            .NumDescriptors = FRAME_COUNT,              //FRAME���� ��ŭ
            .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
            .NodeMask = 0u
        };
        hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
        if (FAILED(hr))
        {
            return hr;
        }

        //RTV����
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());//ù��° descriptor��������(iterator�� ����)
        for (UINT n = 0; n < FRAME_COUNT; n++)//������ ������ŭ
        {
            hr = m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));//����ü�ο��� ���� ��������
            if (FAILED(hr))
            {
                return hr;
            }
            m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);
        }

        // DSV�� Descriptor heap ����
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

        //// Depth-Stencil Buffer�� View ����
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

    // Command Allocator ���� �Լ� (command list�޸� �Ҵ�)
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

    // RootSignature ���� �Լ�
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
        
        hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);// ��Ʈ �ñ״�ó�� binaryȭ
        if (FAILED(hr))
        {
            return hr;
        }
        hr = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));//��Ʈ �ñ״�ó ����
        if (FAILED(hr))
        {
            return hr;
        }

        return hr;
    }

    // CompiledShader �Լ�
    HRESULT Renderer::compiledShaders()
    {
        HRESULT hr = S_OK;

#if defined(_DEBUG)
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        // ���̴� ������
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

    // CPU & GPU ����ȭ(Fence) �Լ�
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