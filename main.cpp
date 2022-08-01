#include "MyClass.h"
#include "Buffer.h"
#include "Input.h"

using namespace DirectX;
using namespace std;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
#pragma region WindowsAPI初期化処理
	// ウィンドウサイズ
	const Int2 WIN_SIZE = { 1280,720 }; // 横幅
	// ウィンドウクラスの設定
	WindowsAPI wAPI = { (WNDPROC)WindowProc,WIN_SIZE };

	// ウィンドウを表示状態にする
	ShowWindow(wAPI.hwnd, SW_SHOW);

	MSG msg{}; // メッセージ
#pragma endregion 
#pragma region DirectX初期化処理
#ifdef _DEBUG
	//デバッグレイヤーをオンに
	ComPtr<ID3D12Debug1> debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif
	ComPtr<ID3D12Device> device = nullptr;

	// 対応レベルの配列
	D3D_FEATURE_LEVEL levels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	DirectXInit directX{};
	directX.AdapterChoice();
	device = directX.CreateDevice(levels, _countof(levels), device.Get());
#ifdef _DEBUG
	ComPtr<ID3D12InfoQueue> infoQueue;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
	}
#endif
	Command command(device.Get());
	// コマンドアロケータを生成
	command.CreateCommandAllocator();

	// コマンドリストを生成
	command.CreateCommandList();

	//コマンドキューを生成
	command.CreateCommandQueue();

	// スワップチェーンの設定
	SwapChain swapChain(device.Get());
	swapChain.Create(directX.dxgiFactory.Get(), command.queue.Get(), wAPI.hwnd);
	swapChain.CreateDescriptorHeap();
	swapChain.CreateRenderTargetView();
	// フェンスの生成
	Fence fence{};
	fence.CreateFence(device.Get());

	// DirectInputの初期化&キーボードデバイスの生成
	Keyboard keyboard;
	keyboard.GetInstance(wAPI.w);
	keyboard.Set(wAPI.hwnd);
#pragma endregion
#pragma region 描画初期化処理
#pragma region 定数バッファ
	ConstBuf cb = ConstBuf::Type::Material;
	cb.SetResource(cb.size, 1, D3D12_RESOURCE_DIMENSION_BUFFER);
	cb.SetHeapProp(D3D12_HEAP_TYPE_UPLOAD); // ヒープ設定
	cb.CreateBuffer(device.Get());
	cb.Mapping(); // 定数バッファのマッピング

	// 値を書き込むと自動的に転送される
	cb.mapMaterial->color = XMFLOAT4(1, 1, 1, 1);

	const size_t OBJ_COUNT = 50;
	vector<Object3d> object3ds;
	for (size_t i = 0; i < OBJ_COUNT; i++)
	{
		object3ds.push_back({ device.Get() });
	}

	ViewProjection viewProjection = { { 0,0,-100 } };
	viewProjection.CreateViewMatrix();
	viewProjection.CreateProjectionMatrix(WIN_SIZE);

	for (size_t i = 0; i < OBJ_COUNT; i++)
	{
		if (i > 0)
		{
			object3ds[i].parent = &object3ds[i - 1];
			object3ds[i].scale = { 0.9f,0.9f,0.9f };
			object3ds[i].rot = { 0,0,XMConvertToRadians(30.0f) };
			object3ds[i].trans = { 0,0,-8.0f };
		}
		object3ds[i].UpdateMatrix();
		object3ds[i].TransferMatrix(viewProjection);
	}
