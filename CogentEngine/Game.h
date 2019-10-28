/*
	Naming Convention:
	sm_exampleStaticMesh
	dm_exampleDynamicMesh //with animations / input for movement
	e_exampleEntity
	t_texture
	m_material

*/
#pragma once

#define ScreenWidth = 1280
#define ScreenHegight = 720
#define TerrainIndex = 3

#include "DXCore.h"
#include <DirectXMath.h>
#include "DXUtility.h"
#include "FrameManager.h"
#include "Constants.h"
#include <CommonStates.h>

#include "ThreadPool.h"
#include "IJob.h"
#include "Job.h"

#include "d3dx12.h"
#include "ConstantBuffer.h"
#include "Camera.h"
#include "Light.h"

#include "Mesh.h"
#include "ModelLoader.h"
#include "Entity.h"

#include "Texture.h"
#include "Material.h"

#include "AStar.h"

class Game
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	int numEntities = 7;
	int currentIndex;
	int textureCount = 3;

	AStar::CoordinateList path;

	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);
	void DrawMesh(Mesh* mesh);
	void DrawEntity(Entity* entity);
	void DrawTransparentEntity(Entity* entity, float blendAmount);
	void CreateMaterials();
	void CreateTextures();
	void DrawSky();

	///AI functions
	void CreateNavmesh();
	AStar::CoordinateList FindPath(AStar::Vec2i source, AStar::Vec2i target);
	XMVECTOR MoveTowards(XMVECTOR current, XMVECTOR target, float distanceDelta);
	void AddCollider(AStar::Generator& generator, AStar::Vec2i coordinates);

	///Ray picking
	bool IsIntersecting(Entity* entity, Camera* camera, int mouseX, int mouseY, float& distance);

	void OnMouseDown(WPARAM buttonState, int x, int y);
	void OnMouseUp(WPARAM buttonState, int x, int y);
	void OnMouseMove(WPARAM buttonState, int x, int y);
	void OnMouseWheel(float wheelDelta, int x, int y);

private:

	void LoadShaders();
	void CreateMatrices();
	void CreateBasicGeometry();
	void CreateRootSigAndPipelineState();

	ID3D12RootSignature* rootSignature;
	ID3D12PipelineState* toonShadingPipeState;
	ID3D12PipelineState* outlinePipeState;
	ID3D12PipelineState* pbrPipeState;
	ID3D12PipelineState* transparencyPipeState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> skyPipeState;

	FrameManager frameManager;
	ConstantBufferView pixelCBV;
	ConstantBufferView transparencyCBV;
	ConstantBufferView skyCBV;

	ID3DBlob* vertexShaderByteCode;
	ID3DBlob* pixelShaderByteCode;

	ID3DBlob* outlineVS;
	ID3DBlob* outlinePS;
	 
	ID3DBlob* skyVS;
	ID3DBlob* skyPS;

	ID3DBlob* pbrPS;

	ID3DBlob* transparencyPS;

	PixelShaderExternalData pixelData;
	TransparencyExternalData transparencyData;

	Mesh* sm_sphere;
	Mesh* sm_skyCube;
	Mesh* sm_quad;
	Mesh* sm_cube;
	Mesh* dm_pawn;
	Mesh* sm_plane;

	ModelLoader mLoader;

	Texture t_skyTexture;
	Texture skyIrradiance;
	Texture skyPrefilter;
	Texture brdfLookUpTexture;

	Material m_floor;
	Material m_cobbleStone;
	Material m_scratchedPaint;
	Material m_paint;
	Material m_water;
	Material m_plane;

	Entity* e_plane;

	std::vector<Entity*> entities;
	std::vector<Entity*> selectedEntities;
	int selectedEntityIndex = -1;
	bool isSelected = false;

	XMFLOAT3 newDestination;

	DirectionalLight light;

	Camera* camera;

	AStar::Generator generator;

	ThreadPool pool{ 4 };
	MyJob job1;
	UpdatePosJob job2;
	PathFinder pathFinderJob;

	// Keeps track of the old mouse position.  Useful for 
	// determining how far the mouse moved in a single frame.
	POINT prevMousePos;
};

