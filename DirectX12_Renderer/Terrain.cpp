#include "Terrain.h"

Terrain::Terrain(Graphics* renderer) :
	m_pipelineStateTes(nullptr),
	m_pipelineStateTes2(nullptr),
	m_pipelineState3D(nullptr),
	m_pipelineState2D(nullptr),
	m_rootSignatureTes(nullptr),
	m_rootSignatureTes2(nullptr),
	m_rootSignature3D(nullptr),
	m_rootSignature2D(nullptr),
	m_srvHeap(nullptr),
	m_uploadHeap(nullptr),
	m_image(),
	m_width(0),
	m_height(0),
	m_CBV(nullptr),
	m_vertexBuffer(nullptr),
	m_vertexBufferUpload(nullptr),
	m_indexBuffer(nullptr),
	m_indexBufferUpload(nullptr),
	m_orbitCycle(5760)
{
	LoadHeightMap(renderer, L"ldem_16.tif", L"lroc_color_poles_4k.tif");
	
	//InitPipeline2D(renderer);
	//InitPipeline3D(renderer);
	InitPipelineTes(renderer);
	InitPipelineTes_Wireframe(renderer);

	CreateGeosphere(renderer, 1737, 10);
	//CreateGeosphere(Renderer, 17374, 10);
}

Terrain::~Terrain()
{
	if (m_srvHeap)
	{
		m_srvHeap->Release();
		m_srvHeap = nullptr;
	}
	if (m_uploadHeap)
	{
		m_uploadHeap->Release();
		m_uploadHeap = nullptr;
	}
	if (m_indexBufferUpload)
	{
		m_indexBufferUpload->Release();
		m_indexBufferUpload = nullptr;
	}
	if (m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = nullptr;
	}
	if (m_vertexBufferUpload)
	{
		m_vertexBufferUpload->Release();
		m_vertexBufferUpload = nullptr;
	}
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = nullptr;
	}
	if (m_pipelineStateTes)
	{
		m_pipelineStateTes->Release();
		m_pipelineStateTes = nullptr;
	}
	if (m_pipelineStateTes2)
	{
		m_pipelineStateTes2->Release();
		m_pipelineStateTes2 = nullptr;
	}
	if (m_pipelineState3D)
	{
		m_pipelineState3D->Release();
		m_pipelineState3D = nullptr;
	}
	if (m_pipelineState2D)
	{
		m_pipelineState2D->Release();
		m_pipelineState2D = nullptr;
	}
	if (m_rootSignatureTes)
	{
		m_rootSignatureTes->Release();
		m_rootSignatureTes = nullptr;
	}
	if (m_rootSignatureTes2)
	{
		m_rootSignatureTes2->Release();
		m_rootSignatureTes2 = nullptr;
	}
	if (m_rootSignature3D)
	{
		m_rootSignature3D->Release();
		m_rootSignature3D = nullptr;
	}
	if (m_rootSignature2D)
	{
		m_rootSignature2D->Release();
		m_rootSignature2D = nullptr;
	}
	if (m_CBV)
	{
		m_CBV->Unmap(0, nullptr);
		m_cbvDataBegin = nullptr;
		m_CBV->Release();
		m_CBV = nullptr;
	}
}

void Terrain::DrawTes(ID3D12GraphicsCommandList* m_commandList, XMFLOAT4X4 viewproj, XMFLOAT4 eye)
{
	m_commandList->SetPipelineState(m_pipelineStateTes);
	m_commandList->SetGraphicsRootSignature(m_rootSignatureTes);

	m_orbitCycle.Update();

	m_constantBufferData.viewproj = viewproj;
	m_constantBufferData.eye = eye;
	m_constantBufferData.height = m_height;
	m_constantBufferData.width = m_width;
	m_constantBufferData.light = m_orbitCycle.GetLight();
	memcpy(m_cbvDataBegin, &m_constantBufferData, sizeof(ConstantBuffer));

	ID3D12DescriptorHeap* heaps[] = { m_srvHeap };
	m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_srvDescSize);
	m_commandList->SetGraphicsRootDescriptorTable(1, cbvHandle);
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvhandle2(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 2, m_srvDescSize);
	m_commandList->SetGraphicsRootDescriptorTable(2, srvhandle2);

	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST); // describe how to read the vertex buffer.
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->IASetIndexBuffer(&m_indexBufferView);

	m_commandList->DrawIndexedInstanced(m_indexcount, 1, 0, 0, 0);
}

