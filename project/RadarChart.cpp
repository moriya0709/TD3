#include "RadarChart.h"
#include "DirectXCommon.h"

void RadarChart::Initialize() {
	// 中心座標をローカル変数にまとめておくとスッキリします
	float centerX = position_.x;
	float centerY = position_.y;

	// 0: 中心点
	vertices.push_back({ {centerX, centerY, 0.0f}, color });

	// 1~n: 各項目の頂点
	float screenWidth = 1920.0f;
	float screenHeight = 1080.0f;

	for (int i = 0; i < kVertices; ++i) {
		float r = maxRadius * values[i];
		float theta = (2.0f * DirectX::XM_PI * i / kVertices) - (DirectX::XM_PI / 2.0f);

		// ピクセル座標を NDC 座標に変換
		float x = ((centerX + r * cosf(theta)) / screenWidth) * 2.0f - 1.0f;
		float y = -(((centerY + r * sinf(theta)) / screenHeight) * 2.0f - 1.0f);

		vertices.push_back({ {x, y, 0.0f}, color });
	}

	indices.clear();
	for (int i = 1; i <= kVertices; ++i) {
		indices.push_back(0);                     // 中心
		indices.push_back(i);                     // 現在の頂点
		indices.push_back((i % kVertices) + 1);    // 次の頂点
	}

	auto device = DirectXCommon::GetInstance()->GetDevice();

	// 1. 頂点バッファの作成 (Upload Heap)
	size_t sizeVB = sizeof(RadarVertex) * vertices.size();

	// 自作エンジンのバッファ作成関数があればそれを使う
	// 無ければ device->CreateCommittedResource で作成
	vertexBuffer = CreateBuffer(device, sizeVB); // ヘルパー関数を想定

	// 2. データの転送
	void* pData = nullptr;
	vertexBuffer->Map(0, nullptr, &pData);
	memcpy(pData, vertices.data(), sizeVB);
	vertexBuffer->Unmap(0, nullptr);

	// 3. Viewの設定
	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
	vbView.SizeInBytes = static_cast<UINT>(sizeVB);
	vbView.StrideInBytes = sizeof(RadarVertex);

	// --- インデックスバッファも同様に作成 ---
	size_t sizeIB = sizeof(uint16_t) * indices.size();
	indexBuffer = CreateBuffer(device, sizeIB);
	indexBuffer->Map(0, nullptr, &pData);
	memcpy(pData, indices.data(), sizeIB);
	indexBuffer->Unmap(0, nullptr);

	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = static_cast<UINT>(sizeIB);


}

void RadarChart::Update() {
	// 1. 頂点配列をクリアして、毎フレーム 0 から作り直す
	vertices.clear();

	float centerX = position_.x;
	float centerY = position_.y;
	float screenWidth = 1920.0f;
	float screenHeight = 1080.0f;

	// 0: 中心点
	float centerNdcX = (centerX / screenWidth) * 2.0f - 1.0f;
	float centerNdcY = -((centerY / screenHeight) * 2.0f - 1.0f);

	vertices.push_back({ {centerNdcX, centerNdcY, 0.0f},color});

	// 1~n: 各項目の頂点
	for (int i = 0; i < kVertices; ++i) {
		float r = maxRadius * values[i]; // values[i] を直接使う
		float theta = (2.0f * DirectX::XM_PI * i / kVertices) - (DirectX::XM_PI / 2.0f);

		float x = ((centerX + r * cosf(theta)) / screenWidth) * 2.0f - 1.0f;
		float y = -(((centerY + r * sinf(theta)) / screenHeight) * 2.0f - 1.0f);

		vertices.push_back({ {x, y, 0.0f}, color });
	}

	// 2. 新しく計算した頂点データを GPU バッファに書き込む
	TransferToGPU();
}

void RadarChart::Draw() {
	// 2. プリミティブトポロジの設定
	DirectXCommon::GetInstance()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 3. バッファのセット
	DirectXCommon::GetInstance()->GetCommandList()->IASetVertexBuffers(0, 1, &vbView);
	DirectXCommon::GetInstance()->GetCommandList()->IASetIndexBuffer(&ibView);
	// 4. 描画実行
	DirectXCommon::GetInstance()->GetCommandList()->DrawIndexedInstanced(static_cast<UINT>(indices.size()), 1, 0, 0, 0);
}

Microsoft::WRL::ComPtr<ID3D12Resource> RadarChart::CreateBuffer(ID3D12Device* device, size_t size) {
	Microsoft::WRL::ComPtr<ID3D12Resource> resource;

	// 1. ヒーププロパティの設定（今回はCPUから書き込めるUpload Heap）
	D3D12_HEAP_PROPERTIES heapProps{};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD; // CPUから書き込み可能

	// 2. リソースの設定（単純なバッファとして設定）
	D3D12_RESOURCE_DESC resDesc{};
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Width = size; // 必要なバイト数
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.SampleDesc.Count = 1;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 3. リソースの生成
	HRESULT hr = device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, // Upload Heapはこの状態が基本
		nullptr,
		IID_PPV_ARGS(&resource)
	);

	if (FAILED(hr)) return nullptr;
	return resource;
}

void RadarChart::TransferToGPU() {
	void* pData = nullptr;
	// バッファを CPU からアクセス可能にする
	HRESULT hr = vertexBuffer->Map(0, nullptr, &pData);
	if (SUCCEEDED(hr)) {
		// std::vector の中身を丸ごとコピー
		memcpy(pData, vertices.data(), sizeof(RadarVertex) * vertices.size());
		// 書き込み終了
		vertexBuffer->Unmap(0, nullptr);
	}
}