#pragma endregion
#pragma region 頂点バッファ
	// 頂点データ
	VertexBuf::Vertex vertices[] =
	{
		// 前
		{{-5.0f,-5.0f,-5.0f},{0.0f,1.0f}},
		{{-5.0f, 5.0f,-5.0f},{0.0f,0.0f}},
		{{ 5.0f,-5.0f,-5.0f},{1.0f,1.0f}},
		{{ 5.0f, 5.0f,-5.0f},{1.0f,0.0f}},
		// 後
		{{-5.0f,-5.0f, 5.0f},{0.0f,1.0f}},
		{{-5.0f, 5.0f, 5.0f},{0.0f,0.0f}},
		{{ 5.0f,-5.0f, 5.0f},{1.0f,1.0f}},
		{{ 5.0f, 5.0f, 5.0f},{1.0f,0.0f}},
		// 左
		{{-5.0f,-5.0f,-5.0f},{0.0f,1.0f}},
		{{-5.0f,-5.0f, 5.0f},{0.0f,0.0f}},
		{{-5.0f, 5.0f,-5.0f},{1.0f,1.0f}},
		{{-5.0f, 5.0f, 5.0f},{1.0f,0.0f}},
		// 右
		{{ 5.0f,-5.0f,-5.0f},{0.0f,1.0f}},
		{{ 5.0f,-5.0f, 5.0f},{0.0f,0.0f}},
		{{ 5.0f, 5.0f,-5.0f},{1.0f,1.0f}},
		{{ 5.0f, 5.0f, 5.0f},{1.0f,0.0f}},
		// 下
		{{-5.0f,-5.0f,-5.0f},{0.0f,1.0f}},
		{{-5.0f,-5.0f, 5.0f},{0.0f,0.0f}},
		{{ 5.0f,-5.0f,-5.0f},{1.0f,1.0f}},
		{{ 5.0f,-5.0f, 5.0f},{1.0f,0.0f}},
		// 上
		{{-5.0f, 5.0f,-5.0f},{0.0f,1.0f}},
		{{-5.0f, 5.0f, 5.0f},{0.0f,0.0f}},
		{{ 5.0f, 5.0f,-5.0f},{1.0f,1.0f}},
		{{ 5.0f, 5.0f, 5.0f},{1.0f,0.0f}},
	};

	// インデックスデータ
	uint16_t indices[] =
	{
		// 前
		0,1,2,
		2,1,3,
		// 後
		5,4,6,
		5,6,7,
		// 左
		8, 9,10,
		10,9,11,
		// 右
		13,12,14,
		13,14,15,
		// 下
		17,16,18,
		17,18,19,
		// 上
		20,21,22,
		22,21,23
	};

	for (int i = 0; i < _countof(vertices) / 2; i++)
	{
		std::vector<unsigned short>index;
		std::vector<XMVECTOR>p, v;

		for (int j = 0; j < 3; j++)
		{
			index.push_back(indices[i * 3 + j]);
			p.push_back(XMLoadFloat3(&vertices[index[j]].pos));
			if (j != 0) { v.push_back(XMVectorSubtract(p[j], p[0])); }
		}

		XMVECTOR normal = XMVector3Cross(v[0], v[1]);
		normal = XMVector3Normalize(normal);

		for (int j = 0; j < 3; j++) { XMStoreFloat3(&vertices[index[j]].normal, normal); }
	}

	VertexBuf vertex(static_cast<UINT>(sizeof(vertices[0]) * _countof(vertices)));
	vertex.SetResource(vertex.size, 1, D3D12_RESOURCE_DIMENSION_BUFFER);
	vertex.SetHeapProp(D3D12_HEAP_TYPE_UPLOAD);
	vertex.CreateBuffer(device.Get());
	vertex.Mapping(vertices, _countof(vertices));
	vertex.CreateView(); // 頂点バッファビューの作成
#pragma endregion
#pragma region インデックスバッファ
	IndexBuf index(static_cast<UINT>(sizeof(uint16_t) * _countof(indices)));
	index.SetResource(index.size, 1, D3D12_RESOURCE_DIMENSION_BUFFER);
	index.SetHeapProp(D3D12_HEAP_TYPE_UPLOAD);
	index.CreateBuffer(device.Get());
	index.Mapping(indices, _countof(indices));
	index.CreateView(); // インデックスビューの作成
