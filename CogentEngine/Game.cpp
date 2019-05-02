#include "Game.h"
#include "Vertex.h"
#include "ConstantBuffer.h"


// Needed for a helper function to read compiled shader files from the hard drive
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,		   // The application's handle
		"DirectX Game",    // Text for the window's title bar
		1280,			   // Width of the window's client area
		720,			   // Height of the window's client area
		true)			   // Show extra stats (fps) in title bar?
{
	// Initialize fields
	vertexBuffer = 0;
	indexBuffer = 0;
	camera = 0;

#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.");
#endif

}


Game::~Game()
{
	// Don't clean up until GPU is actually done
	WaitForGPU();

	delete sphere;
	delete quad;
	delete cube;
	delete camera;

	for (auto e : entities)
	{
		delete e;
	}

	vsConstBufferDescriptorHeap->Release();
	vsConstBufferUploadHeap->Release();

	rootSignature->Release();
	pipeState->Release();
	pipeState2->Release();
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	//ambient diffuse direction intensity
	light = { XMFLOAT4(+0.1f, +0.1f, +0.1f, 1.0f), XMFLOAT4(+0.7f, +0.2f, +0.2f, +1.0f), XMFLOAT3(0.2f, -2.0f, 1.8f), float(5) };
	// Reset the command list to start
	commandAllocator->Reset();
	commandList->Reset(commandAllocator, 0);

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();
	CreateMatrices();
	CreateBasicGeometry();
	CreateRootSigAndPipelineState();

	//AI Initialization
	entities[0]->SetPosition(XMFLOAT3(0, -4, 0));
	currentIndex = 0;
	CreateNavmesh();
	//path = FindPath({ 0,0 }, { 16 , 16 });

	// Wait here until GPU is actually done
	WaitForGPU();
}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files,
// creates an input layout (verifying against the vertex shader)
// and creates a constant buffer to hold external data
// --------------------------------------------------------
void Game::LoadShaders()
{
	// Read our compiled vertex shader code into a blob
	// - Essentially just "open the file and plop its contents here"
	D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderByteCode);
	D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderByteCode);

	D3DReadFileToBlob(L"OutlineVS.cso", &outlineVS);
	D3DReadFileToBlob(L"OutlinePS.cso", &outlinePS);

	// Create a descriptor heap to store constant buffer descriptors.  
	// One big heap is good enough to hold all cbv/srv/uav descriptors.
	D3D12_DESCRIPTOR_HEAP_DESC cbDesc = {};
	cbDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbDesc.NodeMask = 0;
	cbDesc.NumDescriptors = numEntities + 1;
	cbDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	device->CreateDescriptorHeap(&cbDesc, IID_PPV_ARGS(&vsConstBufferDescriptorHeap));

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD; // Upload heap since we'll be copying often!
	heapProps.VisibleNodeMask = 1;

	// Buffers must be multiples of 256 bytes!
	unsigned int bufferSize = sizeof(VertShaderExternalData);
	bufferSize = (bufferSize + 255); // Add 255 so we can drop last few bits
	bufferSize = bufferSize & ~255;  // Flip 255 and then use it to mask 
	unsigned int pixelBufferSize = sizeof(PixelShaderExternalData);
	pixelBufferSize = (pixelBufferSize + 255); // Add 255 so we can drop last few bits
	pixelBufferSize = pixelBufferSize & ~255;  // Flip 255 and then use it to mask 

	D3D12_RESOURCE_DESC resDesc = {};
	resDesc.Alignment = 0;
	resDesc.DepthOrArraySize = 1;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.Height = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Width = numEntities * bufferSize + pixelBufferSize;

	auto incrementSize = device->GetDescriptorHandleIncrementSize(cbDesc.Type);

	// Create a constant buffer resource heap
	device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		0,
		IID_PPV_ARGS(&vsConstBufferUploadHeap));

	// Need to get a view to the constant buffer
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = vsConstBufferUploadHeap->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = bufferSize; // Must be 256-byte aligned!
	device->CreateConstantBufferView(&cbvDesc, vsConstBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_CPU_DESCRIPTOR_HANDLE handle = {};

	for (int i = 1; i < numEntities; ++i)
	{
		handle.ptr = vsConstBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + static_cast<UINT64>(i * incrementSize);
		cbvDesc.BufferLocation = vsConstBufferUploadHeap->GetGPUVirtualAddress() + (i * bufferSize);
		device->CreateConstantBufferView(&cbvDesc, handle);
	}

	handle.ptr = vsConstBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + static_cast<UINT64>(numEntities * incrementSize);
	cbvDesc.SizeInBytes = pixelBufferSize;
	cbvDesc.BufferLocation = vsConstBufferUploadHeap->GetGPUVirtualAddress() + (numEntities * bufferSize);
	device->CreateConstantBufferView(&cbvDesc, handle);


}



