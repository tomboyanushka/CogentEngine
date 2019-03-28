#include "Game.h"
#include "Vertex.h"
#include "ConstantBuffer.h"


// Needed for a helper function to read compiled shader files from the hard drive
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// Creating a struct that matches the cbuffer 
// definition in our shader.  This allows us to
// collect the data locally (CPU-side) with the
// same layout as the GPU's cbuffer, and then
// simply copy it to GPU memory in one step
//struct VertShaderExternalData
//{
//	XMFLOAT4X4 world;
//	XMFLOAT4X4 view;
//	XMFLOAT4X4 proj;
//};

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// DirectX itself, and our window, are not ready yet!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
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

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Release all DirectX objects created here
//  - Delete any objects to prevent memory leaks
// --------------------------------------------------------
Game::~Game()
{
	// Don't clean up until GPU is actually done
	WaitForGPU();

	// Release DX resources
	//vertexBuffer->Release();
	//indexBuffer->Release();
	delete mesh1;
	delete camera;

	vsConstBufferDescriptorHeap->Release();
	vsConstBufferUploadHeap->Release();

	rootSignature->Release();
	pipeState->Release();
}

// --------------------------------------------------------
// Called once per program, after DirectX and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	light = { XMFLOAT4(+0.1f, +0.1f, +0.1f, 1.0f), XMFLOAT4(+0.7f, +0.2f, +0.2f, +1.0f), XMFLOAT3(+1.0f, +0.0f, 0.8f), float(5) };
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

	// Create a descriptor heap to store constant buffer descriptors.  
	// One big heap is good enough to hold all cbv/srv/uav descriptors.
	D3D12_DESCRIPTOR_HEAP_DESC cbDesc = {};
	cbDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbDesc.NodeMask = 0;
	cbDesc.NumDescriptors = 4;
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
	resDesc.Width = 3 * bufferSize + pixelBufferSize;

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
	handle.ptr = vsConstBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + static_cast<UINT64>(incrementSize);
	cbvDesc.BufferLocation = vsConstBufferUploadHeap->GetGPUVirtualAddress() + bufferSize;
	device->CreateConstantBufferView(&cbvDesc, handle);

	handle.ptr = vsConstBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + static_cast<UINT64>(2 * incrementSize);
	cbvDesc.BufferLocation = vsConstBufferUploadHeap->GetGPUVirtualAddress() + (2 * bufferSize);
	device->CreateConstantBufferView(&cbvDesc, handle);


	handle.ptr = vsConstBufferDescriptorHeap->GetCPUDescriptorHandleForHeapStart().ptr + static_cast<UINT64>(3 * incrementSize);
	cbvDesc.SizeInBytes = pixelBufferSize;
	cbvDesc.BufferLocation = vsConstBufferUploadHeap->GetGPUVirtualAddress() + (3 * bufferSize);
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
	XMVECTOR pos = XMVectorSet(0, 0, -5, 0);
	XMVECTOR dir = XMVectorSet(0, 0, 1, 0);
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

	camera = new Camera(0, 0, -5);
	camera->UpdateProjectionMatrix((float)width / height);
}


// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateBasicGeometry()
{
	mesh1 = new Mesh("../../Assets/Models/Lion.obj", device, commandList);
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
	}

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

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Quit if the escape key is pressed
	if (GetAsyncKeyState(VK_ESCAPE))
		Quit();

	camera->Update(deltaTime);

	XMMATRIX W2; // = XMMatrixTranslation(-6, 0, 0);
	XMMATRIX W3 = XMMatrixTranslation(cos(totalTime) + 6, 0, 0);
	//XMStoreFloat4x4(&worldMatrix2, XMMatrixTranspose(W2));
	XMStoreFloat4x4(&worldMatrix3, XMMatrixTranspose(W3));

	if (job1.IsCompleted())
		auto f1 = pool.Enqueue(&job1);


	if (job2.IsCompleted())
	{
		job2.W = W2;
		job2.totalTime = totalTime;
		//XMStoreFloat4x4(&worldMatrix2, XMMatrixTranspose(job2.worldMatrix));
		auto f2 = pool.Enqueue(&job2);
	}
	

	pool.ExecuteCallbacks();
	// Collect data
	VertShaderExternalData data1 = {};
	data1.world = worldMatrix1;
	data1.view = camera->GetViewMatrix();
	data1.proj = camera->GetProjectionMatrix();

	VertShaderExternalData data2 = {};
	data2.world = job2.worldMatrix;
	data2.view = camera->GetViewMatrix();
	data2.proj = camera->GetProjectionMatrix();

	VertShaderExternalData data3 = {};
	data3.world = worldMatrix3;
	data3.view = camera->GetViewMatrix();
	data3.proj = camera->GetProjectionMatrix();

	pixelData.cameraPosition = camera->GetPosition();
	pixelData.dirLight = light;

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
	memcpy(gpuAddress, &data1, sizeof(VertShaderExternalData));
	memcpy(address + bufferSize, &data2, sizeof(VertShaderExternalData));
	memcpy(address + (2 * bufferSize), &data3, sizeof(VertShaderExternalData));
	memcpy(address + (3 * bufferSize), &pixelData, sizeof(PixelShaderExternalData));
	vsConstBufferUploadHeap->Unmap(0, 0);
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
		commandList->SetPipelineState(pipeState);

		// Root sig (must happen before root descriptor table)
		commandList->SetGraphicsRootSignature(rootSignature);

		auto incrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_GPU_DESCRIPTOR_HANDLE handle = {};
		D3D12_GPU_DESCRIPTOR_HANDLE pixelHandle = {};
		handle.ptr = vsConstBufferDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr;

		pixelHandle.ptr = vsConstBufferDescriptorHeap->GetGPUDescriptorHandleForHeapStart().ptr + (3 * incrementSize);

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

		//set mesh buffer
		commandList->IASetVertexBuffers(0, 1, &mesh1->GetVertexBufferView());
		commandList->IASetIndexBuffer(&mesh1->GetIndexBufferView());


		// Draw
		commandList->DrawIndexedInstanced(mesh1->GetIndexCount(), 1, 0, 0, 0);
		handle.ptr = handle.ptr + incrementSize;
		commandList->SetGraphicsRootDescriptorTable(
			0,
			handle);


		// Draw
		commandList->DrawIndexedInstanced(mesh1->GetIndexCount(), 1, 0, 0, 0);
		handle.ptr += incrementSize;
		commandList->SetGraphicsRootDescriptorTable(
			0,
			handle);

		// Draw
		commandList->DrawIndexedInstanced(mesh1->GetIndexCount(), 1, 0, 0, 0);
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