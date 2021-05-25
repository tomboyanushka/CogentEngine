/*
	Naming Convention:
	sm_exampleStaticMesh
	dm_exampleDynamicMesh //with animations / input for movement
	e_exampleEntity
	t_texture
	m_material

*/
#pragma once


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

#include "Mesh.h"
#include "ModelLoader.h"
#include "Entity.h"

#include "Texture.h"
#include "Material.h"

#include "AStar.h"

struct TransparentEntity
{
	Entity* t_Entity;
	float zPosition;
};

class Game
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	int numEntities = 2;
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
	void DoubleBounceRefractionSetup(Entity* entity);
	void DrawRefractionEntity(Entity* entity, Texture textureIn, Texture normal, Texture customDepth);
	void CreateMaterials();
	void CreateTextures();
	void CreateLights();
	void CreateResources();
	void DrawSky();
	void LoadSponza();

	void DrawBlur(Texture texture);

	void TransitionResourceToState(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

	// Compute Z distance from Camera
	float ComputeZDistance(Camera* cam, XMFLOAT3 position);
	static bool CompareByLength(const TransparentEntity& a, const TransparentEntity& b);

	// AI functions
	void CreateNavmesh();
	AStar::CoordinateList FindPath(AStar::Vec2i source, AStar::Vec2i target);
	XMVECTOR MoveTowards(XMVECTOR current, XMVECTOR target, float distanceDelta);
	void AddCollider(AStar::Generator& generator, AStar::Vec2i coordinates);

	// Ray picking
	bool IsIntersecting(Entity* entity, Camera* camera, int mouseX, int mouseY, float& distance);

	void OnMouseDown(WPARAM buttonState, int x, int y);
	void OnMouseUp(WPARAM buttonState, int x, int y);
	void OnMouseMove(WPARAM buttonState, int x, int y);
	void OnMouseWheel(float wheelDelta, int x, int y);

private:

	void LoadShaders();
	void CreateMatrices();
	void CreateMesh();
	void CreateRootSigAndPipelineState();
	D3D12_GRAPHICS_PIPELINE_STATE_DESC CreatePSODescriptor(
		const unsigned int inputElementCount,
		D3D12_INPUT_ELEMENT_DESC inputElements[],
		ID3DBlob* vs,
		ID3DBlob* ps,
		D3D12_RASTERIZER_DESC rs,
		D3D12_DEPTH_STENCIL_DESC ds,
		D3D12_BLEND_DESC bs = CommonStates::Opaque
	);

	ID3D12RootSignature* rootSignature;
	ID3D12PipelineState* quadPipeState;
	ID3D12PipelineState* toonShadingPipeState;
	ID3D12PipelineState* outlinePipeState;
	ID3D12PipelineState* pbrPipeState;
	ID3D12PipelineState* transparencyPipeState;
	ID3D12PipelineState* blurPipeState;
	ID3D12PipelineState* refractionPipeState;
	ID3D12PipelineState* refractionDepthPipeState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> skyPipeState;

	FrameManager frameManager;
	ConstantBufferView pixelCBV;
	ConstantBufferView transparencyCBV;
	ConstantBufferView skyCBV;
	ConstantBufferView blurCBV;
	ConstantBufferView dofCBV;
	ConstantBufferView refractionCBV;

	ID3DBlob* vertexShaderByteCode;
	ID3DBlob* pixelShaderByteCode;

	ID3DBlob* outlineVS;
	ID3DBlob* outlinePS;
	ID3DBlob* skyVS;
	ID3DBlob* skyPS;
	ID3DBlob* pbrPS;
	ID3DBlob* toonPS;
	ID3DBlob* transparencyPS;
	ID3DBlob* blurPS;
	ID3DBlob* quadVS;
	ID3DBlob* quadPS;
	ID3DBlob* refractionVS;
	ID3DBlob* refractionPS;
	ID3DBlob* normalsPS;

	PixelShaderExternalData pixelData = {};
	TransparencyExternalData transparencyData;

	Mesh* sm_sphere;
	Mesh* sm_skyCube;
	Mesh* sm_quad;
	Mesh* sm_cube;
	Mesh* dm_pawn;
	Mesh* sm_plane;
	Mesh* sm_sponza;

	ModelLoader mLoader;

	Texture t_skyTexture;
	Texture skyIrradiance;
	Texture skyPrefilter;
	Texture brdfLookUpTexture;
	Texture blurTexture;
	Texture refractionTexture;
	Texture customDepthTexture;
	Texture backfaceNormalTexture;
	Texture backbufferTexture[FRAME_BUFFER_COUNT];

	ID3D12Resource* blurResource;
	D3D12_CPU_DESCRIPTOR_HANDLE blurRTVHandle;

	ID3D12Resource* refractionResource;
	D3D12_CPU_DESCRIPTOR_HANDLE refractionRTVHandle;

	ID3D12Resource* backfaceNormalResource;
	D3D12_CPU_DESCRIPTOR_HANDLE backfaceNormalHandle;
	
	// default
	Texture defaultDiffuse;
	Texture defaultNormal;
	Texture defaultRoughness;
	Texture defaultMetal;

	Material m_floor;
	Material m_cobbleStone;
	Material m_scratchedPaint;
	Material m_paint;
	Material m_water;
	Material m_plane;
	Material* m_sponza;
	Material m_default;
	std::vector<Material> sponzaMat;

	ModelData sponza;

	Entity* e_plane;
	Entity* e_sponza;
	Entity* e_capitol;
	Entity* e_capitol2;
	Entity* te_sphere1;
	Entity* te_sphere2;
	Entity* ref_sphere;

	std::vector<Entity*> entities;
	std::vector<Entity*> selectedEntities;
	std::vector<TransparentEntity> transparentEntities;
	std::vector<TransparentEntity> depthSortedEntities;
	int selectedEntityIndex = -1;
	bool isSelected = false;
	bool bBlurEnabled = false;

	XMFLOAT3 newDestination;

	DirectionalLight directionalLight1;
	PointLight pointLight;

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

