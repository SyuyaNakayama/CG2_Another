#include "MyClass.h"

ShaderBlob::ShaderBlob(const LPCWSTR fileName, const LPCSTR target, ID3DBlob* errorBlob)
{
	HRESULT result;

	result = D3DCompileFromFile(
		fileName, // シェーダファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
		"main", target, // エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用設定
		0,
		&blob, &errorBlob);

	if (FAILED(result)) {
		// errorBlobからエラー内容をstring型にコピー
		std::string error;
		error.resize(errorBlob->GetBufferSize());
		std::copy_n((char*)errorBlob->GetBufferPointer(),
			errorBlob->GetBufferSize(),
			error.begin());
		error += "\n";
		// エラー内容を出力ウィンドウに表示
		OutputDebugStringA(error.c_str());
		assert(0);
	}
}

// ウィンドウプロシージャ
LRESULT WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	// メッセージに応じてゲーム固有の処理を行う
	switch (msg)
	{
		// ウィンドウが破棄された
	case WM_DESTROY:
		// OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	// 標準のメッセージ処理を行う
	return DefWindowProc(hwnd, msg, wparam, lparam);
}
WindowsAPI::WindowsAPI(WNDPROC lpfnWndProc, Int2 WIN_SIZE)
{
	w.cbSize = sizeof(WNDCLASSEX);
	w.lpfnWndProc = lpfnWndProc; // ウィンドウプロシージャを設定
	w.lpszClassName = L"DirectXGame"; // ウィンドウクラス名
	w.hInstance = GetModuleHandle(nullptr); // ウィンドウハンドル
	w.hCursor = LoadCursor(NULL, IDC_ARROW); // カーソル指定

	// ウィンドウクラスをOSに登録する
	RegisterClassEx(&w);

	wrc = { 0, 0, WIN_SIZE.width, WIN_SIZE.height };
	// 自動でサイズを補正する
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	hwnd = CreateWindow(w.lpszClassName, // クラス名
		L"LE2A_16_ナカヤマ_シュウヤ_CG2", // タイトルバーの文字
		WS_OVERLAPPEDWINDOW, // 標準的なウィンドウスタイル
		CW_USEDEFAULT, // 表示X座標(OSに任せる)
		CW_USEDEFAULT, // 表示Y座標(OSに任せる)
		wrc.right - wrc.left, // ウィンドウ横幅
		wrc.bottom - wrc.top, // ウィンドウ縦幅
		nullptr, // 親ウィンドウハンドル
		nullptr, // メニューハンドル
		w.hInstance, // 呼び出しアプリケーションハンドル
		nullptr); // オプション
}
void WindowsAPI::MyUnregisterClass()
{
	UnregisterClass(w.lpszClassName, w.hInstance);
}

Pipeline::Pipeline()
{
	desc = {};
	state = nullptr;
}
void Pipeline::SetShader(ShaderBlob vs, ShaderBlob ps)
{
	desc.VS.pShaderBytecode = vs.blob->GetBufferPointer();
	desc.VS.BytecodeLength = vs.blob->GetBufferSize();
	desc.PS.pShaderBytecode = ps.blob->GetBufferPointer();
	desc.PS.BytecodeLength = ps.blob->GetBufferSize();
}
void Pipeline::SetInputLayout(D3D12_INPUT_ELEMENT_DESC* inputLayout, UINT layoutNum)
{
	desc.InputLayout.pInputElementDescs = inputLayout;
	desc.InputLayout.NumElements = layoutNum;
}
void Pipeline::SetOthers()
{
	// サンプルマスクの設定
	desc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK; // 標準設定
	// ラスタライザの設定
	desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK; // 背面をカリング
	desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; // ポリゴン内塗りつぶし
	desc.RasterizerState.DepthClipEnable = true; // 深度クリッピングを有効に
	// 図形の形状設定
	desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// その他の設定
	desc.NumRenderTargets = 1; // 描画対象は1つ
	desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 0~255指定のRGBA
	desc.SampleDesc.Count = 1; // 1ピクセルにつき1回サンプリング
	// デプスステンシルステートの設定
	desc.DepthStencilState.DepthEnable = true;
	desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
}
void Pipeline::CreatePipelineState(ID3D12Device* device)
{
	assert(SUCCEEDED(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&state))));
}