void Terrain::DrawTes_Wireframe(ID3D12GraphicsCommandList* m_commandList, XMFLOAT4X4 viewproj, XMFLOAT4 eye)
{
	m_commandList->SetPipelineState(m_pipelineStateTes2);
	m_commandList->SetGraphicsRootSignature(m_rootSignatureTes2);

	m_orbitCycle.Update();

	m_constantBufferData.viewproj = viewproj;
	m_constantBufferData.eye = eye;
	m_constantBufferData.height = m_height;
	m_constantBufferData.width = m_width;
	m_constantBufferData.light = m_orbitCycle.GetLight();
	memcpy(m_cbvDataBegin, &m_constantBufferData, sizeof(ConstantBuffer));

	ID3D12DescriptorHeap* heaps[] = { m_srvHeap };
	m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_srvDescSize);
	m_commandList->SetGraphicsRootDescriptorTable(1, cbvHandle);
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvhandle2(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 2, m_srvDescSize);
	m_commandList->SetGraphicsRootDescriptorTable(2, srvhandle2);

	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST); // describe how to read the vertex buffer.
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->IASetIndexBuffer(&m_indexBufferView);

	m_commandList->DrawIndexedInstanced(m_indexcount, 1, 0, 0, 0);
}

void Terrain::Draw3D(ID3D12GraphicsCommandList* m_commandList, XMFLOAT4X4 viewproj, XMFLOAT4 eye)
{
	m_commandList->SetPipelineState(m_pipelineState3D);
	m_commandList->SetGraphicsRootSignature(m_rootSignature3D);

	m_constantBufferData.viewproj = viewproj;
	m_constantBufferData.eye = eye;
	m_constantBufferData.height = m_height;
	m_constantBufferData.width = m_width;
	memcpy(m_cbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

	ID3D12DescriptorHeap* heaps[] = { m_srvHeap };
	m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	m_commandList->SetGraphicsRootDescriptorTable(0, m_srvHeap->GetGPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_srvDescSize);
	m_commandList->SetGraphicsRootDescriptorTable(1, cbvHandle);
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvhandle2(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 2, m_srvDescSize);
	m_commandList->SetGraphicsRootDescriptorTable(2, srvhandle2);
	
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // describe how to read the vertex buffer.
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->IASetIndexBuffer(&m_indexBufferView);

	m_commandList->DrawIndexedInstanced(m_indexcount, 1, 0, 0, 0);
}

void Terrain::Draw2D(ID3D12GraphicsCommandList* m_commandList)
{
	m_commandList->SetPipelineState(m_pipelineState2D);
	m_commandList->SetGraphicsRootSignature(m_rootSignature2D);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // describe how to read the vertex buffer.
	ID3D12DescriptorHeap* heaps[] = { m_srvHeap };
	m_commandList->SetDescriptorHeaps(1, heaps);

	CD3DX12_GPU_DESCRIPTOR_HANDLE srvhandle(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 0, m_srvDescSize);
	m_commandList->SetGraphicsRootDescriptorTable(0, srvhandle);

	CD3DX12_GPU_DESCRIPTOR_HANDLE srvhandle2(m_srvHeap->GetGPUDescriptorHandleForHeapStart(), 2, m_srvDescSize);
	m_commandList->SetGraphicsRootDescriptorTable(2, srvhandle2);


	m_commandList->DrawInstanced(3, 1, 0, 0);
}

void Terrain::ClearUnusedUploadBuffersAfterInit()
{
	if (m_uploadHeap)
	{
		m_uploadHeap->Release();
		m_uploadHeap = nullptr;
	}
	if (m_indexBufferUpload)
	{
		m_indexBufferUpload->Release();
		m_indexBufferUpload = nullptr;
	}
	if (m_vertexBufferUpload) 
	{
		m_vertexBufferUpload->Release();
		m_vertexBufferUpload = nullptr;
	}
}

void Terrain::InitPipelineTes(Graphics* Renderer)
{
	CD3DX12_DESCRIPTOR_RANGE range[3];
	CD3DX12_ROOT_PARAMETER paramsRoot[3];
	// Root Signature 积己
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	paramsRoot[0].InitAsDescriptorTable(1, &range[0]);

	// ConstantBufferview甫 困茄 Root Parameter
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	paramsRoot[1].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_ALL);

	// Slot3 : Color Map, Register(t1)
	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	paramsRoot[2].InitAsDescriptorTable(1, &range[2]);

	CD3DX12_STATIC_SAMPLER_DESC descSamplers[2];
	descSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
	descSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	descSamplers[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
	descSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_ROOT_SIGNATURE_DESC rootDesc;
	rootDesc.Init(
		_countof(paramsRoot),
		paramsRoot,
		2,
		descSamplers,
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	Renderer->createRootSignature(&rootDesc, m_rootSignatureTes);

	CreateConstantBuffer(Renderer);

	// Shader Compile
	D3D12_SHADER_BYTECODE PSBytecode = {};
	D3D12_SHADER_BYTECODE VSBytecode = {};
	D3D12_SHADER_BYTECODE HSBytecode = {};
	D3D12_SHADER_BYTECODE DSBytecode = {};
	Renderer->CompileShader(L"VertexShaderTes.hlsl", "VSTes", VSBytecode, VERTEX_SHADER);
	Renderer->CompileShader(L"PixelShaderTes.hlsl", "PSTes", PSBytecode, PIXEL_SHADER);
	Renderer->CompileShader(L"HullShader.hlsl", "HS", HSBytecode, HULL_SHADER);
	Renderer->CompileShader(L"DomainShader.hlsl", "DS", DSBytecode, DOMAIN_SHADER);

	// Input Layout 积己
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_INPUT_LAYOUT_DESC	inputLayoutDesc = {};
	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.pRootSignature = m_rootSignatureTes;
	psoDesc.VS = VSBytecode;
	psoDesc.PS = PSBytecode;
	psoDesc.HS = HSBytecode;
	psoDesc.DS = DSBytecode;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	Renderer->createPSO(&psoDesc, m_pipelineStateTes);
}

void Terrain::InitPipelineTes_Wireframe(Graphics* Renderer)
{
	CD3DX12_DESCRIPTOR_RANGE range[3];
	CD3DX12_ROOT_PARAMETER paramsRoot[3];
	// Root Signature 积己
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	paramsRoot[0].InitAsDescriptorTable(1, &range[0]);

	// ConstantBufferview甫 困茄 Root Parameter
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	paramsRoot[1].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_ALL);

	// Slot3 : Color Map, Register(t1)
	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	paramsRoot[2].InitAsDescriptorTable(1, &range[2]);

	CD3DX12_STATIC_SAMPLER_DESC descSamplers[2];
	descSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
	descSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	descSamplers[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
	descSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_ROOT_SIGNATURE_DESC rootDesc;
	rootDesc.Init(
		_countof(paramsRoot),
		paramsRoot,
		2,
		descSamplers,
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	Renderer->createRootSignature(&rootDesc, m_rootSignatureTes2);

	CreateConstantBuffer(Renderer);

	// Shader Compile
	D3D12_SHADER_BYTECODE PSBytecode = {};
	D3D12_SHADER_BYTECODE VSBytecode = {};
	D3D12_SHADER_BYTECODE HSBytecode = {};
	D3D12_SHADER_BYTECODE DSBytecode = {};
	Renderer->CompileShader(L"VertexShaderTes.hlsl", "VSTes", VSBytecode, VERTEX_SHADER);
	Renderer->CompileShader(L"PixelShaderTes.hlsl", "PSTes", PSBytecode, PIXEL_SHADER);
	Renderer->CompileShader(L"HullShader.hlsl", "HS", HSBytecode, HULL_SHADER);
	Renderer->CompileShader(L"DomainShader.hlsl", "DS", DSBytecode, DOMAIN_SHADER);

	// Input Layout 积己
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		//{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_INPUT_LAYOUT_DESC	inputLayoutDesc = {};
	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.pRootSignature = m_rootSignatureTes2;
	psoDesc.VS = VSBytecode;
	psoDesc.PS = PSBytecode;
	psoDesc.HS = HSBytecode;
	psoDesc.DS = DSBytecode;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	Renderer->createPSO(&psoDesc, m_pipelineStateTes2);
}

void Terrain::InitPipeline3D(Graphics* Renderer)
{
	// Root Signature 积己
	CD3DX12_DESCRIPTOR_RANGE range[3];
	CD3DX12_ROOT_PARAMETER paramsRoot[3];
	// Slot : Displacement Map, Register(t0)
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	paramsRoot[0].InitAsDescriptorTable(1, &range[0]);
	
	// ConstantBufferview甫 困茄 Root Parameter
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	paramsRoot[1].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_ALL);

	// Slot : Color Map, Register(t1)
	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	paramsRoot[2].InitAsDescriptorTable(1, &range[2]);
	
	CD3DX12_STATIC_SAMPLER_DESC descSamplers[2];
	descSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
	descSamplers[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

	CD3DX12_ROOT_SIGNATURE_DESC rootDesc;
	rootDesc.Init(
		_countof(paramsRoot),
		paramsRoot,
		2,
		descSamplers,
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	Renderer->createRootSignature(&rootDesc, m_rootSignature3D);

	CreateConstantBuffer(Renderer);

	// Shader Compile
	D3D12_SHADER_BYTECODE PSBytecode = {};
	D3D12_SHADER_BYTECODE VSBytecode = {};
	Renderer->CompileShader(L"VertexShader.hlsl", "VS", VSBytecode, VERTEX_SHADER);
	Renderer->CompileShader(L"PixelShader.hlsl", "PS", PSBytecode, PIXEL_SHADER);

	// Input Layout 积己
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = 
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	D3D12_INPUT_LAYOUT_DESC	inputLayoutDesc = {};
	inputLayoutDesc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	inputLayoutDesc.pInputElementDescs = inputLayout;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_rootSignature3D;
	psoDesc.InputLayout = inputLayoutDesc;
	psoDesc.VS = VSBytecode;
	psoDesc.PS = PSBytecode;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	Renderer->createPSO(&psoDesc, m_pipelineState3D);
}

void Terrain::InitPipeline2D(Graphics* Renderer)
{
	// Root Signature 积己
	CD3DX12_DESCRIPTOR_RANGE range[3];
	CD3DX12_ROOT_PARAMETER paramsRoot[3];
	// Slot : Displacement Map, Register(t0)
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	paramsRoot[0].InitAsDescriptorTable(1, &range[0]);

	// ConstantBufferview甫 困茄 Root Parameter
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	paramsRoot[1].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_ALL);

	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	paramsRoot[2].InitAsDescriptorTable(1, &range[2]);

	CD3DX12_STATIC_SAMPLER_DESC descSamplers[2];
	descSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
	descSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	descSamplers[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
	descSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	CD3DX12_ROOT_SIGNATURE_DESC rootDesc;
	rootDesc.Init(
		_countof(paramsRoot),
		paramsRoot,
		2,
		descSamplers,
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS);

	Renderer->createRootSignature(&rootDesc, m_rootSignature2D);

	// Shader Compile
	D3D12_SHADER_BYTECODE PSBytecode = {};
	D3D12_SHADER_BYTECODE VSBytecode = {};
	Renderer->CompileShader(L"VertexShader2D.hlsl", "VS2D", VSBytecode, VERTEX_SHADER);
	Renderer->CompileShader(L"PixelShader2D.hlsl", "PS2D", PSBytecode, PIXEL_SHADER);

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = m_rootSignature2D;
	psoDesc.VS = VSBytecode;
	psoDesc.PS = PSBytecode;
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	psoDesc.DepthStencilState.DepthEnable = false;
	psoDesc.DepthStencilState.StencilEnable = false;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;

	Renderer->createPSO(&psoDesc, m_pipelineState2D);

}

void Terrain::CreateConstantBuffer(Graphics* Renderer)
{
	UINT64 bufferSize = sizeof(ConstantBuffer);
	Renderer->CreateBuffer(m_CBV, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize));
	m_CBV->SetName(L"CBV");

	// ConstantBufferView 积己
	D3D12_CONSTANT_BUFFER_VIEW_DESC	cbvDesc = {};
	cbvDesc.BufferLocation = m_CBV->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (bufferSize + 255) & ~255; // Constant Buffer绰 256 byte aligned

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_srvDescSize);

	Renderer->CreateCBV(&cbvDesc, srvHandle);

	ZeroMemory(&m_constantBufferData, sizeof(m_constantBufferData));

	CD3DX12_RANGE readRange(0, 0); 
	if (FAILED(m_CBV->Map(0, &readRange, reinterpret_cast<void**>(&m_cbvDataBegin)))) 
	{
		throw (GFX_Exception("Failed to map CBV in Terrain."));
	}
}

void Terrain::CreateMesh3D(Graphics* Renderer)
{
	int height = m_height;
	int width = m_width;
	int arraysize = height * width;

	// Vertex Buffer 积己
	Vertex1 *vertices = new Vertex1[arraysize];
	for (int y = 0; y < height; ++y) 
	{
		for (int x = 0; x < width; ++x) 
		{
			vertices[y * width + x].position = XMFLOAT3((float)x, (float)y, 0.0f);
		}
	}

	int bufferSize = sizeof(Vertex1) * arraysize;

	Renderer->CreateCommittedBuffer(m_vertexBuffer, m_vertexBufferUpload, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize));

	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = vertices;
	vertexData.RowPitch = bufferSize;
	vertexData.SlicePitch = bufferSize;

	UpdateSubresources(Renderer->GetCommandList(), m_vertexBuffer, m_vertexBufferUpload, 0, 0, 1, &vertexData);
	Renderer->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex1);
	m_vertexBufferView.SizeInBytes = bufferSize;


	// Index Buffer 积己
	arraysize = (m_width - 1) * (m_height - 1) * 6;

	UINT* indices = new UINT[arraysize];
	int i = 0;
	for (int y = 0; y < m_height - 1; ++y)
	{
		for (int x = 0; x < m_width - 1; ++x)
		{
			indices[i++] = x + y * m_width;
			indices[i++] = x + 1 + y * m_width;
			indices[i++] = x + (y + 1) * m_width;

			indices[i++] = x + 1 + y * m_width;
			indices[i++] = x + 1 + (y + 1) * m_width;
			indices[i++] = x + (y + 1) * m_width;
		}
	}

	bufferSize = sizeof(UINT) * arraysize;

	Renderer->CreateCommittedBuffer(m_indexBuffer, m_indexBufferUpload, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize));

	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = indices;
	indexData.RowPitch = bufferSize;
	indexData.SlicePitch = bufferSize;

	UpdateSubresources(Renderer->GetCommandList(), m_indexBuffer, m_indexBufferUpload, 0, 0, 1, &indexData);
	Renderer->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = bufferSize;

	m_indexcount = arraysize;
}

