#define NOMINMAX
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
	/*vertexBuffer = 0;
	indexBuffer = 0;*/
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

	//delete meshes
	delete sm_sphere;
	delete sm_skyCube;
	delete sm_plane;
	delete sm_sponza;

	//delete entities
	delete e_plane;
	delete e_sponza;
	delete camera;

	for (auto e : entities)
	{
		delete e;
	}

	rootSignature->Release();
	toonShadingPipeState->Release();
	outlinePipeState->Release();
	pbrPipeState->Release();
	transparencyPipeState->Release();
}


void Game::Init()
{
	frameManager.Initialize(device.Get());
	CreateLights();
	// Reset the command list to start
	commandAllocator[currentBackBufferIndex]->Reset();
	commandList->Reset(commandAllocator[currentBackBufferIndex], 0);

	LoadShaders();
	CreateMatrices();
	CreateTextures();
	CreateMaterials();
	CreateMesh();
	CreateRootSigAndPipelineState();

	//AI Initialization
	//entities[0]->SetPosition(XMFLOAT3(0, -4, 0));
	//entities[0]->SetScale(XMFLOAT3(1, 1, 1));
	//entities[0]->SetRotation(XMFLOAT3(-XM_PIDIV2, 0, 0));
	currentIndex = 0;
	CreateNavmesh();
	//path = FindPath({ 0,0 }, { 16 , 16 });
}


void Game::LoadShaders()
{
	D3DReadFileToBlob(L"VertexShader.cso", &vertexShaderByteCode);
	D3DReadFileToBlob(L"PixelShader.cso", &pixelShaderByteCode);
	D3DReadFileToBlob(L"OutlineVS.cso", &outlineVS);
	D3DReadFileToBlob(L"OutlinePS.cso", &outlinePS);
	D3DReadFileToBlob(L"SkyVS.cso", &skyVS);
	D3DReadFileToBlob(L"SkyPS.cso", &skyPS);
	D3DReadFileToBlob(L"PBRPixelShader.cso", &pbrPS);
	D3DReadFileToBlob(L"ToonPS.cso", &toonPS);
	D3DReadFileToBlob(L"TransparencyPS.cso", &transparencyPS);

	unsigned int bufferSize = sizeof(PixelShaderExternalData);
	bufferSize = (bufferSize + 255); 
	bufferSize = bufferSize & ~255;  

	pixelCBV = frameManager.CreateConstantBufferView(sizeof(PixelShaderExternalData));
	transparencyCBV = frameManager.CreateConstantBufferView(sizeof(TransparencyExternalData));
	skyCBV = frameManager.CreateConstantBufferView(sizeof(SkyboxExternalData));
}

void Game::CreateMatrices()
{
	camera = new Camera(-1.5, 3.5, -7);
	camera->UpdateProjectionMatrix((float)width / height);
	camera->Rotate(0.2f, 0);
}