// --------------------------------------------------------
// Initializes the matrices necessary to represent our geometry's 
// transformations and our 3D camera
// --------------------------------------------------------
void Game::CreateMatrices()
{
	// Set up world matrix
	// - In an actual game, each object will need one of these and they should
	//    update when/if the object moves (every frame)
	// - You'll notice a "transpose" happening below, which is redundant for
	//    an identity matrix.  This is just to show that HLSL expects a different
	//    matrix (column major vs row major) than the DirectX Math library
	XMMATRIX W = XMMatrixIdentity();
	XMStoreFloat4x4(&worldMatrix1, XMMatrixTranspose(W)); // Transpose for HLSL!

	XMMATRIX W2 = XMMatrixTranslation(-1, 0, 0);
	XMMATRIX W3 = XMMatrixTranslation(1, 0, 0);
	XMStoreFloat4x4(&worldMatrix2, XMMatrixTranspose(W2));
	XMStoreFloat4x4(&worldMatrix3, XMMatrixTranspose(W3));


	// Create the View matrix
	// - In an actual game, recreate this matrix every time the camera 
	//    moves (potentially every frame)
	// - We're using the LOOK TO function, which takes the position of the
	//    camera and the direction vector along which to look (as well as "up")
	// - Another option is the LOOK AT function, to look towards a specific
	//    point in 3D space
	XMVECTOR pos = XMVectorSet(5, 5, -5, 0);
	XMVECTOR dir = XMVectorSet(0, -5, 1, 0);
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);
	XMMATRIX V = XMMatrixLookToLH(
		pos,     // The position of the "camera"
		dir,     // Direction the camera is looking
		up);     // "Up" direction in 3D space (prevents roll)
	XMStoreFloat4x4(&viewMatrix, XMMatrixTranspose(V)); // Transpose for HLSL!

	// Create the Projection matrix
	// - This should match the window's aspect ratio, and also update anytime
	//    the window resizes (which is already happening in OnResize() below)
	XMMATRIX P = XMMatrixPerspectiveFovLH(
		0.25f * 3.1415926535f,		// Field of View Angle
		(float)width / height,		// Aspect ratio
		0.1f,						// Near clip plane distance
		100.0f);					// Far clip plane distance
	XMStoreFloat4x4(&projectionMatrix, XMMatrixTranspose(P)); // Transpose for HLSL!

	camera = new Camera(8, 7, -6);
	camera->UpdateProjectionMatrix((float)width / height);
	camera->Rotate(0.5f, 0);
}


// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateBasicGeometry()
{
	sphere = new Mesh("../../Assets/Models/sphere.obj", device, commandList);
	quad = new Mesh("../../Assets/Models/quad.obj", device, commandList);
	cube = new Mesh("../../Assets/Models/cube.obj", device, commandList);


	//entities.push_back(new Entity(mesh1));
	unsigned int bufferSize = sizeof(VertShaderExternalData);
	bufferSize = (bufferSize + 255); // Add 255 so we can drop last few bits
	bufferSize = bufferSize & ~255;  // Flip 255 and then use it to mask 

	void* gpuAddress;
	vsConstBufferUploadHeap->Map(0, 0, &gpuAddress);


	auto incrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_GPU_DESCRIPTOR_HANDLE handle = {};
	handle.ptr = vsConstBufferDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;


	char* address = reinterpret_cast<char*>(gpuAddress);
	for (int i = 0; i < numEntities; ++i)
	{
		entities.push_back(new Entity(sphere, (address + (i * bufferSize)), handle));
		handle.ptr += incrementSize;
	}

	CloseExecuteAndResetCommandList();
}