void Terrain::LoadHeightMap(Graphics* Renderer, const wchar_t* displacementmap, const wchar_t* colormap)
{
	// SRV Discriptor Heap 积己
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 3;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	Renderer->CreateDescriptorHeap(&srvHeapDesc, m_srvHeap);

	m_srvDescSize = Renderer->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Displacement Map 且寸
	std::unique_ptr<uint8_t[]> displacementMapdecodedData;
	ID3D12Resource* displacementMap;
	D3D12_SUBRESOURCE_DATA displacementMapData;

	D3D12_SHADER_RESOURCE_VIEW_DESC	srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//srvDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	LoadWICTextureFromFileEx(Renderer->GetDevice(), displacementmap, 0, D3D12_RESOURCE_FLAG_NONE, WIC_LOADER_FORCE_RGBA32, &displacementMap, displacementMapdecodedData, displacementMapData);

	// Color Map 且寸
	std::unique_ptr<uint8_t[]> colorMapdecodedData;
	ID3D12Resource* colorMap;
	D3D12_SUBRESOURCE_DATA colorMapData;

	D3D12_SHADER_RESOURCE_VIEW_DESC colorsrvDesc = {};
	colorsrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	colorsrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	colorsrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	colorsrvDesc.Texture2D.MipLevels = 1;

	LoadWICTextureFromFileEx(Renderer->GetDevice(), colormap, 0, D3D12_RESOURCE_FLAG_NONE, WIC_LOADER_FORCE_RGBA32, &colorMap, colorMapdecodedData, colorMapData);

	D3D12_RESOURCE_DESC texDesc = displacementMap->GetDesc();
	m_width = texDesc.Width;
	m_height = texDesc.Height;

	const UINT64 displacementMapSize = GetRequiredIntermediateSize(displacementMap, 0, 1);
	const UINT64 colorMapSize = GetRequiredIntermediateSize(colorMap, 0, 1);
		
	Renderer->GetDevice()->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(displacementMapSize + colorMapSize), D3D12_RESOURCE_STATE_GENERIC_READ,
		NULL, IID_PPV_ARGS(&m_uploadHeap));

	//const unsigned int subresourceCount = texDesc.DepthOrArraySize * texDesc.MipLevels;
	UpdateSubresources(Renderer->GetCommandList(), displacementMap, m_uploadHeap, 0, 0, 1, &displacementMapData);
	Renderer->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(displacementMap, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
	CD3DX12_CPU_DESCRIPTOR_HANDLE handleSRV(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 0, m_srvDescSize);
	Renderer->CreateSRV(displacementMap, &srvDesc, handleSRV);

	UpdateSubresources(Renderer->GetCommandList(), colorMap, m_uploadHeap, displacementMapSize, 0, 1, &colorMapData);
	Renderer->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(colorMap, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
	CD3DX12_CPU_DESCRIPTOR_HANDLE colorhandle(m_srvHeap->GetCPUDescriptorHandleForHeapStart(), 2, m_srvDescSize);
	Renderer->CreateSRV(colorMap, &colorsrvDesc, colorhandle);
}

void Terrain::CreateSphere(Graphics* Renderer, float radius, UINT slice, UINT stack)
{
	std::vector<Vertex> vertices;

	Vertex topVertex(0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	Vertex bottomVertex(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	vertices.push_back(topVertex);

	float phiStep = XM_PI / stack;
	float thetaStep = 2.0f * XM_PI / slice;

	for(int i = 1; i <= stack-1; ++i)
	{
		float phi = i*phiStep;

		// Vertices of ring.
        for(int j = 0; j <= slice; ++j)
		{
			float theta = j*thetaStep;

			Vertex v;

			// spherical to cartesian
			v.Position.x = radius*sinf(phi)*cosf(theta);
			v.Position.y = radius*cosf(phi);
			v.Position.z = radius*sinf(phi)*sinf(theta);

			// Partial derivative of P with respect to theta
			v.TangentU.x = -radius*sinf(phi)*sinf(theta);
			v.TangentU.y = 0.0f;
			v.TangentU.z = +radius*sinf(phi)*cosf(theta);

			XMVECTOR T = XMLoadFloat3(&v.TangentU);
			XMStoreFloat3(&v.TangentU, XMVector3Normalize(T));

			XMVECTOR p = XMLoadFloat3(&v.Position);
			XMStoreFloat3(&v.Normal, XMVector3Normalize(p));

			v.TexC.x = theta / XM_2PI;
			v.TexC.y = phi / XM_PI;

			vertices.push_back( v );
		}
	}

	//vertices.push_back(bottomVertex);

	int bufferSize = sizeof(Vertex) * vertices.size();

	Renderer->CreateCommittedBuffer(m_vertexBuffer, m_vertexBufferUpload, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize));

	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = &vertices[0];
	vertexData.RowPitch = bufferSize;
	vertexData.SlicePitch = bufferSize;

	UpdateSubresources(Renderer->GetCommandList(), m_vertexBuffer, m_vertexBufferUpload, 0, 0, 1, &vertexData);
	Renderer->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = bufferSize;

	std::vector<UINT> indices;

	for (int i = 1; i <= slice; ++i)
	{
		indices.push_back(0);
		indices.push_back(i + 1);
		indices.push_back(i);
	}

	int baseIndex = 1;
	int ringVertexCount = slice + 1;
	for (int i = 0; i < stack - 2; ++i)
	{
		for (int j = 0; j < slice; ++j)
		{
			indices.push_back(baseIndex + i * ringVertexCount + j);
			indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);

			indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
			indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
		}
	}

	int southPoleIndex = vertices.size() - 1;
	baseIndex = southPoleIndex = ringVertexCount;
	for (int i = 0; i < slice; ++i)
	{
		indices.push_back(southPoleIndex);
		indices.push_back(baseIndex + i);
		indices.push_back(baseIndex + i + 1);
	}

	bufferSize = sizeof(UINT) * indices.size();

	Renderer->CreateCommittedBuffer(m_indexBuffer, m_indexBufferUpload, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize));

	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = &indices[0];
	indexData.RowPitch = bufferSize;
	indexData.SlicePitch = bufferSize;

	UpdateSubresources(Renderer->GetCommandList(), m_indexBuffer, m_indexBufferUpload, 0, 0, 1, &indexData);
	Renderer->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = bufferSize;

	m_indexcount = indices.size();
}