void Game::CreateMesh()
{
	// Load meshes
	sm_sphere = mLoader.Load("../../Assets/Models/sphere.obj", device.Get(), commandList);
	sm_skyCube = mLoader.Load("../../Assets/Models/cube.obj", device.Get(), commandList);
	sm_plane = mLoader.Load("../../Assets/Models/lowPolyPlane.fbx", device.Get(), commandList);

	LoadSponza();
	
	// Create Entities
	for (int i = 0; i < numEntities; ++i)
	{
		entities.push_back(frameManager.CreateEntity(sm_sphere, &m_floor));
	}
	e_plane = frameManager.CreateEntity(sm_plane, &m_plane);
	e_sponza = frameManager.CreateEntity(sm_sponza, &m_default);

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
		//creating descriptor ranges
		CD3DX12_DESCRIPTOR_RANGE range[4];
		//vertex cbv
		range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		//pixel cbv
		range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		//texture 1 cbv
		range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);
		//texture 2 cbv
		range[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 4); //to simplify separating pbr and ibl textures

		//creating root parameters
		CD3DX12_ROOT_PARAMETER rootParameters[4];
		//vertex
		rootParameters[0].InitAsDescriptorTable(1, &range[0], D3D12_SHADER_VISIBILITY_VERTEX);
		//pixel
		rootParameters[1].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_PIXEL);
		//texture 1
		rootParameters[2].InitAsDescriptorTable(1, &range[2], D3D12_SHADER_VISIBILITY_PIXEL);
		//texture 2
		rootParameters[3].InitAsDescriptorTable(1, &range[3], D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(4, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | // we can deny shader stages here for better performance
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS);

		CD3DX12_STATIC_SAMPLER_DESC StaticSamplers[1];
		StaticSamplers[0].Init(0, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0, (UINT)2.0f);

		rootSignatureDesc.NumStaticSamplers = 1;
		rootSignatureDesc.pStaticSamplers = StaticSamplers;

		ID3DBlob* serializedRootSig = 0;
		ID3DBlob* errors = 0;

		D3D12SerializeRootSignature(
			&rootSignatureDesc,
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

		// -- Toon shading pipe state --
		psoDesc.PS.pShaderBytecode = toonPS->GetBufferPointer();
		psoDesc.PS.BytecodeLength = toonPS->GetBufferSize();
		device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&toonShadingPipeState));

		// -- PBR pipe state --
		psoDesc.PS.pShaderBytecode = pbrPS->GetBufferPointer();
		psoDesc.PS.BytecodeLength = pbrPS->GetBufferSize();
		device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pbrPipeState));

		// -- Transparency pipe state --
		psoDesc.PS.pShaderBytecode = transparencyPS->GetBufferPointer();
		psoDesc.PS.BytecodeLength = transparencyPS->GetBufferSize();
		psoDesc.BlendState = CommonStates::AlphaBlend;
		device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&transparencyPipeState));

		// -- Outline (VS/PS) --- 
		psoDesc.VS.pShaderBytecode = outlineVS->GetBufferPointer();
		psoDesc.VS.BytecodeLength = outlineVS->GetBufferSize();
		psoDesc.PS.pShaderBytecode = outlinePS->GetBufferPointer();
		psoDesc.PS.BytecodeLength = outlinePS->GetBufferSize();
		psoDesc.BlendState = CommonStates::Opaque;

		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
		//psoDesc.DepthStencilState.DepthEnable = false;

		device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&outlinePipeState));

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
	VertexShaderExternalData vertexData = {};
	vertexData.world = entity->GetWorldMatrix();
	vertexData.view = camera->GetViewMatrixTransposed();
	vertexData.proj = camera->GetProjectionMatrixTransposed();

	pixelData.dirLight = directionalLight1;
	pixelData.pointLight[0] = pointLight;
	pixelData.cameraPosition = camera->GetPosition();
	pixelData.pointLightCount = MaxPointLights;

	frameManager.CopyData(&vertexData, sizeof(VertexShaderExternalData), entity->GetConstantBufferView(), currentBackBufferIndex);
	frameManager.CopyData(&pixelData, sizeof(PixelShaderExternalData), pixelCBV, currentBackBufferIndex);

	// set the material
	commandList->SetGraphicsRootDescriptorTable(0, frameManager.GetGPUHandle(entity->GetConstantBufferView().heapIndex, currentBackBufferIndex));
	commandList->SetGraphicsRootDescriptorTable(2, entity->GetMaterial()->GetFirstGPUHandle(frameManager.GetGPUDescriptorHeap(), currentBackBufferIndex));

	DrawMesh(entity->GetMesh());
}

void Game::DrawTransparentEntity(Entity* entity, float blendAmount)
{
	VertexShaderExternalData vertexData = {};
	vertexData.world = entity->GetWorldMatrix();
	vertexData.view = camera->GetViewMatrixTransposed();
	vertexData.proj = camera->GetProjectionMatrixTransposed();

	transparencyData.cameraPosition = camera->GetPosition();
	transparencyData.dirLight = directionalLight1;
	transparencyData.blendAmount = blendAmount;

	frameManager.CopyData(&vertexData, sizeof(VertexShaderExternalData), entity->GetConstantBufferView(), currentBackBufferIndex);
	frameManager.CopyData(&transparencyData, sizeof(TransparencyExternalData), transparencyCBV, currentBackBufferIndex);
	commandList->SetGraphicsRootDescriptorTable(0, frameManager.GetGPUHandle(entity->GetConstantBufferView().heapIndex, currentBackBufferIndex));
	commandList->SetGraphicsRootDescriptorTable(2, entity->GetMaterial()->GetFirstGPUHandle(frameManager.GetGPUDescriptorHeap(),currentBackBufferIndex));

	DrawMesh(entity->GetMesh());
}


