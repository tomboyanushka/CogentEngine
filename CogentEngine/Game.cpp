#include "Game.h"
#include "Vertex.h"
#include "ConstantBuffer.h"
#include "WICTextureLoader.h"
#include "ResourceUploadBatch.h"

#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

using namespace DirectX;

Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,		   
		"DirectX Game",    
		1280,			   
		720,			   
		true)			   
{
	vertexBuffer = 0;
	indexBuffer = 0;
	camera = 0;

#if defined(DEBUG) || defined(_DEBUG)
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.");
#endif

}


Game::~Game()
{
	// Don't clean up until GPU is actually done
	WaitForGPU();

	delete sphere;
	delete skyCube;
	delete camera;

	for (auto e : entities)
	{
		delete e;
	}

	rootSignature->Release();
	pipeState->Release();
	pipeState2->Release();
}


void Game::Init()
{
	// Buffers must be multiples of 256 bytes!
	unsigned int bufferSize = sizeof(VertShaderExternalData);
	bufferSize = (bufferSize + 255); 
	bufferSize = bufferSize & ~255; 
	unsigned int pixelBufferSize = sizeof(PixelShaderExternalData);
	bufferSize = (bufferSize + 255); 
	bufferSize = bufferSize & ~255;  

	gpuConstantBuffer.Create(device.Get(), C_MaxConstBufferSize, bufferSize);
	pixelConstantBuffer.Create(device.Get(), C_MaxConstBufferSize, pixelBufferSize);

	gpuHeap.Create(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	//ambient diffuse direction intensity
	light = { XMFLOAT4(+0.1f, +0.1f, +0.1f, 1.0f), XMFLOAT4(+1.0f, +1.0f, +1.0f, +1.0f), XMFLOAT3(0.2f, -2.0f, 1.8f), float(10) };
	// Reset the command list to start
	commandAllocator->Reset();
	commandList->Reset(commandAllocator, 0);

	LoadShaders();
	CreateMatrices();
	CreateBasicGeometry();
	CreateMaterials();
	CreateRootSigAndPipelineState();

	//AI Initialization
	//entities[0]->SetPosition(XMFLOAT3(0, -4, 0));
	//entities[0]->SetScale(XMFLOAT3(1, 1, 1));
	//entities[0]->SetRotation(XMFLOAT3(-XM_PIDIV2, 0, 0));
	currentIndex = 0;
	CreateNavmesh();
	//path = FindPath({ 0,0 }, { 16 , 16 });

	// Wait here until GPU is actually done
	WaitForGPU();
}


void Game::LoadShaders()
{
	D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderByteCode);
	D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderByteCode);

	D3DReadFileToBlob(L"OutlineVS.cso", &outlineVS);
	D3DReadFileToBlob(L"OutlinePS.cso", &outlinePS);

	D3DReadFileToBlob(L"SkyVS.cso", &skyVS);
	D3DReadFileToBlob(L"SkyPS.cso", &skyPS);

	unsigned int bufferSize = sizeof(VertShaderExternalData);
	bufferSize = (bufferSize + 255); 
	bufferSize = bufferSize & ~255;  

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = gpuConstantBuffer.GetAddress();
	cbvDesc.SizeInBytes = bufferSize; 
	device->CreateConstantBufferView(&cbvDesc, gpuHeap.hCPUHeapStart); //hCPUHeapStart = handleCPU(0)
	heapCounter++;

	D3D12_CPU_DESCRIPTOR_HANDLE handle = {};

	for (int i = 1; i < numEntities; ++i)
	{
		cbvDesc.BufferLocation = gpuConstantBuffer.GetAddressWithIndex(i);
		device->CreateConstantBufferView(&cbvDesc, gpuHeap.handleCPU(heapCounter));
		heapCounter++;
	}

	cbvDesc.BufferLocation = pixelConstantBuffer.GetAddress();
	device->CreateConstantBufferView(&cbvDesc, gpuHeap.handleCPU(heapCounter));
	heapCounter++;

	//sky
	skyIndex = numEntities;
	skyHeapIndex = heapCounter;
	cbvDesc.BufferLocation = gpuConstantBuffer.GetAddressWithIndex(skyIndex);
	device->CreateConstantBufferView(&cbvDesc, gpuHeap.handleCPU(skyHeapIndex));
	heapCounter++;

}