#pragma endregion
#pragma region テクスチャバッファ
	ShaderResourceView srv{};
	srv.SetHeapDesc();
	srv.CreateDescriptorHeap(device.Get());
	srv.GetDescriptorHandleForHeapStart(ShaderResourceView::Type::CPU);
	TextureBuf texture[2] = { L"Resources/mario.jpg",L"Resources/reimu.png" };
	UINT incrementSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (size_t i = 0; i < 2; i++)
	{
		texture[i].SetResource();
		texture[i].SetHeapProp(D3D12_HEAP_TYPE_CUSTOM, D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
		texture[i].CreateBuffer(device.Get());
		texture[i].Transfer();
		texture[i].CreateView();
		device->CreateShaderResourceView(texture[i].buff.Get(), &texture[i].view, srv.handle);
		srv.handle.ptr += incrementSize;
	}
#pragma endregion
#pragma region シェーダ
	ComPtr<ID3DBlob> errorBlob = nullptr; // エラーオブジェクト
	ShaderBlob vs = { L"BasicVS.hlsl", "vs_5_0", errorBlob.Get() }; // 頂点シェーダの読み込みとコンパイル
	ShaderBlob ps = { L"BasicPS.hlsl", "ps_5_0", errorBlob.Get() }; // ピクセルシェーダの読み込みとコンパイル

	// 頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{	// xyz座標
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{	// 法線ベクトル
			"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		},
		{	// uv座標
			"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
			D3D12_APPEND_ALIGNED_ELEMENT,
			D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0
		}
	};
#pragma endregion
#pragma region パイプライン
	// 深度バッファ
	D3D12_RESOURCE_DESC depthResourceDesc{};
	depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResourceDesc.Width = WIN_SIZE.width;
	depthResourceDesc.Height = WIN_SIZE.height;
	depthResourceDesc.DepthOrArraySize = 1;
	depthResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthResourceDesc.SampleDesc.Count = 1;
	depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES depthHeapProp{};
	depthHeapProp.Type = D3D12_HEAP_TYPE_DEFAULT;

	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT;

	ComPtr<ID3D12Resource> depthBuff = nullptr;
	device->CreateCommittedResource(
		&depthHeapProp, D3D12_HEAP_FLAG_NONE,
		&depthResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue, IID_PPV_ARGS(&depthBuff));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	ComPtr<ID3D12DescriptorHeap> dsvHeap = nullptr;
	device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&dsvHeap));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device->CreateDepthStencilView(
		depthBuff.Get(), &dsvDesc, dsvHeap->GetCPUDescriptorHandleForHeapStart());

	// グラフィックスパイプライン設定
	Pipeline pipeline{};

	pipeline.SetShader(vs, ps); // シェーダーの設定
	pipeline.SetInputLayout(inputLayout, _countof(inputLayout)); // 頂点レイアウトの設定
	pipeline.SetOthers();

	// レンダーターゲットのブレンド設定
	Blend blend(&pipeline.desc.BlendState.RenderTarget[0]);
	blend.UseBlendMode();
	blend.SetBlend(Blend::BlendMode::ALPHA);

	D3D12_DESCRIPTOR_RANGE descriptorRange{};
	descriptorRange.NumDescriptors = 1;
	descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descriptorRange.BaseShaderRegister = 0;
	descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_STATIC_SAMPLER_DESC samplerDesc{};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	RootSignature rootSignature{};								// ルートシグネチャ
	rootSignature.SetParam(descriptorRange);					// ルートパラメータの設定
	rootSignature.SetRootSignature(samplerDesc);				// ルートシグネチャの設定
	rootSignature.SerializeRootSignature(device.Get(), errorBlob.Get());	// ルートシグネチャのシリアライズ
	// パイプラインにルートシグネチャをセット
	pipeline.desc.pRootSignature = rootSignature.rs.Get();
	// パイプランステートの生成
	pipeline.CreatePipelineState(device.Get());
#pragma endregion
#pragma endregion
#pragma region ゲームループで使う変数の定義
	float angle = 0.0f;
	ResourceBarrier barrier{};
	FLOAT clearColor[] = { 0.1f,0.25f,0.5f,0.0f }; // 青っぽい色
	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};
	
	bool texHandle = 0;
#pragma endregion
	// ゲームループ
	while (1)
	{
#pragma region ウィンドウメッセージ処理
		// メッセージがある?
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg); // キー入力メッセージの処理
			DispatchMessage(&msg); // プロシージャにメッセージを送る
		}

		// ✖ボタンで終了メッセージが来たらゲームループを抜ける
		if (msg.message == WM_QUIT) { break; }