void Game::OnResize()
{
	DXCore::OnResize();

	/*XMMATRIX P = XMMatrixPerspectiveFovLH(
		0.25f * 3.1415926535f,	
		(float)width / height,	
		0.1f,				  	
		100.0f);			  	
	XMStoreFloat4x4(&projectionMatrix, XMMatrixTranspose(P)); */
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

	//SRT

	/*entities[0]->SetPosition(job2.pos);
	auto bounds = entities[0]->GetBoundingOrientedBox();*/
	entities[0]->SetPosition(XMFLOAT3(1.0f, 0.0f, 2.0f));

	entities[1]->SetPosition(XMFLOAT3(2.0f, 0.0f, 2.0f));
	entities[1]->SetMaterial(&m_scratchedPaint);

	entities[2]->SetPosition(XMFLOAT3(1.0f, 0.0f, 4.0f));
	entities[2]->SetMaterial(&m_water);

	entities[3]->SetPosition(XMFLOAT3(2.0f, 0.0f, 4.0f));
	entities[3]->SetMaterial(&m_water);

	entities[4]->SetPosition(XMFLOAT3(1.0f, 0.0f, 6.0f));
	entities[4]->SetMaterial(&m_cobbleStone);

	entities[5]->SetPosition(XMFLOAT3(2.0f, 0.0f, 6.0f));
	entities[5]->SetMaterial(&m_paint);

	entities[6]->SetMesh(sm_skyCube);
	entities[6]->SetMaterial(&m_floor);
	entities[6]->SetScale(XMFLOAT3(25.0f, 0.2f, 25.0f));
	entities[6]->SetPosition(XMFLOAT3(-2.0f, -3.0f, 10.0f));

	e_plane->SetScale(XMFLOAT3(0.7f, 0.7f, 0.7f));
	e_plane->SetRotation(XMFLOAT3(-90.0f, -90.0f, 0.0f));
	e_plane->SetPosition(XMFLOAT3(-2.8f, 2.0f, 2.0f));
	e_plane->SetMaterial(&m_plane);

	e_sponza->SetScale(XMFLOAT3(0.02f, 0.02f, 0.02f));
	//e_sponza->SetRotation(XMFLOAT3(-90.0f, -90.0f, 0.0f));
	e_sponza->SetPosition(XMFLOAT3(0, 0.0f, 10.0f));

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

	//frameManager.CopyData(&pixelData, sizeof(PixelShaderExternalData), pixelCBV);
	//frameManager.CopyData(&transparencyData, sizeof(TransparencyExternalData), transparencyCBV, currentBackBufferIndex);
	//vsConstBufferUploadHeap->Unmap(0, 0);
}


void Game::Draw(float deltaTime, float totalTime)
{
	WaitForGPU();
	// Figure out which buffer is next
	previousBackBufferIndex = currentBackBufferIndex;
	currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();

	commandAllocator[currentBackBufferIndex]->Reset();
	commandList->Reset(commandAllocator[currentBackBufferIndex], 0);

	ID3D12Resource* backBuffer = backBuffers[currentBackBufferIndex];

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
			rtvHandles[currentBackBufferIndex],
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
		commandList->SetPipelineState(outlinePipeState);

		// Root sig (must happen before root descriptor table)
		commandList->SetGraphicsRootSignature(rootSignature);

		// Set up other commands for rendering
		commandList->OMSetRenderTargets(1, &rtvHandles[currentBackBufferIndex], true, &dsvHandle);
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Set constant buffer
		commandList->SetDescriptorHeaps(1, frameManager.GetGPUDescriptorHeap(currentBackBufferIndex).pDescriptorHeap.GetAddressOf());
		
		//for pixel shader
		commandList->SetGraphicsRootDescriptorTable(
			1,
			frameManager.GetGPUHandle(pixelCBV.heapIndex, currentBackBufferIndex));

		//for image based lighting
		commandList->SetGraphicsRootDescriptorTable(
			3,
			skyIrradiance.GetGPUHandle(frameManager.GetGPUDescriptorHeap(), currentBackBufferIndex));

		commandList->SetPipelineState(pbrPipeState);
		//DrawEntity(entities[0]);
		//DrawEntity(entities[1]);
		//DrawEntity(entities[6]);
		DrawEntity(e_sponza);

		commandList->SetPipelineState(toonShadingPipeState);
		//DrawEntity(entities[4]);
		//DrawEntity(entities[5]);
		DrawEntity(e_plane);

		DrawSky();

		//transparent objects are drawn last
		commandList->SetGraphicsRootDescriptorTable(
			1,
			frameManager.GetGPUHandle(transparencyCBV.heapIndex, currentBackBufferIndex));
		commandList->SetPipelineState(transparencyPipeState);
		// TO DO: Fix Transparency to use depth
		//DrawTransparentEntity(entities[2], 0.02f);
		//DrawTransparentEntity(entities[3], 0.08f);
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

		//WaitForGPU();

	}
}

