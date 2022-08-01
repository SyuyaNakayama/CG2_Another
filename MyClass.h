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
#include "Buffer.h"

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
	ComPtr<ID3DBlob> blob = nullptr;

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
	ComPtr<ID3D12PipelineState> state;

	Pipeline();
	void SetShader(ShaderBlob vs, ShaderBlob ps);
	void SetInputLayout(D3D12_INPUT_ELEMENT_DESC* inputLayout, UINT layoutNum);
	void SetOthers();
	void CreatePipelineState(ID3D12Device* device);
};

class RootSignature
{
private:
	D3D12_ROOT_PARAMETER params[3];
	D3D12_ROOT_SIGNATURE_DESC desc;
	ComPtr<ID3DBlob> blob;
public:
	ComPtr<ID3D12RootSignature> rs;

	RootSignature();
	void SetParam(D3D12_DESCRIPTOR_RANGE descriptorRange);
	void SetRootSignature(D3D12_STATIC_SAMPLER_DESC samplerDesc);
	void SerializeRootSignature(ID3D12Device* device, ID3DBlob* errorBlob);
};

class DirectXInit
{
private:
	std::vector<ComPtr<IDXGIAdapter4>> adapters;
	ComPtr<IDXGIAdapter4> tmpAdapter = nullptr;
	D3D_FEATURE_LEVEL featureLevel{};
public:
	ComPtr<IDXGIFactory7> dxgiFactory;

	DirectXInit();
	void AdapterChoice();
	ID3D12Device* CreateDevice(D3D_FEATURE_LEVEL* levels, size_t levelsNum, ID3D12Device* device);
};

class RenderTargetView
{
protected:
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	ComPtr<ID3D12Device> devicePtr;
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
	std::vector<ComPtr<ID3D12Resource>> backBuffers;
	ComPtr<IDXGISwapChain4> sc;

	SwapChain(ID3D12Device* device);
	void Create(IDXGIFactory7*, ID3D12CommandQueue* commandQueue, HWND hwnd);
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
	ComPtr<ID3D12Device> devicePtr;
	D3D12_COMMAND_QUEUE_DESC queueDesc;
	ComPtr<ID3D12CommandAllocator> allocator;
	ID3D12CommandList* cLists;
public:
	ComPtr<ID3D12GraphicsCommandList> list;
	ComPtr<ID3D12CommandQueue> queue;

	Command(ID3D12Device* device);
	void CreateCommandAllocator();
	void CreateCommandList();
	void CreateCommandQueue();
	void Reset();
	void ExecuteCommandLists();
};

class Fence
{
private:
	HANDLE event;
public:
	ComPtr<ID3D12Fence> f;
	UINT64 val;

	Fence();
	void CreateFence(ID3D12Device* device);
	void Wait();
};

class ShaderResourceView
{
private:
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
public:
	enum Type { CPU, GPU };

	ID3D12DescriptorHeap* heap;
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE handle;

	void SetHeapDesc();
	void CreateDescriptorHeap(ID3D12Device* device);
	void GetDescriptorHandleForHeapStart(Type type);
};

class ViewProjection
{
private:
	XMMATRIX view, projection;

public:
	XMFLOAT3 eye, target, up;

	ViewProjection(XMFLOAT3 eye, XMFLOAT3 target = {}, XMFLOAT3 up = { 0,1,0 });
	void CreateViewMatrix();
	void CreateProjectionMatrix(Int2 windowSize);
	XMMATRIX GetViewProjectionMatrix() { return view * projection; }
};

class WorldTransform
{
protected:
	XMMATRIX matWorld, matScale, matRot, matTrans;

	void InitializeMatrix();
public:
	XMFLOAT3 scale, rot, trans;
	WorldTransform* parent = nullptr;

	WorldTransform();
	WorldTransform(XMFLOAT3 scale, XMFLOAT3 rot, XMFLOAT3 trans);
	void UpdateMatrix();
};

class Object3d :public ConstBuf, public WorldTransform
{
public:
	Object3d(ID3D12Device* device, Type type = Transform);
	void TransferMatrix(ViewProjection viewProjection);
};