RootSignature::RootSignature()
{
	params[0] = {};
	params[1] = {};
	desc = {};
	rs = nullptr;
	blob = nullptr;
}
void RootSignature::SetParam(D3D12_DESCRIPTOR_RANGE descriptorRange)
{
	for (UINT i = 0, j = 0; i < 3; i += 2)
	{
		params[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	// 定数バッファビュー
		params[i].Descriptor.ShaderRegister = j++;					// 定数バッファ番号
		params[i].Descriptor.RegisterSpace = 0;						// デフォルト値
		params[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;	// 全てのシェーダから見える
	}

	params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	params[1].DescriptorTable.pDescriptorRanges = &descriptorRange;
	params[1].DescriptorTable.NumDescriptorRanges = 1;
	params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
}
void RootSignature::SetRootSignature(D3D12_STATIC_SAMPLER_DESC samplerDesc)
{
	desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	desc.pParameters = params;
	desc.NumParameters = _countof(params);
	desc.pStaticSamplers = &samplerDesc;
	desc.NumStaticSamplers = 1;
}
void RootSignature::SerializeRootSignature(ID3D12Device* device, ID3DBlob* errorBlob)
{
	HRESULT result = D3D12SerializeRootSignature(&desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0, &blob, &errorBlob);
	assert(SUCCEEDED(result));
	result = device->CreateRootSignature(0, blob->GetBufferPointer(),
		blob->GetBufferSize(), IID_PPV_ARGS(&rs));
	assert(SUCCEEDED(result));
	blob->Release();
}

DirectXInit::DirectXInit()
{
	assert(SUCCEEDED(CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory))));
}
void DirectXInit::AdapterChoice()
{
	// パフォーマンスが高いものから順に、全てのアダプターを列挙する
	for (UINT i = 0;
		dxgiFactory->EnumAdapterByGpuPreference(i,
			DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
			IID_PPV_ARGS(&tmpAdapter)) != DXGI_ERROR_NOT_FOUND;
		i++)
	{
		// 動的配列に追加する
		adapters.push_back(tmpAdapter);
	}
	// 妥当なアダプタを選別する
	for (size_t i = 0; i < adapters.size(); i++)
	{
		DXGI_ADAPTER_DESC3 adapterDesc;
		// アダプターの情報を取得する
		adapters[i]->GetDesc3(&adapterDesc);
		// ソフトウェアデバイスを回避
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE))
		{
			// デバイスを採用してループを抜ける
			tmpAdapter = adapters[i];
			break;
		}
	}
}
ID3D12Device* DirectXInit::CreateDevice(D3D_FEATURE_LEVEL* levels, size_t levelsNum, ID3D12Device* device)
{
	HRESULT result;

	for (size_t i = 0; i < levelsNum; i++)
	{
		// 採用したアダプターでデバイスを生成
		result = D3D12CreateDevice(tmpAdapter.Get(), levels[i], IID_PPV_ARGS(&device));
		if (result == S_OK)
		{
			// デバイスを生成できた時点でループを抜ける
			featureLevel = levels[i];
			return device;
		}
	}
	return nullptr;
}

RenderTargetView::RenderTargetView()
{
	rtvHeap = nullptr;
	rtvHeapDesc = {};
	rtvDesc = {};
	rtvHandle = {};
	devicePtr = nullptr;
}
void RenderTargetView::GetHandle()
{
	rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += bbIndex * devicePtr->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
}