void Game::DrawMesh(Mesh* mesh)
{
	commandList->IASetVertexBuffers(0, 1, &mesh->GetVertexBufferView());
	commandList->IASetIndexBuffer(&mesh->GetIndexBufferView());
	int i = 0;
	// Check if mesh is complex or simple
	if (mesh->MeshEntries.size() > 1)
	{
		for (auto m : mesh->MeshEntries)
		{
			auto mat = sponzaMat[i];
			//commandList->SetGraphicsRootDescriptorTable(0, frameManager.GetGPUHandle(mat.materialIndex, currentBackBufferIndex));
			commandList->SetGraphicsRootDescriptorTable(2, mat.GetFirstGPUHandle(frameManager.GetGPUDescriptorHeap(), currentBackBufferIndex));

			commandList->DrawIndexedInstanced(m.NumIndices, 1, m.BaseIndex, m.BaseVertex, 0);
			i++;
		}
		
	}
	
	else
	{
		commandList->DrawIndexedInstanced(mesh->GetIndexCount(), 1, 0, 0, 0);
	}
}

void Game::CreateMaterials()
{
	m_default = frameManager.CreateMaterial(
		"../../Assets/Textures/default/diffuse.png",
		"../../Assets/Textures/default/normal.png",
		"../../Assets/Textures/default/metal.png",
		"../../Assets/Textures/default/roughness.png",
		commandQueue);


	m_floor = frameManager.CreateMaterial(
		"../../Assets/Textures/floor/diffuse.png",
		"../../Assets/Textures/floor/normal.png",
		"../../Assets/Textures/floor/metal.png",
		"../../Assets/Textures/floor/roughness.png",
		commandQueue);

	m_scratchedPaint = frameManager.CreateMaterial(
		"../../Assets/Textures/scratched/diffuse.png",
		"../../Assets/Textures/scratched/normal.png",
		"../../Assets/Textures/scratched/metal.png",
		"../../Assets/Textures/scratched/roughness.png",
		commandQueue);

	m_cobbleStone = frameManager.CreateMaterial(
		"../../Assets/Textures/cobbleStone/diffuse.png",
		"../../Assets/Textures/cobbleStone/normal.png",
		"../../Assets/Textures/cobbleStone/metal.png",
		"../../Assets/Textures/cobbleStone/roughness.png",
		commandQueue);

	m_paint = frameManager.CreateMaterial(
		"../../Assets/Textures/paint/diffuse.png",
		"../../Assets/Textures/paint/normal.png",
		"../../Assets/Textures/paint/metal.png",
		"../../Assets/Textures/paint/roughness.png",
		commandQueue);

	m_water = frameManager.CreateMaterial(
		"../../Assets/Textures/water/diffuse.png",
		"../../Assets/Textures/water/normal.png",
		"../../Assets/Textures/default_metal.png",
		"../../Assets/Textures/water/roughness.png",
		commandQueue);

	m_plane = frameManager.CreateMaterial(
		"../../Assets/Textures/plane/diffuse.png",
		"../../Assets/Textures/plane/normal.png",
		"../../Assets/Textures/default_metal.png",
		"../../Assets/Textures/default_roughness.png",
		commandQueue);


}

void Game::CreateTextures()
{
	t_skyTexture = frameManager.CreateTexture(
		"../../Assets/Textures/skybox/envEnvHDR.dds",
		commandQueue,
		DDS);

	skyIrradiance = frameManager.CreateTexture(
		"../../Assets/Textures/skybox/envDiffuseHDR.dds",
		commandQueue,
		DDS);

	skyPrefilter = frameManager.CreateTexture(
		"../../Assets/Textures/skybox/envSpecularHDR.dds",
		commandQueue,
		DDS);

	brdfLookUpTexture = frameManager.CreateTexture(
		"../../Assets/Textures/skybox/envBrdf.dds",
		commandQueue,
		DDS);

	defaultDiffuse = frameManager.CreateTexture(
		"../../Assets/Textures/default/diffuse.png",
		commandQueue);

	defaultNormal = frameManager.CreateTexture(
		"../../Assets/Textures/default/normal.png",
		commandQueue);

	defaultRoughness = frameManager.CreateTexture(
		"../../Assets/Textures/default/defaultRoughness.png",
		commandQueue);

	defaultMetal = frameManager.CreateTexture(
		"../../Assets/Textures/default/metal.png",
		commandQueue);
}