void Game::CreateRootSigAndPipelineState()
{

	// Create an input layout that describes the vertex format
	// used by the vertex shader we're using
	//  - This is used by the pipeline to know how to interpret the raw data
	//     sitting inside a vertex buffer
	//  - Doing this NOW because it requires a vertex shader's byte code to verify against!
	//  - Luckily, we already have that loaded (the blob above)
	const unsigned int inputElementCount = 4;


	D3D12_INPUT_ELEMENT_DESC inputElements[inputElementCount] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};


	// Root Sig
	{
		// Create a table of CBV's (constant buffer views)
		D3D12_DESCRIPTOR_RANGE cbvTable = {};
		cbvTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvTable.NumDescriptors = 1;
		cbvTable.BaseShaderRegister = 0;
		cbvTable.RegisterSpace = 0;
		cbvTable.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		D3D12_DESCRIPTOR_RANGE cbvTable2 = {};
		cbvTable2.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvTable2.NumDescriptors = 1;
		cbvTable2.BaseShaderRegister = 0;
		cbvTable2.RegisterSpace = 0;
		cbvTable2.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

		// Create the root parameter
		D3D12_ROOT_PARAMETER rootParam = {};
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParam.DescriptorTable.NumDescriptorRanges = 1;
		rootParam.DescriptorTable.pDescriptorRanges = &cbvTable;

		D3D12_ROOT_PARAMETER rootParam2 = {};
		rootParam2.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam2.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParam2.DescriptorTable.NumDescriptorRanges = 1;
		rootParam2.DescriptorTable.pDescriptorRanges = &cbvTable2;

		D3D12_ROOT_PARAMETER params[] = { rootParam, rootParam2 };

		// Describe and serialize the root signature
		D3D12_ROOT_SIGNATURE_DESC rootSig = {};
		rootSig.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSig.NumParameters = 2;
		rootSig.pParameters = params;
		rootSig.NumStaticSamplers = 0;
		rootSig.pStaticSamplers = 0;

		ID3DBlob* serializedRootSig = 0;
		ID3DBlob* errors = 0;

		D3D12SerializeRootSignature(
			&rootSig,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&serializedRootSig,
			&errors);

		// Check for errors during serialization
		if (errors != 0)
		{
			OutputDebugString((char*)errors->GetBufferPointer());
		}

		// Actually create the root sig
		device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature));
	}

	// Pipeline state
	{
		// Describe the pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		// -- Input assembler related ---
		psoDesc.InputLayout.NumElements = inputElementCount;
		psoDesc.InputLayout.pInputElementDescs = inputElements;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		// Overall primitive topology type (triangle, line, etc.) is set here 
		// IASetPrimTop() is still used to set list/strip/adj options
		// See: https://docs.microsoft.com/en-us/windows/desktop/direct3d12/managing-graphics-pipeline-state-in-direct3d-12

		// Root sig
		psoDesc.pRootSignature = rootSignature;

		// -- Shaders (VS/PS) --- 
		psoDesc.VS.pShaderBytecode = vertexShaderByteCode->GetBufferPointer();
		psoDesc.VS.BytecodeLength = vertexShaderByteCode->GetBufferSize();
		psoDesc.PS.pShaderBytecode = pixelShaderByteCode->GetBufferPointer();
		psoDesc.PS.BytecodeLength = pixelShaderByteCode->GetBufferSize();

		// -- Render targets ---
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM; // TODO: Parameterize
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT; // Parameterize
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		// -- States ---
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.DepthClipEnable = true;

		psoDesc.DepthStencilState.DepthEnable = true;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

		psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// -- Misc ---
		psoDesc.SampleMask = 0xffffffff;

		// Create the pipe state object
		device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeState));


		// -- Outline (VS/PS) --- 
		psoDesc.VS.pShaderBytecode = outlineVS->GetBufferPointer();
		psoDesc.VS.BytecodeLength = outlineVS->GetBufferSize();
		psoDesc.PS.pShaderBytecode = outlinePS->GetBufferPointer();
		psoDesc.PS.BytecodeLength = outlinePS->GetBufferSize();

		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
		//psoDesc.DepthStencilState.DepthEnable = false;

		device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipeState2));
	}

}

