#include "Scene.h"

Scene::Scene(int height, int width, Graphics* renderer) : 
	m_terrain(renderer),
	m_sky(renderer),
	m_renderer(renderer),
	m_camera(height, width)
{
	m_viewport.TopLeftX = 0;
	m_viewport.TopLeftY = 0;
	m_viewport.Width = (float)width;
	m_viewport.Height = (float)height;
	m_viewport.MinDepth = 0;
	m_viewport.MaxDepth = 1;

	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = width;
	m_scissorRect.bottom = height;

	CloseCommandList();
	m_renderer->LoadAsset();

	m_terrain.ClearUnusedUploadBuffersAfterInit();
	m_sky.ClearUnusedUploadBuffersAfterInit();
}

Scene::~Scene()
{
	m_renderer->ClearAllFrames();
	m_renderer = nullptr;
}

void Scene::Draw()
{
	m_renderer->ResetPipeline();

	const float clearColor[] = { 0.1f, 0.1f, 0.1f, 1.0f };
	m_renderer->SetBackBufferRender(m_renderer->GetCommandList(), clearColor);

	SetViewport();

	//m_terrain.Draw2D(m_renderer->GetCommandList());
	//m_terrain.Draw3D(m_renderer->GetCommandList(), m_camera.GetViewProjectionMatrixTransposed(), m_camera.GetEyePosition());
	if (m_DrawMode == 1)
	{
		m_terrain.DrawTes(m_renderer->GetCommandList(), m_camera.GetViewProjectionMatrixTransposed(), m_camera.GetEyePosition());
		m_sky.Draw3D(m_renderer->GetCommandList(), m_camera.GetViewProjectionMatrixTransposed(), m_camera.GetEyePosition());
	}
	else
	{
		m_terrain.DrawTes_Wireframe(m_renderer->GetCommandList(), m_camera.GetViewProjectionMatrixTransposed(), m_camera.GetEyePosition());
		m_sky.Draw3D(m_renderer->GetCommandList(), m_camera.GetViewProjectionMatrixTransposed(), m_camera.GetEyePosition());
	}

	m_renderer->SetBackBufferPresent(m_renderer->GetCommandList());
	CloseCommandList();
	m_renderer->Render();
}

void Scene::HandleInput(const InputDirections& directions, float deltaTime)
{
	if (directions.bFront)
	{
		m_camera.Translate(XMFLOAT3(SPEED * deltaTime, 0.0f, 0.0f));
	}

	if (directions.bBack)
	{
		m_camera.Translate(XMFLOAT3(-SPEED * deltaTime, 0.0f, 0.0f));
	}

	if (directions.bLeft)
	{
		m_camera.Translate(XMFLOAT3(0.0f, SPEED * deltaTime, 0.0f));
	}

	if (directions.bRight)
	{
		m_camera.Translate(XMFLOAT3(0.0f, -SPEED * deltaTime, 0.0f));
	}

	if (directions.bUp)
	{
		m_camera.Translate(XMFLOAT3(0.0f, 0.0f, SPEED * deltaTime));
	}

	if (directions.bDown)
	{
		m_camera.Translate(XMFLOAT3(0.0f, 0.0f, -SPEED * deltaTime));
	}
	if (directions.bMode1)
	{
		m_DrawMode = 1;
	}
	if (directions.bMode2)
	{
		m_DrawMode = 2;
	}
}

void Scene::HandleMouseInput(int x, int y)
{
	m_camera.Pitch(ROT_ANGLE * y);
	m_camera.Yaw(-ROT_ANGLE * x);
}

void Scene::CloseCommandList()
{
	if (FAILED(m_renderer->GetCommandList()->Close()))
	{
		throw GFX_Exception("CommandList Close failed.");
	}
}

void Scene::SetViewport()
{
	m_renderer->GetCommandList()->RSSetViewports(1, &m_viewport);
	m_renderer->GetCommandList()->RSSetScissorRects(1, &m_scissorRect);
}
