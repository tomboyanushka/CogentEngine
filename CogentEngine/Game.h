#pragma once

#include "DXCore.h"
#include <DirectXMath.h>
#include "Mesh.h"
#include "Camera.h"
#include "Light.h"
#include "ConstantBuffer.h"
#include "ThreadPool.h"
#include "IJob.h"
#include "Job.h"

class Game
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

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

	ID3DBlob* vertexShaderByteCode;
	ID3DBlob* pixelShaderByteCode;

	D3D12_VERTEX_BUFFER_VIEW vbView;
	ID3D12Resource* vertexBuffer;

	D3D12_INDEX_BUFFER_VIEW ibView;
	ID3D12Resource* indexBuffer;

	ID3D12DescriptorHeap* vsConstBufferDescriptorHeap;
	ID3D12Resource* vsConstBufferUploadHeap;


	// The matrices to go from model space to screen space
	DirectX::XMFLOAT4X4 worldMatrix1;
	DirectX::XMFLOAT4X4 worldMatrix2;
	DirectX::XMFLOAT4X4 worldMatrix3;
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projectionMatrix;

	PixelShaderExternalData pixelData;

	Mesh* mesh1;
	Mesh* mesh2;
	Mesh* mesh3;

	DirectionalLight light;

	Camera* camera;

	ThreadPool pool{ 4 };
	MyJob job1;
	UpdatePosJob job2;

	// Keeps track of the old mouse position.  Useful for 
	// determining how far the mouse moved in a single frame.
	POINT prevMousePos;
};

