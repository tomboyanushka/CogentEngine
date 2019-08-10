#pragma once

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include "d3dx12.h"
#include <wrl.h>
// We can include the correct library files here
// instead of in Visual Studio settings if we want
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

class DXCore
{
public:
	DXCore(
		HINSTANCE hInstance,		// The application's handle
		const char* titleBarText,			// Text for the window's title bar
		unsigned int windowWidth,	// Width of the window's client area
		unsigned int windowHeight,	// Height of the window's client area
		bool debugTitleBarStats);	// Show extra stats (fps) in title bar?
	~DXCore();

	// Static requirements for OS-level message processing
	static DXCore* DXCoreInstance;
	static LRESULT CALLBACK WindowProc(
		HWND hWnd,		// Window handle
		UINT uMsg,		// Message
		WPARAM wParam,	// Message's first parameter
		LPARAM lParam	// Message's second parameter
	);

	// Internal method for message handling
	LRESULT ProcessMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	// Initialization and game-loop related methods
	HRESULT InitWindow();
	HRESULT InitDirectX();
	HRESULT Run();
	void Quit();
	virtual void OnResize();

	// Pure virtual methods for setup and game functionality
	virtual void Init() = 0;
	virtual void Update(float deltaTime, float totalTime) = 0;
	virtual void Draw(float deltaTime, float totalTime) = 0;

	// Convenience methods for handling mouse input, since we
	// can easily grab mouse input from OS-level messages
	virtual void OnMouseDown(WPARAM buttonState, int x, int y) { }
	virtual void OnMouseUp(WPARAM buttonState, int x, int y) { }
	virtual void OnMouseMove(WPARAM buttonState, int x, int y) { }
	virtual void OnMouseWheel(float wheelDelta, int x, int y) { }

protected:
	HINSTANCE	hInstance;		// The handle to the application
	HWND		hWnd;			// The handle to the window itself
	std::string titleBarText;	// Custom text in window's title bar
	bool		titleBarStats;	// Show extra stats in title bar?

	// Size of the window's client area
	unsigned int width;
	unsigned int height;

	// How many swap chain buffers
	static const unsigned int numBackBuffers = 2;
	unsigned int currentSwapBuffer = 0;

	// DirectX related objects and variables
	D3D_FEATURE_LEVEL		dxFeatureLevel;
	IDXGIFactory*			dxgiFactory;
	IDXGISwapChain*			swapChain;
	Microsoft::WRL::ComPtr<ID3D12Device>	device;

	bool					vsync = false;

	D3D12_VIEWPORT			viewport;
	D3D12_RECT				scissorRect;

	ID3D12GraphicsCommandList*	commandList;
	ID3D12CommandQueue*			commandQueue;
	ID3D12CommandAllocator*		commandAllocator;

	unsigned int			rtvDescriptorSize;
	ID3D12DescriptorHeap*	rtvHeap;	// Heap that will store RTV's (probably 2, for back buffer swaps)
	ID3D12DescriptorHeap*	dsvHeap;

	ID3D12Resource* backBuffers[numBackBuffers];
	ID3D12Resource* depthStencilBuffer;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[numBackBuffers]; // Pointers into the RTV desc heap
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle;

	// Fence for CPU/GPU sync
	ID3D12Fence* fence;
	HANDLE fenceEvent;
	unsigned long currentFence = 0;

	// DX12 Helper Functions
	void WaitForGPU();
	void CloseExecuteAndResetCommandList();
	HRESULT CreateStaticBuffer(unsigned int dataStride, unsigned int dataCount, void* data, ID3D12Resource** buffer);
	HRESULT CreateIndexBuffer(DXGI_FORMAT format, unsigned int dataCount, void* data, ID3D12Resource** buffer, D3D12_INDEX_BUFFER_VIEW* ibView);
	HRESULT CreateVertexBuffer(unsigned int dataStride, unsigned int dataCount, void* data, ID3D12Resource** buffer, D3D12_VERTEX_BUFFER_VIEW* vbView);


	// Helper function for allocating a console window
	void CreateConsoleWindow(int bufferLines, int bufferColumns, int windowLines, int windowColumns);

private:
	// Timing related data
	double perfCounterSeconds;
	float totalTime;
	float deltaTime;
	__int64 startTime;
	__int64 currentTime;
	__int64 previousTime;

	// FPS calculation
	int fpsFrameCount;
	float fpsTimeElapsed;

	void UpdateTimer();			// Updates the timer for this frame
	void UpdateTitleBarStats();	// Puts debug info in the title bar

	unsigned int SizeOfDXGIFormat(DXGI_FORMAT format);
};

