#include "MyClass.h"
#include "Buffer.h"
#include "Input.h"

using namespace DirectX;

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
	ID3D12Debug* debugController;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer();
	}
#endif
	ID3D12Device* device = nullptr;

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
	device = directX.CreateDevice(levels, _countof(levels), device);

	Command command(device);
	// コマンドアロケータを生成
	command.CreateCommandAllocator();

	// コマンドリストを生成
	command.CreateCommandList();

	//コマンドキューを生成
	command.CreateCommandQueue();

	// スワップチェーンの設定
	SwapChain swapChain(device);
	swapChain.Create(directX.dxgiFactory, command.queue, wAPI.hwnd);
	swapChain.CreateDescriptorHeap();
	swapChain.CreateRenderTargetView();
	// フェンスの生成
	Fence fence{};
	fence.CreateFence(device);

	// DirectInputの初期化&キーボードデバイスの生成
	Keyboard keyboard;
	keyboard.GetInstance(wAPI.w);
	keyboard.SetDataStdFormat(); // 入力データ形式を標準設定でセット
	keyboard.SetCooperativeLevel(wAPI.hwnd); // 排他制御レベルのセット
#pragma endregion
#pragma region 描画初期化処理
#pragma region 定数バッファ
	ConstBuf cb[2] = { ConstBuf::Type::Material,ConstBuf::Type::Transform };
	for (size_t i = 0; i < _countof(cb); i++)
	{
		cb[i].SetResource(cb[i].size, 1, D3D12_RESOURCE_DIMENSION_BUFFER);
		cb[i].SetHeapProp(D3D12_HEAP_TYPE_UPLOAD); // ヒープ設定
		cb[i].CreateBuffer(device);
		cb[i].Mapping(); // 定数バッファのマッピング
	}

	cb[ConstBuf::Type::Transform].mapTransform->mat = XMMatrixOrthographicOffCenterLH(
		0, WIN_SIZE.width, WIN_SIZE.height, 0, 0, 1.0f);

	// ビュー変換行列
	XMMATRIX matView;
	XMFLOAT3 eye(0, 100, -100), target(0, 0, 0), up(0, 1, 0);
	matView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));

	// 射影変換行列 
	XMMATRIX matProjection = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(45.0f), (float)WIN_SIZE.width / WIN_SIZE.height, 0.1f, 1000.0f);

	// 行列の合成
	cb[ConstBuf::Type::Transform].mapTransform->mat = matView * matProjection;

	// 値を書き込むと自動的に転送される
	cb[ConstBuf::Type::Material].mapMaterial->color = XMFLOAT4(1, 1, 1, 1);
#pragma endregion
#pragma region 頂点バッファ
	// 頂点データ
	VertexBuf::Vertex vertices[] =
	{
		{{ -50.0f,-50.0f,0.0f },{0.0f,1.0f}}, // 左下
		{{ -50.0f, 50.0f,0.0f },{0.0f,0.0f}}, // 左上
		{{  50.0f,-50.0f,0.0f },{1.0f,1.0f}}, // 右下
		{{  50.0f, 50.0f,0.0f },{1.0f,0.0f}}, // 右上
	};

	VertexBuf vertex(static_cast<UINT>(sizeof(vertices[0]) * _countof(vertices)));
	vertex.SetResource(vertex.size, 1, D3D12_RESOURCE_DIMENSION_BUFFER);
	vertex.SetHeapProp(D3D12_HEAP_TYPE_UPLOAD);
	vertex.CreateBuffer(device);
	vertex.Mapping(vertices, _countof(vertices));
	vertex.CreateView(); // 頂点バッファビューの作成
#pragma endregion
#pragma region インデックスバッファ
	// インデックスデータ
	uint16_t indices[] =
	{
		0,1,2,
		1,2,3,
	};

	IndexBuf index(static_cast<UINT>(sizeof(uint16_t) * _countof(indices)));
	index.SetResource(index.size, 1, D3D12_RESOURCE_DIMENSION_BUFFER);
	index.SetHeapProp(D3D12_HEAP_TYPE_UPLOAD);
	index.CreateBuffer(device);
	index.Mapping(indices, _countof(indices));
	index.CreateView(); // インデックスビューの作成
#pragma endregion
#pragma region テクスチャバッファ
	TextureBuf texture{};
	texture.SetResource();
	texture.SetHeapProp(D3D12_HEAP_TYPE_CUSTOM, D3D12_CPU_PAGE_PROPERTY_WRITE_BACK, D3D12_MEMORY_POOL_L0);
	texture.CreateBuffer(device);
	texture.Transfer();

	ShaderResourceView srv{};
	srv.SetHeapDesc();
	srv.CreateDescriptorHeap(device);
	srv.GetDescriptorHandleForHeapStart(ShaderResourceView::Type::CPU);

	texture.CreateView();
	device->CreateShaderResourceView(texture.buff, &texture.view, srv.handle);