void Game::DrawEntity(Entity * entity)
{
	VertShaderExternalData vertexData = {};
	vertexData.world = entity->GetWorldMatrix();
	vertexData.view = camera->GetViewMatrixTransposed();
	vertexData.proj = camera->GetProjectionMatrixTransposed();

	pixelData.cameraPosition = camera->GetPosition();
	pixelData.dirLight = light;

	memcpy(entity->GetAddress(), &vertexData, sizeof(VertShaderExternalData));
	commandList->SetGraphicsRootDescriptorTable(0, entity->GetHandle());

	DrawMesh(entity->GetMesh());


}


// --------------------------------------------------------
// Handle resizing DirectX "stuff" to match the new window size.
// For instance, updating our projection matrix's aspect ratio.
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();

	// Update our projection matrix since the window size changed
	XMMATRIX P = XMMatrixPerspectiveFovLH(
		0.25f * 3.1415926535f,	// Field of View Angle
		(float)width / height,	// Aspect ratio
		0.1f,				  	// Near clip plane distance
		100.0f);			  	// Far clip plane distance
	XMStoreFloat4x4(&projectionMatrix, XMMatrixTranspose(P)); // Transpose for HLSL!
}


void Game::Update(float deltaTime, float totalTime)
{
	// Quit if the escape key is pressed
	if (GetAsyncKeyState(VK_ESCAPE))
		Quit();

	camera->Update(deltaTime);
	if (selectedEntityIndex != -1)
	{
		auto pos = entities[selectedEntityIndex]->GetPosition();
		if (currentIndex < path.size())
		{
			XMFLOAT3 newPosition = XMFLOAT3((float)path[path.size() - currentIndex - 1].x, -4.0f, (float)path[path.size() - currentIndex - 1].y);
			auto targetPos = XMLoadFloat3(&newPosition);

			XMStoreFloat3(&pos, MoveTowards(XMLoadFloat3(&pos), targetPos, 5 * deltaTime));
			entities[selectedEntityIndex]->SetPosition(pos);
		}

		pos.y = -4;
	}



	entities[1]->SetPosition(job2.pos);
	auto bounds = entities[1]->GetBoundingOrientedBox();
	//entities[2]->SetPosition(XMFLOAT3(sin(totalTime) + 6, 0, 0));

	entities[3]->SetMesh(cube);
	entities[3]->SetScale(XMFLOAT3(20, 0.5f, 20));
	entities[3]->SetPosition(XMFLOAT3(10, -5, 10));


	entities[4]->SetMesh(cube);
	entities[4]->SetPosition(XMFLOAT3(14, -4, 13));

	entities[5]->SetMesh(cube);
	entities[5]->SetPosition(XMFLOAT3(6, -4, 6));

	entities[6]->SetScale(XMFLOAT3(0.3f, 0.3f, 0.3f));
	entities[6]->SetPosition(XMFLOAT3(16, -4, 16));

	if (job1.IsCompleted())
		auto f1 = pool.Enqueue(&job1);


	if (job2.IsCompleted())
	{
		//job2.W = W2;
		job2.totalTime = totalTime;
		auto f2 = pool.Enqueue(&job2);
	}

	//for the callback functions
	pool.ExecuteCallbacks();


	// Copy data to the constant buffer
	// Note: Apparently upload heaps (like constant buffers) do NOT need to be
	// unmapped to be used by the GPU.  Keeping it mapped can speed things up.
	// See examples here: https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/nf-d3d12-id3d12resource-map

	unsigned int bufferSize = sizeof(VertShaderExternalData);
	bufferSize = (bufferSize + 255); // Add 255 so we can drop last few bits
	bufferSize = bufferSize & ~255;  // Flip 255 and then use it to mask 

	void* gpuAddress;
	vsConstBufferUploadHeap->Map(0, 0, &gpuAddress);

	char* address = reinterpret_cast<char*>(gpuAddress);
	memcpy(address + (numEntities * bufferSize), &pixelData, sizeof(PixelShaderExternalData));
	//vsConstBufferUploadHeap->Unmap(0, 0);
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Grab the current back buffer for this frame
	ID3D12Resource* backBuffer = backBuffers[currentSwapBuffer];

	// Clearing the render target
	{
		// Transition the back buffer from present to render target
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = backBuffer;
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &rb);

		// Background color (Cornflower Blue in this case) for clearing
		float color[] = { 0.0f, 0.2f, 0.3f, 1.0f };

		// Clear the RTV
		commandList->ClearRenderTargetView(
			rtvHandles[currentSwapBuffer],
			color,
			0, 0); // No scissor rectangles

		// Clear the depth buffer, too
		commandList->ClearDepthStencilView(
			dsvHandle,
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f,	// Depth = 1.0f
			0,		// Not clearing stencil, but need a value
			0, 0);	// No scissor rects
	}

	// Rendering here!
	{
		// Set overall pipeline state
		commandList->SetPipelineState(pipeState2);

		// Root sig (must happen before root descriptor table)
		commandList->SetGraphicsRootSignature(rootSignature);

		auto incrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//D3D12_GPU_DESCRIPTOR_HANDLE handle = {};
		//handle.ptr = vsConstBufferDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;

		D3D12_GPU_DESCRIPTOR_HANDLE pixelHandle = {};
		pixelHandle.ptr = vsConstBufferDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + (numEntities * incrementSize);

		// Set up other commands for rendering
		commandList->OMSetRenderTargets(1, &rtvHandles[currentSwapBuffer], true, &dsvHandle);
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Set constant buffer
		commandList->SetDescriptorHeaps(1, &vsConstBufferDescriptorHeap);

		//for pixel shader
		commandList->SetGraphicsRootDescriptorTable(
			1,
			pixelHandle);

		//set const buffer for current mesh
		commandList->SetGraphicsRootDescriptorTable(
			0,
			vsConstBufferDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

		// Draw outline for mesh 
		for (auto e : entities)
		{
			DrawEntity(e);
		}

		//Draw cel shaded mesh
		commandList->SetPipelineState(pipeState);
		for (auto e : entities)
		{
			DrawEntity(e);
		}


	}

	// Present
	{
		// Transition back to present
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = backBuffer;
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &rb);

		// Must occur BEFORE present
		CloseExecuteAndResetCommandList();

		// Present the current back buffer
		swapChain->Present(vsync ? 1 : 0, 0);

		// Figure out which buffer is next
		currentSwapBuffer++;
		if (currentSwapBuffer >= numBackBuffers)
			currentSwapBuffer = 0;

	}
}