void Game::CreateMatrices()
{
	XMMATRIX W = XMMatrixIdentity();
	XMStoreFloat4x4(&worldMatrix1, XMMatrixTranspose(W)); 

	XMMATRIX W2 = XMMatrixTranslation(-1, 0, 0);
	XMMATRIX W3 = XMMatrixTranslation(1, 0, 0);
	XMStoreFloat4x4(&worldMatrix2, XMMatrixTranspose(W2));
	XMStoreFloat4x4(&worldMatrix3, XMMatrixTranspose(W3));

	XMVECTOR pos = XMVectorSet(5, 5, -5, 0);
	XMVECTOR dir = XMVectorSet(0, -5, 1, 0);
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);
	XMMATRIX V = XMMatrixLookToLH(
		pos,     
		dir,     
		up);     
	XMStoreFloat4x4(&viewMatrix, XMMatrixTranspose(V)); 

	XMMATRIX P = XMMatrixPerspectiveFovLH(
		0.25f * 3.1415926535f,		// Field of View Angle
		(float)width / height,		// Aspect ratio
		0.1f,						// Near clip plane distance
		100.0f);					// Far clip plane distance
	XMStoreFloat4x4(&projectionMatrix, XMMatrixTranspose(P)); 

	camera = new Camera(-1.5, 3.5, -7);
	camera->UpdateProjectionMatrix((float)width / height);
	camera->Rotate(0.5f, 0);
}


void Game::CreateBasicGeometry()
{
	sphere = new Mesh("../../Assets/Models/sphere.obj", device.Get(), commandList);
	skyCube = new Mesh("../../Assets/Models/cube.obj", device.Get(), commandList);

	for (int i = 0; i < numEntities; ++i)
	{
		entities.push_back(new Entity(sphere, (gpuConstantBuffer.GetMappedAddressWithIndex(i)), gpuHeap.handleGPU(i), i, &floorMaterial));
	}

	CloseExecuteAndResetCommandList();
}

void Game::CreateRootSigAndPipelineState()
{
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

		D3D12_DESCRIPTOR_RANGE srvTable = {};
		srvTable.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvTable.NumDescriptors = 2;
		srvTable.BaseShaderRegister = 0;
		srvTable.RegisterSpace = 0;
		srvTable.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;


		// Create the root parameter
		D3D12_ROOT_PARAMETER rootParam = {};
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParam.DescriptorTable.NumDescriptorRanges = 1;
		rootParam.DescriptorTable.pDescriptorRanges = &cbvTable;

		//to pass the pixel constant buffer
		D3D12_ROOT_PARAMETER rootParam2 = {};
		rootParam2.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam2.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParam2.DescriptorTable.NumDescriptorRanges = 1;
		rootParam2.DescriptorTable.pDescriptorRanges = &cbvTable2;

		//to pass the pixel constant buffer
		D3D12_ROOT_PARAMETER rootParam3 = {};
		rootParam3.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;  
		rootParam3.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
		rootParam3.DescriptorTable.NumDescriptorRanges = 1;
		rootParam3.DescriptorTable.pDescriptorRanges = &srvTable;

		D3D12_ROOT_PARAMETER params[] = { rootParam, rootParam2, rootParam3 };
		CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[1];
		StaticSamplers[0].Init(0, D3D12_FILTER_ANISOTROPIC);
		D3D12_ROOT_SIGNATURE_DESC rootSig = {};
		rootSig.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSig.NumParameters = 3;
		rootSig.pParameters = params;
		rootSig.NumStaticSamplers = 1;
		rootSig.pStaticSamplers = StaticSamplers;

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

		// -- SKY (VS/PS) --- 
		psoDesc.VS.pShaderBytecode = skyVS->GetBufferPointer();
		psoDesc.VS.BytecodeLength = skyVS->GetBufferSize();
		psoDesc.PS.pShaderBytecode = skyPS->GetBufferPointer();
		psoDesc.PS.BytecodeLength = skyPS->GetBufferSize();

		D3D12_RASTERIZER_DESC rs = {};
		rs.CullMode = D3D12_CULL_MODE_FRONT;
		rs.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState = rs;
		
		D3D12_DEPTH_STENCIL_DESC ds = {};
		ds.DepthEnable = true;
		ds.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		ds.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		psoDesc.DepthStencilState = ds;

		device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&skyPipeState));
		
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

	gpuConstantBuffer.CopyDataWithIndex(&vertexData, sizeof(VertShaderExternalData), entity->GetConstantBufferIndex());
	commandList->SetGraphicsRootDescriptorTable(0, entity->GetHandle());
	commandList->SetGraphicsRootDescriptorTable(2, entity->GetMaterial()->GetFirstGPUHandle());

	DrawMesh(entity->GetMesh());
}