SwapChain::SwapChain(ID3D12Device* device)
{
	scDesc.Width = 1280;
	scDesc.Height = 720;
	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色情報の書式
	scDesc.SampleDesc.Count = 1; // マルチサンプルしない
	scDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER; // バックバッファ用
	scDesc.BufferCount = 2; // バッファ数を2つに設定
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // フリップ後は破棄
	scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	backBuffers.resize(scDesc.BufferCount);
	devicePtr = device;
	sc = nullptr;
}
void SwapChain::Create(IDXGIFactory7* dxgiFactory, ID3D12CommandQueue* commandQueue, HWND hwnd)
{
	ComPtr<IDXGISwapChain1> swapchain1;

	assert(SUCCEEDED(
		dxgiFactory->CreateSwapChainForHwnd(
			commandQueue, hwnd, &scDesc, nullptr, nullptr,
			&swapchain1)));

	swapchain1.As(&sc);
}
void SwapChain::CreateRenderTargetView()
{
	for (size_t i = 0; i < backBuffers.size(); i++)
	{
		sc->GetBuffer((UINT)i, IID_PPV_ARGS(&backBuffers[i]));
		rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += i * devicePtr->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
		rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		devicePtr->CreateRenderTargetView(backBuffers[i].Get(), &rtvDesc, rtvHandle);
	}
}
void SwapChain::CreateDescriptorHeap()
{
	// デスクリプタヒープの設定
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // レンダーターゲットビュー
	rtvHeapDesc.NumDescriptors = scDesc.BufferCount; // 裏表の2つ
	// デスクリプタヒープの生成
	devicePtr->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap));
}
ID3D12Resource* SwapChain::GetBackBuffersPtr()
{
	bbIndex = sc->GetCurrentBackBufferIndex();
	return backBuffers[bbIndex].Get();
}

Blend::Blend(D3D12_RENDER_TARGET_BLEND_DESC* blenddesc)
{
	desc = blenddesc;
	desc->RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
}
void Blend::UseBlendMode()
{
	desc->BlendEnable = true;
	desc->BlendOpAlpha = D3D12_BLEND_OP_ADD;	// 加算
	desc->SrcBlendAlpha = D3D12_BLEND_ONE;		// ソースの値を100%使う
	desc->DestBlendAlpha = D3D12_BLEND_ZERO;	// 使わない
}
void Blend::SetBlend(BlendMode blendMode)
{
	switch (blendMode)
	{
	case Blend::ADD:
		desc->BlendOp = D3D12_BLEND_OP_ADD;
		desc->SrcBlend = D3D12_BLEND_ONE;
		desc->DestBlend = D3D12_BLEND_ONE;
		break;
	case Blend::SUB:
		desc->BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
		desc->SrcBlend = D3D12_BLEND_ONE;
		desc->DestBlend = D3D12_BLEND_ONE;
		break;
	case Blend::COLORFLIP:
		desc->BlendOp = D3D12_BLEND_OP_ADD;
		desc->SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
		desc->DestBlend = D3D12_BLEND_ZERO;
		break;
	case Blend::ALPHA:
		desc->BlendOp = D3D12_BLEND_OP_ADD;
		desc->SrcBlend = D3D12_BLEND_SRC_ALPHA;
		desc->DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		break;
	}
}

ResourceBarrier::ResourceBarrier()
{
	desc = {};
	desc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
}
void ResourceBarrier::SetState(ID3D12GraphicsCommandList* commandList)
{
	static int state = 0;
	if (!(state++))
	{
		desc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		desc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}
	else
	{
		desc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		desc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}
	commandList->ResourceBarrier(1, &desc);
	state %= 2;
}

