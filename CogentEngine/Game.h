#pragma once

#define ScreenWidth = 1280
#define ScreenHegight = 720
#define TerrainIndex = 3

#include "DXCore.h"
#include <DirectXMath.h>

#include "ThreadPool.h"
#include "IJob.h"
#include "Job.h"

#include "d3dx12.h"
#include "ConstantBuffer.h"
#include "Camera.h"
#include "Light.h"

#include "Mesh.h"
#include "Entity.h"

#include "AStar.h"

class Game
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	//ints
	int numEntities = 7;
	int currentIndex;
	int textureCount = 3;


	AStar::CoordinateList path;

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);
	void DrawMesh(Mesh* mesh);

	///AI functions
	void CreateNavmesh();
	AStar::CoordinateList FindPath(AStar::Vec2i source, AStar::Vec2i target);
	XMVECTOR MoveTowards(XMVECTOR current, XMVECTOR target, float distanceDelta);
	void AddCollider(AStar::Generator& generator, AStar::Vec2i coordinates);

	///Ray picking
	bool IsIntersecting(Entity* entity, Camera* camera, int mouseX, int mouseY, float& distance);

	// Overridden mouse input helper methods
	void OnMouseDown(WPARAM buttonState, int x, int y);
	void OnMouseUp(WPARAM buttonState, int x, int y);
	void OnMouseMove(WPARAM buttonState, int x, int y);
	void OnMouseWheel(float wheelDelta, int x, int y);
private:

	// Initialization helper methods - feel free to customize, combine, etc.
	void LoadShaders();
	void CreateMatrices();
	void CreateBasicGeometry();
	void CreateRootSigAndPipelineState();

	//// ********** NO SIMPLE SHADER CHANGE **********
	//ID3D11Buffer* vsConstantBuffer;
	//ID3D11VertexShader* vs;
	//ID3D11PixelShader* ps;
	//ID3D11InputLayout* inputLayout;
	//// ********** NO SIMPLE SHADER CHANGE **********

	//// Buffers to hold actual geometry data
	//ID3D11Buffer* vertexBuffer;
	//ID3D11Buffer* indexBuffer;

	ID3D12RootSignature* rootSignature;
	ID3D12PipelineState* pipeState;
	ID3D12PipelineState* pipeState2;

	ID3DBlob* vertexShaderByteCode;
	ID3DBlob* pixelShaderByteCode;

	ID3DBlob* outlineVS;
	ID3DBlob* outlinePS;

	D3D12_VERTEX_BUFFER_VIEW vbView;
	ID3D12Resource* vertexBuffer;

	D3D12_INDEX_BUFFER_VIEW ibView;
	ID3D12Resource* indexBuffer;

	ID3D12DescriptorHeap* vsConstBufferDescriptorHeap;
	ID3D12Resource* vsConstBufferUploadHeap;

	PixelShaderExternalData pixelData;
	//PixelShaderExternalData* pixelShaderData;

	// The matrices to go from model space to screen space
	DirectX::XMFLOAT4X4 worldMatrix1;
	DirectX::XMFLOAT4X4 worldMatrix2;
	DirectX::XMFLOAT4X4 worldMatrix3;
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projectionMatrix;

	Mesh* sphere;
	Mesh* quad;
	Mesh* cube;
	Mesh* pawn;

	ID3D12Resource* testTexture;
	ID3D12Resource* groundTexture;
	ID3D12Resource* woodTexture;
	ID3D12Resource* chessTexture;

	D3D12_GPU_DESCRIPTOR_HANDLE testHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE woodHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE chessHandle;

	std::vector<Entity*> entities;
	std::vector<Entity*> selectedEntities;
	int selectedEntityIndex = -1;
	bool isSelected = false;

	XMFLOAT3 newDestination;

	DirectionalLight light;

	Camera* camera;

	AStar::Generator generator;

	void DrawEntity(Entity* entity);

	ThreadPool pool{ 4 };
	MyJob job1;
	UpdatePosJob job2;
	PathFinder pathFinderJob;

	// Keeps track of the old mouse position.  Useful for 
	// determining how far the mouse moved in a single frame.
	POINT prevMousePos;
};