void Terrain::CreateGeosphere(Graphics* Renderer, float radius, UINT numSubdivisions)
{
	numSubdivisions = min(numSubdivisions, 5u);

	const float X = 0.525731f;
	const float Z = 0.850651f;

	XMFLOAT3 pos[12] =
	{
		XMFLOAT3(-X, 0.0f, Z),  XMFLOAT3(X, 0.0f, Z),
		XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),
		XMFLOAT3(0.0f, Z, X),   XMFLOAT3(0.0f, Z, -X),
		XMFLOAT3(0.0f, -Z, X),  XMFLOAT3(0.0f, -Z, -X),
		XMFLOAT3(Z, X, 0.0f),   XMFLOAT3(-Z, X, 0.0f),
		XMFLOAT3(Z, -X, 0.0f),  XMFLOAT3(-Z, -X, 0.0f)
	};

	DWORD k[60] =
	{
		1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0,
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7
	};

	std::vector<Vertex> vertices;
	std::vector<UINT> indices;

	vertices.resize(12);
	indices.resize(60);
	
	for (UINT i = 0; i < 12; ++i)
	{
		vertices[i].Position = pos[i];
	}

	for (UINT i = 0; i < 60; ++i)
	{
		indices[i] = k[i];
	}

	for (UINT i = 0; i < numSubdivisions; ++i)
	{
		std::vector<Vertex> tempvertices = vertices;
		std::vector<UINT> tempindices = indices;

		vertices.resize(0);
		indices.resize(0);

		//       v1
		//       *
		//      / \
		//     /   \
		//  m0*-----*m1
		//   / \   / \
		//  /   \ /   \
		// *-----*-----*
		// v0    m2     v2

		UINT numTris = tempindices.size() / 3;
		for (UINT i = 0; i < numTris; ++i)
		{
			Vertex v0 = tempvertices[tempindices[i * 3 + 0]];
			Vertex v1 = tempvertices[tempindices[i * 3 + 1]];
			Vertex v2 = tempvertices[tempindices[i * 3 + 2]];

			//
			// Generate the midpoints.
			//

			Vertex m0, m1, m2;

			// For subdivision, we just care about the position component.  We derive the other
			// vertex components in CreateGeosphere.

			m0.Position = XMFLOAT3(
				0.5f * (v0.Position.x + v1.Position.x),
				0.5f * (v0.Position.y + v1.Position.y),
				0.5f * (v0.Position.z + v1.Position.z));

			m1.Position = XMFLOAT3(
				0.5f * (v1.Position.x + v2.Position.x),
				0.5f * (v1.Position.y + v2.Position.y),
				0.5f * (v1.Position.z + v2.Position.z));

			m2.Position = XMFLOAT3(
				0.5f * (v0.Position.x + v2.Position.x),
				0.5f * (v0.Position.y + v2.Position.y),
				0.5f * (v0.Position.z + v2.Position.z));

			//
			// Add new geometry.
			//

			vertices.push_back(v0); // 0
			vertices.push_back(v1); // 1
			vertices.push_back(v2); // 2
			vertices.push_back(m0); // 3
			vertices.push_back(m1); // 4
			vertices.push_back(m2); // 5

			indices.push_back(i * 6 + 0);
			indices.push_back(i * 6 + 3);
			indices.push_back(i * 6 + 5);

			indices.push_back(i * 6 + 3);
			indices.push_back(i * 6 + 4);
			indices.push_back(i * 6 + 5);

			indices.push_back(i * 6 + 5);
			indices.push_back(i * 6 + 4);
			indices.push_back(i * 6 + 2);

			indices.push_back(i * 6 + 3);
			indices.push_back(i * 6 + 1);
			indices.push_back(i * 6 + 4);
		}
	}

	// Project vertices onto sphere and scale.
	for (UINT i = 0; i < vertices.size(); ++i)
	{
		// Project onto unit sphere.
		XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&vertices[i].Position));

		// Project onto sphere.
		XMVECTOR p = radius * n;

		XMStoreFloat3(&vertices[i].Position, p);
		XMStoreFloat3(&vertices[i].Normal, n);

		// Derive texture coordinates from spherical coordinates.
		float theta = MathHelper::AngleFromXY(
			vertices[i].Position.x,
			vertices[i].Position.z);
		

		float phi = acosf(vertices[i].Position.y / radius);

		vertices[i].TexC.x = theta / XM_2PI;
		vertices[i].TexC.y = phi / XM_PI;

		// Partial derivative of P with respect to theta
		vertices[i].TangentU.x = -radius * sinf(phi) * sinf(theta);
		vertices[i].TangentU.y = 0.0f;
		vertices[i].TangentU.z = +radius * sinf(phi) * cosf(theta);

		XMVECTOR T = XMLoadFloat3(&vertices[i].TangentU);
		XMStoreFloat3(&vertices[i].TangentU, XMVector3Normalize(T));
	}

	int bufferSize = sizeof(Vertex) * vertices.size();

	Renderer->CreateCommittedBuffer(m_vertexBuffer, m_vertexBufferUpload, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize));

	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = &vertices[0];
	vertexData.RowPitch = bufferSize;
	vertexData.SlicePitch = bufferSize;

	UpdateSubresources(Renderer->GetCommandList(), m_vertexBuffer, m_vertexBufferUpload, 0, 0, 1, &vertexData);
	Renderer->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = bufferSize;

	bufferSize = sizeof(UINT) * indices.size();

	Renderer->CreateCommittedBuffer(m_indexBuffer, m_indexBufferUpload, &CD3DX12_RESOURCE_DESC::Buffer(bufferSize));

	D3D12_SUBRESOURCE_DATA indexData = {};
	indexData.pData = &indices[0];
	indexData.RowPitch = bufferSize;
	indexData.SlicePitch = bufferSize;

	UpdateSubresources(Renderer->GetCommandList(), m_indexBuffer, m_indexBufferUpload, 0, 0, 1, &indexData);
	Renderer->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_indexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER));

	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	m_indexBufferView.SizeInBytes = bufferSize;

	m_indexcount = indices.size();
}