void Game::DrawMesh(Mesh* mesh)
{
	commandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
	commandList->IASetIndexBuffer(&mesh->GetIndexBufferView());
	// Draw
	commandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
}

void Game::CreateNavmesh()
{
	generator.setWorldSize({ 20, 20 });
	generator.setHeuristic(AStar::Heuristic::manhattan);
	generator.setDiagonalMovement(true);
	AddCollider(generator, { 14, 13 });
	AddCollider(generator, { 6, 6 });
	/*generator.addCollision({ 6,8 });*/

}

AStar::CoordinateList Game::FindPath(AStar::Vec2i source, AStar::Vec2i target)
{
	auto path = generator.findPath(source, target);
	return path;
}

XMVECTOR Game::MoveTowards(XMVECTOR current, XMVECTOR target, float distanceDelta)
{
	XMVECTOR diff = XMVectorSubtract(target, current);
	XMVECTOR length = XMVector3Length(diff);

	float currentDist = 0;
	XMStoreFloat(&currentDist, length);

	if (currentDist <= distanceDelta || currentDist == 0)
	{
		currentIndex++;
		return target;

	}
	auto v = current + diff / currentDist * distanceDelta;
	return v;
}

void Game::AddCollider(AStar::Generator& generator, AStar::Vec2i coordinates)
{
	generator.addCollision({ coordinates.x ,coordinates.y });
	generator.addCollision({ coordinates.x , coordinates.y - 1 });
	generator.addCollision({ coordinates.x ,coordinates.y + 1 });

	generator.addCollision({ coordinates.x ,coordinates.y });
	generator.addCollision({ coordinates.x + 1 , coordinates.y });
	generator.addCollision({ coordinates.x - 1,coordinates.y });
}