Command::Command(ID3D12Device* device)
{
	allocator = nullptr;
	list = nullptr;
	queue = nullptr;
	queueDesc = {};
	devicePtr = device;
}
void Command::CreateCommandAllocator()
{
	assert(SUCCEEDED(
		devicePtr->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&allocator))));
}
void Command::CreateCommandList()
{
	assert(SUCCEEDED(
		devicePtr->CreateCommandList(0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			allocator.Get(), nullptr,
			IID_PPV_ARGS(&list))));
}
void Command::CreateCommandQueue()
{
	assert(SUCCEEDED(
		devicePtr->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue))));
}
void Command::Reset()
{
	assert(SUCCEEDED(allocator->Reset())); // キューをクリア
	assert(SUCCEEDED(list->Reset(allocator.Get(), nullptr))); // 再びコマンドリストを貯める準備
}
void Command::ExecuteCommandLists()
{
	cLists = list.Get();
	queue->ExecuteCommandLists(1, &cLists);
}

Fence::Fence()
{
	f = nullptr;
	val = 0;
}
void Fence::CreateFence(ID3D12Device* device)
{
	assert(SUCCEEDED(device->CreateFence(val, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&f))));
}
void Fence::Wait()
{
	if (f->GetCompletedValue() != val)
	{
		HANDLE event = CreateEvent(nullptr, false, false, nullptr);
		f->SetEventOnCompletion(val, event);
		if (event != 0)
		{
			WaitForSingleObject(event, INFINITE);
			CloseHandle(event);
		}
	}
}

void ShaderResourceView::SetHeapDesc()
{
	heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NumDescriptors = 2056;
}
void ShaderResourceView::CreateDescriptorHeap(ID3D12Device* device)
{
	assert(SUCCEEDED(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap))));
}
void ShaderResourceView::GetDescriptorHandleForHeapStart(Type type)
{
	if (type == CPU) { handle = heap->GetCPUDescriptorHandleForHeapStart(); }
	if (type == GPU) { gpuHandle = heap->GetGPUDescriptorHandleForHeapStart(); }
}

ViewProjection::ViewProjection(XMFLOAT3 eye, XMFLOAT3 target, XMFLOAT3 up)
{
	view = XMMatrixIdentity();
	projection = XMMatrixIdentity();
	this->eye = eye;
	this->target = target;
	this->up = up;
}
void ViewProjection::CreateViewMatrix()
{
	view = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
}
void ViewProjection::CreateProjectionMatrix(Int2 windowSize)
{
	projection = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(45.0f), (float)windowSize.width / windowSize.height, 0.1f, 1000.0f);
}

void WorldTransform::InitializeMatrix()
{
	matWorld = XMMatrixIdentity();
	matScale = XMMatrixIdentity();
	matRot = XMMatrixIdentity();
	matTrans = XMMatrixIdentity();
}
WorldTransform::WorldTransform()
{
	InitializeMatrix();
	scale = { 1.0f,1.0f,1.0f };
	rot = {};
	trans = {};
}
WorldTransform::WorldTransform(XMFLOAT3 scale, XMFLOAT3 rot, XMFLOAT3 trans)
{
	InitializeMatrix();
	this->scale = scale;
	this->rot = rot;
	this->trans = trans;
}
void WorldTransform::UpdateMatrix()
{
	matScale = XMMatrixScaling(scale.x, scale.y, scale.z);
	matRot = XMMatrixRotationZ(rot.z) * XMMatrixRotationX(rot.x) * XMMatrixRotationY(rot.y);
	matTrans = XMMatrixTranslation(trans.x, trans.y, trans.z);
	matWorld = matScale * matRot * matTrans;
	if (parent != nullptr) { matWorld *= parent->matWorld; }
}

Object3d::Object3d(ID3D12Device* device, Type type) :ConstBuf(type)
{
	SetResource(size, 1, D3D12_RESOURCE_DIMENSION_BUFFER);
	SetHeapProp(D3D12_HEAP_TYPE_UPLOAD); // ヒープ設定
	CreateBuffer(device);
	Mapping(); // 定数バッファのマッピング
}
void Object3d::TransferMatrix(ViewProjection viewProjection)
{
	mapTransform->mat = matWorld * viewProjection.GetViewProjectionMatrix();
}