void Game::OnResize()
{
	DXCore::OnResize();

	XMMATRIX P = XMMatrixPerspectiveFovLH(
		0.25f * 3.1415926535f,	
		(float)width / height,	
		0.1f,				  	
		100.0f);			  	
	XMStoreFloat4x4(&projectionMatrix, XMMatrixTranspose(P)); 
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



	entities[0]->SetPosition(job2.pos);
	auto bounds = entities[0]->GetBoundingOrientedBox();

	entities[1]->SetPosition(XMFLOAT3(0, 0, 0));
	entities[1]->SetMaterial(&scratchedMaterial);

	entities[2]->SetPosition(XMFLOAT3(2, 0, 0));
	entities[2]->SetMaterial(&waterMaterial);

	entities[3]->SetPosition(XMFLOAT3(-2, 0, 0));

	if (job1.IsCompleted())
		auto f1 = pool.Enqueue(&job1);


	if (job2.IsCompleted())
	{
		//job2.W = W2;
		job2.totalTime = totalTime;
		auto f2 = pool.Enqueue(&job2);
	}

	if (pathFinderJob.IsCompleted())
	{
		path = pathFinderJob.path;
	}

	//for the callback functions
	pool.ExecuteCallbacks();

	pixelConstantBuffer.CopyData(&pixelData, sizeof(PixelShaderExternalData));
	//vsConstBufferUploadHeap->Unmap(0, 0);
}


void Game::Draw(float deltaTime, float totalTime)
{

	ID3D12Resource* backBuffer = backBuffers[currentSwapBuffer];

	{
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

		// Set up other commands for rendering
		commandList->OMSetRenderTargets(1, &rtvHandles[currentSwapBuffer], true, &dsvHandle);
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


		// Set constant buffer
		commandList->SetDescriptorHeaps(1, gpuHeap.pDescriptorHeap.GetAddressOf());
		
		//for pixel shader
		commandList->SetGraphicsRootDescriptorTable(
			1,
			gpuHeap.handleGPU(numEntities));

		//set const buffer for current mesh
		commandList->SetGraphicsRootDescriptorTable(
			0,
			gpuHeap.hGPUHeapStart);

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

		DrawSky();

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

void Game::CreateMaterials()
{
	heapCounter = floorMaterial.Create(device.Get(),
		L"../../Assets/Textures/floor/diffuse.png",
		L"../../Assets/Textures/floor/normal.png",
		commandQueue,
		heapCounter,
		gpuHeap);

	heapCounter = scratchedMaterial.Create(device.Get(),
		L"../../Assets/Textures/scratched/diffuse.png",
		L"../../Assets/Textures/scratched/normal.png",
		commandQueue,
		heapCounter,
		gpuHeap);

	heapCounter = waterMaterial.Create(device.Get(),
		L"../../Assets/Textures/water/diffuse.png",
		L"../../Assets/Textures/water/normal.png",
		commandQueue,
		heapCounter,
		gpuHeap);

	skyTexture.Create(device.Get(),
		L"../../Assets/Textures/SunnyCubeMap.dds",
		commandQueue,
		heapCounter,
		gpuHeap,
		DDS);
	heapCounter++;

}

void Game::DrawSky()
{
	SkyboxExternalData skyboxExternalData = {};
	skyboxExternalData.view = camera->GetViewMatrixTransposed();
	skyboxExternalData.proj = camera->GetProjectionMatrixTransposed();

	gpuConstantBuffer.CopyDataWithIndex(&skyboxExternalData, sizeof(SkyboxExternalData), skyIndex);

	commandList->SetPipelineState(skyPipeState.Get());

	commandList->SetGraphicsRootDescriptorTable(0, gpuHeap.handleGPU(skyHeapIndex));
	commandList->SetGraphicsRootDescriptorTable(2, skyTexture.GetGPUHandle());

	DrawMesh(skyCube);
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

	generator.addCollision({ coordinates.x + 1,coordinates.y + 1 });
	generator.addCollision({ coordinates.x - 1, coordinates.y - 1 });
	generator.addCollision({ coordinates.x - 1,coordinates.y + 1 });
	generator.addCollision({ coordinates.x + 1,coordinates.y - 1 });

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
		pathFinderJob.currentPos = entities[selectedEntityIndex]->GetPosition();
		pathFinderJob.targetPos = newDestination;
		pathFinderJob.generator = &generator;
		auto f2 = pool.Enqueue(&pathFinderJob);
		//path = FindPath({ (int)entities[selectedEntityIndex]->GetPosition().x, (int)entities[selectedEntityIndex]->GetPosition().z }, { (int)newDestination.x, (int)newDestination.z });
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