bool Game::IsIntersecting(Entity* entity, Camera * camera, int mouseX, int mouseY, float& distance)
{
	newDestination = XMFLOAT3(0, 0, 0);
	uint16_t screenWidth = 1280;
	uint16_t screenHeight = 720;
	auto viewMatrix = XMMatrixTranspose(XMLoadFloat4x4(&camera->GetViewMatrixTransposed()));
	auto projMatrix = XMMatrixTranspose(XMLoadFloat4x4(&camera->GetProjectionMatrixTransposed()));

	auto orig = XMVector3Unproject(XMVectorSet((float)mouseX, (float)mouseY, 0.f, 0.f),
		0,
		0,
		screenWidth,
		screenHeight,
		0,
		1,
		projMatrix,
		viewMatrix,
		XMMatrixIdentity());

	auto dest = XMVector3Unproject(XMVectorSet((float)mouseX, (float)mouseY, 1.f, 0.f),
		0,
		0,
		screenWidth,
		screenHeight,
		0,
		1,
		projMatrix,
		viewMatrix,
		XMMatrixIdentity());

	auto direction = dest - orig;
	direction = XMVector3Normalize(direction);
	auto box = entity->GetBoundingOrientedBox();
	bool intersecting = box.Intersects(orig, direction, distance);

	//world space to local space
	float tMin = 0.0f;
	XMMATRIX W = XMLoadFloat4x4(&entity->GetWorldMatrix());
	XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(W), W);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(viewMatrix), viewMatrix);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(projMatrix), projMatrix);
	XMFLOAT4X4 P;
	XMStoreFloat4x4(&P, projMatrix);

	// Tranform ray to vi space of Mesh.
	XMMATRIX toLocal = XMMatrixMultiply(invView, invWorld);
	XMMATRIX toWorld = XMMatrixMultiply(invView, XMMatrixInverse(&XMMatrixDeterminant(toLocal), toLocal));

	// Compute picking ray in view space.
	float vx = (+2.0f* (float)mouseX / screenWidth - 1.0f) / P(0, 0);
	float vy = (-2.0f* (float)mouseX / screenHeight + 1.0f) / P(1, 1);

	// Ray definition in view space.
	XMVECTOR rayOrigin = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR rayDir = XMVectorSet(vx, vy, 1.0f, 0.0f);

	rayOrigin = XMVector3TransformCoord(rayOrigin, toLocal);
	rayDir = XMVector3TransformNormal(rayDir, toLocal);

	// Make the ray direction unit length for the intersection tests.
	rayDir = XMVector3Normalize(rayDir);

	if (intersecting)
	{
		auto temp = orig + distance * direction;
		auto mesh = entity->GetMesh();
		auto vertices = mesh->GetVertices();
		auto indices = mesh->GetIndices();

		UINT triCount = mesh->GetIndexCount() / 3;

		tMin = FLT_MAX;

		for (UINT i = 0; i < triCount; ++i)
		{
			// Indices for this triangle.
			UINT i0 = indices[i * 3 + 0];
			UINT i1 = indices[i * 3 + 1];
			UINT i2 = indices[i * 3 + 2];

			// Vertices for this triangle.
			XMVECTOR v0 = XMLoadFloat3(&vertices[i0].Position);
			XMVECTOR v1 = XMLoadFloat3(&vertices[i1].Position);
			XMVECTOR v2 = XMLoadFloat3(&vertices[i2].Position);

			// We have to iterate over all the triangles in order to find the nearest intersection.
			float t = 0.0f;
			//To Do: 
			//if (TriangleTests::Intersects(rayOrigin, rayDir, v0, v1, v2, t))
			//{
			//	if (t < tMin)
			//	{
			//		tMin = t;
			//		UINT pickedTriangle = i;

			//		//mPickedRitem->Visible = true;
			//		//mPickedRitem->IndexCount = 3;
			//		//mPickedRitem->BaseVertexLocation = 0;

			//		//// Picked render item needs same world matrix as object picked.
			//		//mPickedRitem->World = ri->World;
			//		//mPickedRitem->NumFramesDirty = gNumFrameResources;

			//		//// Offset to the picked triangle in the mesh index buffer.
			//		//mPickedRitem->StartIndexLocation = 3 * pickedTriangle;
			//	}
			//}

		}

		auto intersectionPoint = direction * distance + orig;

		//intersectionPoint = XMVector3TransformCoord(intersectionPoint,  W);

		XMStoreFloat3(&newDestination, intersectionPoint);
	}
	return intersecting;
}

