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
#include <map>
#include "GameUtility.h"

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
	void UpdateAreaLights();
	void UpdateDiscLightDirection(Entity* areaLightEntity);
	void UpdateRectLights(Entity* areaLightEntity);

	// Create and Load
	void CreateMaterials();
	void CreateTextures();
	void CreateLights();
	void CreateResources();
	void LoadSponza();

	// Drawing 
	void Draw(float deltaTime, float totalTime);
	void DrawMesh(Mesh* mesh);
	void DrawEntity(Entity* entity, XMFLOAT3 position = XMFLOAT3(0, 0, 0));
	void DrawTransparentEntity(Entity* entity, float blendAmount);
	void DoubleBounceRefractionSetup(Entity* entity);
	void DrawRefractionEntity(Entity* entity, Texture textureIn, Texture normal, Texture customDepth, bool doubleBounce);
	void DrawSky();
	void DrawBlur(Texture texture);
	void DrawTransparentEntities();
	void DrawAreaLights(Entity* entity, AreaLightType type);
	void DispatchCompute();

	// Core Gfx
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
	void CreateComputeRootSigAndPipelineState();
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
	Microsoft::WRL::ComPtr<ID3D12RootSignature> computeRootSignature;

	ID3D12PipelineState* quadPipeState;
	ID3D12PipelineState* quadDepthPipeState;
	ID3D12PipelineState* toonShadingPipeState;
	ID3D12PipelineState* outlinePipeState;
	ID3D12PipelineState* pbrPipeState;
	ID3D12PipelineState* transparencyPipeState;
	ID3D12PipelineState* blurPipeState;
	ID3D12PipelineState* refractionPipeState;
	ID3D12PipelineState* refractionDepthPipeState;
	ID3D12PipelineState* areaLightEntityPipeState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> skyPipeState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> gameOfLifePipeState;

	FrameManager frameManager;
	ConstantBufferView pixelCBV;
	ConstantBufferView transparencyCBV;
	ConstantBufferView skyCBV;
	ConstantBufferView blurCBV;
	ConstantBufferView dofCBV;
	ConstantBufferView refractionCBV;
	GameUtility gameUtil;

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
	ID3DBlob* areaLightEntityPS;
	ID3DBlob* gameOfLifeCS;

	PixelShaderExternalData pixelData = {};
	TransparencyExternalData transparencyData;

	Mesh* sm_sphere;
	Mesh* sm_skyCube;
	Mesh* sm_quad;
	Mesh* sm_cube;
	Mesh* sm_buddhaStatue;
	Mesh* sm_plane;
	Mesh* sm_sponza;
	Mesh* sm_disc;
	Mesh* dm_pawn;

	ModelLoader mLoader;

	Texture t_skyTexture;
	Texture skyIrradiance;
	Texture skyPrefilter;
	Texture brdfLookUpTexture;
	Texture blurTexture;
	Texture refractionTexture;
	Texture customDepthTexture;
	Texture backfaceNormalTexture;
	Texture gameOfLifeUAV;
	Texture gameOfLifeSRV;
	Texture backbufferTexture[FRAME_BUFFER_COUNT];

	ID3D12Resource* blurResource;
	D3D12_CPU_DESCRIPTOR_HANDLE blurRTVHandle;

	ID3D12Resource* refractionResource;
	D3D12_CPU_DESCRIPTOR_HANDLE refractionRTVHandle;

	ID3D12Resource* backfaceNormalResource;
	D3D12_CPU_DESCRIPTOR_HANDLE backfaceNormalHandle;

	ID3D12Resource* gameOfLifeResource;
	D3D12_CPU_DESCRIPTOR_HANDLE gameOfLifeHandle;
	
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
	Material m_gameOfLife;
	Material* m_sponza;
	Material m_default;
	std::vector<Material> sponzaMat;

	ModelData sponza;

	Entity* e_plane;
	Entity* e_sponza;
	Entity* e_buddhaStatue;
	Entity* te_sphere1;
	Entity* te_sphere2;
	Entity* ref_sphere;
	Entity* e_sphereLight;
	Entity* e_discLight;
	Entity* e_rectLight;
	Entity* e_gameOfLife;

	// Containers
	std::vector<Entity*> entities;
	std::vector<Entity*> selectedEntities;
	std::vector<TransparentEntity> transparentEntities;
	std::vector<TransparentEntity> depthSortedEntities;
	std::vector<Material> pbrMaterials;
	std::vector<Entity*> pbrEntities;
	std::map<Entity*, SphereAreaLight> sphereAreaLightMap;
	std::map<Entity*, DiscAreaLight*> discAreaLightMap;
	std::map<Entity*, RectAreaLight*> rectAreaLightMap;

	// constants
	const int pbrSphereCount = 4;
	const int pbrCubeCount = 8;
	int selectedEntityIndex = -1;
	bool isSelected = false;
	bool bBlurEnabled = false;

	// Lights
	DirectionalLight directionalLight1;
	PointLight pointLight;
	SphereAreaLight sphereLight;
	DiscAreaLight discLight;
	RectAreaLight rectLight;

	// Camera
	Camera* camera;

	AStar::Generator generator;
	XMFLOAT3 newDestination;

	// Job System
	ThreadPool pool{ 4 };
	MyJob job1;
	UpdatePosJob job2;
	PathFinder pathFinderJob;

	// Keeps track of the old mouse position.  Useful for 
	// determining how far the mouse moved in a single frame.
	POINT prevMousePos;
};