#pragma endregion
#pragma region シェーダ
	ID3DBlob* errorBlob = nullptr; // エラーオブジェクト
	ShaderBlob vs = { L"BasicVS.hlsl", "vs_5_0", errorBlob }; // 頂点シェーダの読み込みとコンパイル
	ShaderBlob ps = { L"BasicPS.hlsl", "ps_5_0", errorBlob }; // ピクセルシェーダの読み込みとコンパイル

	// 頂点レイアウト
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{	// xyz座標
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
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
	// グラフィックスパイプライン設定
	Pipeline pipeline{};

	pipeline.SetShader(vs, ps); // シェーダーの設定
	pipeline.SetSampleMask(); // サンプルマスクの設定
	pipeline.SetRasterizer(); // ラスタライザの設定
	pipeline.SetInputLayout(inputLayout, _countof(inputLayout)); // 頂点レイアウトの設定
	pipeline.SetPrimitiveTopology(); // 図形の形状設定
	pipeline.SetOthers(); // その他の設定

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
	rootSignature.SerializeRootSignature(device, errorBlob);	// ルートシグネチャのシリアライズ
	// パイプラインにルートシグネチャをセット
	pipeline.desc.pRootSignature = rootSignature.rs;

	// パイプランステートの生成
	pipeline.CreatePipelineState(device);
#pragma endregion
#pragma endregion
#pragma region ゲームループで使う変数の定義
	float angle = 0.0f;
	ResourceBarrier barrier{};
	FLOAT clearColor[] = { 0.1f,0.25f,0.5f,0.0f }; // 青っぽい色
	D3D12_VIEWPORT viewport{};
	D3D12_RECT scissorRect{};
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
		keyboard.GetDeviceState();

		if (keyboard.isInput(DIK_D) || keyboard.isInput(DIK_A))
		{
			angle += (keyboard.isInput(DIK_D) - keyboard.isInput(DIK_A)) * XMConvertToRadians(1.0f);

			eye.x = -100 * sinf(angle);
			eye.z = -100 * cosf(angle);

			matView = XMMatrixLookAtLH(XMLoadFloat3(&eye), XMLoadFloat3(&target), XMLoadFloat3(&up));
			cb[ConstBuf::Type::Transform].mapTransform->mat = matView * matProjection;
		}
#pragma endregion
		// 1.リソースバリアで書き込み可能に変更
		barrier.desc.Transition.pResource = swapChain.GetBackBuffersPtr(); // バックバッファを指定
		barrier.SetState(command.list);

		// 2.描画先の変更
		swapChain.GetHandle();
		command.list->OMSetRenderTargets(1, &swapChain.rtvHandle, false, nullptr);

		// 3.画面クリアRGBA
		command.list->ClearRenderTargetView(swapChain.rtvHandle, clearColor, 0, nullptr);
#pragma region 描画コマンド
		// ビューポート設定コマンド
		viewport.Width = WIN_SIZE.width;
		viewport.Height = WIN_SIZE.height;
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
		command.list->SetPipelineState(pipeline.state);
		command.list->SetGraphicsRootSignature(rootSignature.rs);
		command.list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // プリミティブ形状の設定コマンド
		command.list->IASetVertexBuffers(0, 1, &vertex.view); // 頂点バッファビューの設定コマンド
		command.list->IASetIndexBuffer(&index.view); // 頂点バッファビューの設定コマンド
		command.list->RSSetViewports(1, &viewport); // ビューポート設定コマンドを、コマンドリストに積む
		// 定数バッファビューの設定コマンド
		command.list->SetGraphicsRootConstantBufferView(0, cb[ConstBuf::Type::Material].buff->GetGPUVirtualAddress());
		command.list->SetGraphicsRootConstantBufferView(2, cb[ConstBuf::Type::Transform].buff->GetGPUVirtualAddress());
		command.list->SetDescriptorHeaps(1, &srv.heap);
		srv.GetDescriptorHandleForHeapStart(ShaderResourceView::Type::GPU);
		command.list->SetGraphicsRootDescriptorTable(1, srv.gpuHandle);

		// 描画コマンド
		command.list->DrawIndexedInstanced(_countof(indices), 1, 0, 0, 0); // 全ての頂点を使って描画
#pragma endregion
#pragma endregion
#pragma region 画面入れ替え
		// 5.リソースバリアを戻す
		barrier.SetState(command.list);
		// 命令のクローズ
		assert(SUCCEEDED(command.list->Close()));
		// コマンドリストの実行
		command.ExecuteCommandLists();
		// 画面に表示するバッファをフリップ(裏表の入替え)
		swapChain.Flip();

		// コマンドの実行完了を待つ
		command.queue->Signal(fence.f, ++fence.val);
		fence.Wait();

		command.Reset();
#pragma endregion
	}

	// ウィンドウクラスを登録解除
	wAPI.MyUnregisterClass();

	return 0;
}