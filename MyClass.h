#pragma once
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#include <vector>
#include <string>
#include <DirectXMath.h>
#include <d3dcompiler.h>
#include <dinput.h>
#include <DirectXTex.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
using namespace DirectX;

LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
struct Int2 { int width, height; };

class ShaderBlob
{
public:
	ID3DBlob* blob = nullptr;

	ShaderBlob(const LPCWSTR fileName, const LPCSTR target, ID3DBlob* errorBlob);
};

class WindowsAPI
{
private:
	RECT wrc;
public:
	WNDCLASSEX w;
	HWND hwnd;

	WindowsAPI(WNDPROC lpfnWndProc, Int2 WIN_SIZE);
	void MyUnregisterClass();
};

class Pipeline
{
public:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc;
	ID3D12PipelineState* state;

	Pipeline();
	void SetShader(ShaderBlob vs, ShaderBlob ps);
	void SetSampleMask();
	void SetRasterizer();
	void SetInputLayout(D3D12_INPUT_ELEMENT_DESC* inputLayout, UINT layoutNum);
	void SetPrimitiveTopology();
	void SetOthers();
	void CreatePipelineState(ID3D12Device* device);
};

class RootSignature
{
private:
	D3D12_ROOT_PARAMETER params[3];
	D3D12_ROOT_SIGNATURE_DESC desc;
	ID3DBlob* blob;
public:
	ID3D12RootSignature* rs;

	RootSignature();
	void SetParam(D3D12_DESCRIPTOR_RANGE descriptorRange);
	void SetRootSignature(D3D12_STATIC_SAMPLER_DESC samplerDesc);
	void SerializeRootSignature(ID3D12Device* device, ID3DBlob* errorBlob);
};

class DirectXInit
{
private:
	std::vector<IDXGIAdapter4*> adapters;
	IDXGIAdapter4* tmpAdapter;
	D3D_FEATURE_LEVEL featureLevel;
public:
	IDXGIFactory7* dxgiFactory;

	DirectXInit();
	void AdapterChoice();
	ID3D12Device* CreateDevice(D3D_FEATURE_LEVEL* levels, size_t levelsNum, ID3D12Device* device);
};

class RenderTargetView
{
protected:
	ID3D12DescriptorHeap* rtvHeap;
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	ID3D12Device* devicePtr;
	UINT bbIndex;
public:
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;

	RenderTargetView();
	void GetHandle();
};

class SwapChain :public RenderTargetView
{
private:
	DXGI_SWAP_CHAIN_DESC1 scDesc;

public:
	std::vector<ID3D12Resource*> backBuffers;
	IDXGISwapChain4* sc;

	SwapChain(ID3D12Device* device);
	void Create(IDXGIFactory7* dxgiFactory, ID3D12CommandQueue* commandQueue, HWND hwnd);
	void CreateRenderTargetView();
	void Flip() { assert(SUCCEEDED(sc->Present(1, 0))); }
	void CreateDescriptorHeap();
	ID3D12Resource* GetBackBuffersPtr();
};

class Blend
{
private:
	D3D12_RENDER_TARGET_BLEND_DESC* desc;
public:
	enum BlendMode
	{
		ADD,
		SUB,
		COLORFLIP,
		ALPHA,
	};

	Blend(D3D12_RENDER_TARGET_BLEND_DESC* blenddesc);
	void UseBlendMode();
	void SetBlend(BlendMode blendMode);
};

class ResourceBarrier
{
public:
	D3D12_RESOURCE_BARRIER desc;

	ResourceBarrier();
	void SetState(ID3D12GraphicsCommandList* commandList);
};

class Command
{
private:
	ID3D12CommandAllocator* commandAllocator;
	ID3D12GraphicsCommandList* commandList;
	ID3D12CommandQueue* commandQueue;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	ID3D12Device* devicePtr;
public:
	Command(ID3D12Device* device)
	{
		commandAllocator = nullptr;
		commandList = nullptr;
		commandQueue = nullptr;
		commandQueueDesc = {};
		devicePtr = device;
	}
	void CreateCommandAllocator()
	{
		assert(SUCCEEDED(
			devicePtr->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&commandAllocator))));
	}
	void CreateCommandList()
	{
		assert(SUCCEEDED(
			devicePtr->CreateCommandList(0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			commandAllocator, nullptr,
			IID_PPV_ARGS(&commandList))));
	}
	void CreateCommandQueue()
	{
		assert(SUCCEEDED(
			devicePtr->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue))));
	}
};