#pragma region Mouse Input

// --------------------------------------------------------
// Helper method for mouse clicking.  We get this information
// from the OS-level messages anyway, so these helpers have
// been created to provide basic mouse input if you want it.
// --------------------------------------------------------
void Game::OnMouseDown(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...

	// Save the previous mouse position, so we have it for the future
	prevMousePos.x = x;
	prevMousePos.y = y;

	// Caputure the mouse so we keep getting mouse move
	// events even if the mouse leaves the window.  we'll be
	// releasing the capture once a mouse button is released
	SetCapture(hWnd);
	float distance;
	/*selectedEntities.clear();*/

	//if (IsIntersecting(entities[0], camera, x, y, distance) && !isSelected)
	//{

	//	selectedEntityIndex = 0;
	//	printf("Selected Entity %d\n", selectedEntityIndex);
	//	isSelected = true;
	//}
	if (IsIntersecting(entities[3], camera, x, y, distance) && isSelected)
	{
		currentIndex = 0;
		path = FindPath({ (int)entities[selectedEntityIndex]->GetPosition().x, (int)entities[selectedEntityIndex]->GetPosition().z }, { (int)newDestination.x, (int)newDestination.z });
		//printf("Intersecting at %f %f %f\n", newDestination.x, newDestination.y, newDestination.z);
		isSelected = false;
	}


	for (int i = 0; i < entities.size(); ++i)
	{
		if (IsIntersecting(entities[i], camera, x, y, distance) && i != 3)
		{
			selectedEntityIndex = i;
			isSelected = true;
			printf("Intersecting %d\n", i);
			printf("Selected Entity %d\n", selectedEntityIndex);
			break;
		}
		else if(i != 3)
		{
			//selectedEntityIndex = -1;
			printf("Unselect Entity \n");
			isSelected = false;
		}
	}



}

// --------------------------------------------------------
// Helper method for mouse release
// --------------------------------------------------------
void Game::OnMouseUp(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...

	// We don't care about the tracking the cursor outside
	// the window anymore (we're not dragging if the mouse is up)
	ReleaseCapture();
}

// --------------------------------------------------------
// Helper method for mouse movement.  We only get this message
// if the mouse is currently over the window, or if we're 
// currently capturing the mouse.
// --------------------------------------------------------
void Game::OnMouseMove(WPARAM buttonState, int x, int y)
{
	// Add any custom code here...
	if (buttonState & 0x0001)
	{
		float xDiff = (x - prevMousePos.x) * 0.005f;
		float yDiff = (y - prevMousePos.y) * 0.005f;
		camera->Rotate(yDiff, xDiff);
	}

	// Save the previous mouse position, so we have it for the future
	prevMousePos.x = x;
	prevMousePos.y = y;
}

// --------------------------------------------------------
// Helper method for mouse wheel scrolling.  
// WheelDelta may be positive or negative, depending 
// on the direction of the scroll
// --------------------------------------------------------
void Game::OnMouseWheel(float wheelDelta, int x, int y)
{
	// Add any custom code here...
}
#pragma endregion