#pragma endregion
#pragma region DirectX毎フレーム処理
#pragma region 更新処理
		keyboard.device->Acquire(); // キーボード情報の取得開始
		// 全キーの入力状態を取得する
		keyboard.TransferOldkey();
		keyboard.GetDeviceState();

		object3ds[0].trans.y += keyboard.Move(DIK_UP, DIK_DOWN, 1.0f);
		object3ds[0].trans.x += keyboard.Move(DIK_RIGHT, DIK_LEFT, 1.0f);

		if (keyboard.IsInput(DIK_D) || keyboard.IsInput(DIK_A))
		{
			angle += keyboard.Move(DIK_A, DIK_D, XMConvertToRadians(2.0f));

			viewProjection.eye.x = -100 * sinf(angle);
			viewProjection.eye.z = -100 * cosf(angle);

			viewProjection.CreateViewMatrix();
		}

		for (size_t i = 0; i < OBJ_COUNT; i++)
		{
			object3ds[i].UpdateMatrix();
			object3ds[i].TransferMatrix(viewProjection);
		}

		if (keyboard.IsTrigger(DIK_SPACE)) { texHandle = !texHandle; }
#pragma endregion
		// 1.リソースバリアで書き込み可能に変更
		barrier.desc.Transition.pResource = swapChain.GetBackBuffersPtr(); // バックバッファを指定
		barrier.SetState(command.list.Get());

		// 2.描画先の変更
		swapChain.GetHandle();
		D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
		command.list->OMSetRenderTargets(1, &swapChain.rtvHandle, false, &dsvHandle);

		// 3.画面クリアRGBA
		command.list->ClearRenderTargetView(swapChain.rtvHandle, clearColor, 0, nullptr);
		command.list->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
#pragma region 描画コマンド
		// ビューポート設定コマンド
		viewport.Width = (float)WIN_SIZE.width;
		viewport.Height = (float)WIN_SIZE.height;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		// シザー矩形
		scissorRect.left = 0; // 切り抜き座標左
		scissorRect.right = scissorRect.left + WIN_SIZE.width; // 切り抜き座標右
		scissorRect.top = 0; // 切り抜き座標上
		scissorRect.bottom = scissorRect.top + WIN_SIZE.height; // 切り抜き座標下

		// シザー矩形設定コマンドを、コマンドリストに積む
		command.list->RSSetScissorRects(1, &scissorRect);
		// パイプラインステートとルートシグネチャの設定コマンド
		command.list->SetPipelineState(pipeline.state.Get());
		command.list->SetGraphicsRootSignature(rootSignature.rs.Get());
		command.list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // プリミティブ形状の設定コマンド
		command.list->IASetVertexBuffers(0, 1, &vertex.view); // 頂点バッファビューの設定コマンド
		command.list->IASetIndexBuffer(&index.view); // 頂点バッファビューの設定コマンド
		command.list->RSSetViewports(1, &viewport); // ビューポート設定コマンドを、コマンドリストに積む
		// 定数バッファビューの設定コマンド
		command.list->SetGraphicsRootConstantBufferView(0, cb.buff->GetGPUVirtualAddress());
		command.list->SetDescriptorHeaps(1, &srv.heap);
		srv.GetDescriptorHandleForHeapStart(ShaderResourceView::Type::GPU);
		srv.gpuHandle.ptr += incrementSize * texHandle;
		command.list->SetGraphicsRootDescriptorTable(1, srv.gpuHandle);

		for (size_t i = 0; i < OBJ_COUNT; i++)
		{
			command.list->SetGraphicsRootConstantBufferView(2, object3ds[i].buff->GetGPUVirtualAddress());
			// 描画コマンド
			command.list->DrawIndexedInstanced(_countof(indices), 1, 0, 0, 0); // 全ての頂点を使って描画
		}
#pragma endregion
#pragma endregion
#pragma region 画面入れ替え
		// 5.リソースバリアを戻す
		barrier.SetState(command.list.Get());
		// 命令のクローズ
		assert(SUCCEEDED(command.list->Close()));
		// コマンドリストの実行
		command.ExecuteCommandLists();
		// 画面に表示するバッファをフリップ(裏表の入替え)
		swapChain.Flip();

		// コマンドの実行完了を待つ
		command.queue->Signal(fence.f.Get(), ++fence.val);
		fence.Wait();

		command.Reset();
#pragma endregion
	}

	// ウィンドウクラスを登録解除
	wAPI.MyUnregisterClass();

	return 0;
}