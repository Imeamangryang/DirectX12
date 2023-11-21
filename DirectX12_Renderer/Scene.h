#pragma once

#include "Terrain.h"
#include "Camera.h"

using namespace graphics;

#define SPEED 1000.0f
#define ROT_ANGLE 0.75f

struct InputDirections
{
	BOOL bFront;
	BOOL bBack;
	BOOL bLeft;
	BOOL bRight;
	BOOL bUp;
	BOOL bDown;
};

class Scene
{
public:
	Scene(int height, int width, Graphics* renderer);
	~Scene();

	void Draw();

	void HandleInput(const InputDirections& directions, float deltaTime);

	void HandleMouseInput(int x, int y);

private:
	void CloseCommandList();
	void SetViewport();

	Graphics* m_renderer;
	Terrain m_terrain;
	Camera m_camera;
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
};