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

enum BlendMode
{
	BLENDMODE_ADD,
	BLENDMODE_SUB,
	BLENDMODE_COLORFLIP,
	BLENDMODE_ALPHA,
};
void UseBlendMode(D3D12_RENDER_TARGET_BLEND_DESC& blenddesc);
void SetBlend(D3D12_RENDER_TARGET_BLEND_DESC& blenddesc, int blendMode = BLENDMODE_ADD);
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
	std::vector<IDXGIAdapter4*> adapters;
	IDXGIAdapter4* tmpAdapter;
	D3D_FEATURE_LEVEL featureLevel;
public:
	IDXGIFactory7* dxgiFactory;

	DirectXInit();
	void AdapterChoice();
	ID3D12Device* CreateDevice(D3D_FEATURE_LEVEL* levels, size_t levelsNum, ID3D12Device* device);
};

class SwapChain
{
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
public:
	std::vector<ID3D12Resource*> backBuffers;
	IDXGISwapChain4* sc;
	DXGI_SWAP_CHAIN_DESC1 desc;
	SwapChain()
	{
		desc.Width = 1280;
		desc.Height = 720;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色情報の書式
		desc.SampleDesc.Count = 1; // マルチサンプルしない
		desc.BufferUsage = DXGI_USAGE_BACK_BUFFER; // バックバッファ用
		desc.BufferCount = 2; // バッファ数を2つに設定
		desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // フリップ後は破棄
		desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		backBuffers.resize(desc.BufferCount);
		rtvDesc = {};
		sc = nullptr;
	}
	void Create(IDXGIFactory7* dxgiFactory, ID3D12CommandQueue* commandQueue, HWND hwnd)
	{
		assert(SUCCEEDED(
			dxgiFactory->CreateSwapChainForHwnd(
				commandQueue, hwnd, &desc, nullptr, nullptr,
				(IDXGISwapChain1**)&sc)));
	}
	void Set(ID3D12Device* device, ID3D12DescriptorHeap* rtvHeap, D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc)
	{
		for (size_t i = 0; i < backBuffers.size(); i++) {
			sc->GetBuffer((UINT)i, IID_PPV_ARGS(&backBuffers[i]));
			rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
			rtvHandle.ptr += i * device->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
			rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			device->CreateRenderTargetView(backBuffers[i], &rtvDesc, rtvHandle);
		}
	}
	void Flip()
	{
		assert(SUCCEEDED(sc->Present(1, 0)));
	}
};