void Game::CreateLights()
{
	//DIRECTIONAL LIGHTS: ambient diffuse direction intensity =====================
	directionalLight1 = { XMFLOAT4(+0.1f, +0.1f, +0.1f, 1.0f), XMFLOAT4(+1.0f, +1.0f, +1.0f, +1.0f), XMFLOAT3(0.2f, -2.0f, 1.8f), float(2) };

	//POINT LIGHTS: color position range intensity padding  =========================
	pointLight = { XMFLOAT4(0.5f, 0, 0, 0), XMFLOAT3(1, 0, 0), 10, 1 };
}

void Game::DrawSky()
{
	SkyboxExternalData skyboxExternalData = {};
	skyboxExternalData.view = camera->GetViewMatrixTransposed();
	skyboxExternalData.proj = camera->GetProjectionMatrixTransposed();

	frameManager.CopyData(&skyboxExternalData, sizeof(SkyboxExternalData), skyCBV, currentBackBufferIndex);

	commandList->SetPipelineState(skyPipeState.Get());
	commandList->SetGraphicsRootDescriptorTable(0, frameManager.GetGPUHandle(skyCBV.heapIndex, currentBackBufferIndex));
	commandList->SetGraphicsRootDescriptorTable(2, t_skyTexture.GetGPUHandle(frameManager.GetGPUDescriptorHeap(), currentBackBufferIndex));

	DrawMesh(sm_skyCube);
}

void Game::LoadSponza()
{
	// Sponza
	sponza = mLoader.LoadComplexModel("../../Assets/Models/Sponza.fbx", device.Get(), commandList);
	sm_sponza = sponza.Mesh;
	std::string sponzaDirectory = "../../Assets/Textures/sponza/";
	for (auto &material : sponza.Materials)
	{
		auto diffuse = sponzaDirectory + material.Diffuse;
		auto normal = sponzaDirectory + material.Normal;
		auto roughness = sponzaDirectory + material.Roughness;
		auto metal = sponzaDirectory + material.Metalness;

		if (material.Diffuse.empty())
		{
			diffuse = defaultDiffuse.GetName();
		}
		if (material.Normal.empty())
		{
			normal = defaultNormal.GetName();
		}
		if (material.Roughness.empty())
		{
			roughness = defaultRoughness.GetName();
		}
		if (material.Metalness.empty())
		{
			metal = defaultMetal.GetName();
		}

		Material m = frameManager.CreateMaterial(diffuse, normal, metal, roughness, commandQueue);
		sponzaMat.push_back(m);
	}
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
	prevMousePos.x = x;
	prevMousePos.y = y;

	SetCapture(hWnd);
	float distance = 0.0f;

	//if (IsIntersecting(entities[3], camera, x, y, distance) && isSelected)
	//{
	//	currentIndex = 0;
	//	pathFinderJob.currentPos = entities[selectedEntityIndex]->GetPosition();
	//	pathFinderJob.targetPos = newDestination;
	//	pathFinderJob.generator = &generator;
	//	auto f2 = pool.Enqueue(&pathFinderJob);
	//	isSelected = false;
	//}


	//for (int i = 0; i < entities.size(); ++i)
	//{
	//	if (IsIntersecting(entities[i], camera, x, y, distance) && i != 3)
	//	{
	//		selectedEntityIndex = i;
	//		isSelected = true;
	//		printf("Intersecting %d\n", i);
	//		printf("Selected Entity %d\n", selectedEntityIndex);
	//		break;
	//	}
	//	else if(i != 3)
	//	{
	//		//selectedEntityIndex = -1;
	//		printf("Unselect Entity \n");
	//		isSelected = false;
	//	}
	//}



}


void Game::OnMouseUp(WPARAM buttonState, int x, int y)
{
	ReleaseCapture();
}

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

void Game::OnMouseWheel(float wheelDelta, int x, int y)
{
	// Add any custom code here...
}
#pragma endregion