#pragma once
#include <directxmath.h>
#include <vector>
#include <wrl.h>
#include <D3d12.h>

#include "Calc.h"
#include "CommonStructs.h"

struct RadarVertex {
    DirectX::XMFLOAT3 Position; // (x, y, z)
    DirectX::XMFLOAT4 Color;    // RGBA
};

class RadarChart {
public:
    // 初期化
	void Initialize();
	// 更新
	void Update();
    // 描画
	void Draw();

    // setter
    void SetValues(float newValues[]) {
        for (int i = 0; i < kVertices; ++i) {
            values[i] = newValues[i];
        }
	}
    void SetColor(const DirectX::XMFLOAT4& newColor) { color = newColor;}
	void SetMaxRadius(float newMaxRadius) { maxRadius = newMaxRadius; }
	void SetPosition(const Vector2& position) { position_ = position; }

private:
	Vector2 position_{ 960.0f, 540.0f }; // レーダーチャートの中心位置

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vbView{};

    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;
    D3D12_INDEX_BUFFER_VIEW ibView{};

    std::vector<RadarVertex> vertices;
    std::vector<uint16_t> indices;

    const int kVertices = 5; // 五角形の場合
    float values[5] = { 0.8f, 0.6f, 0.9f, 0.5f, 0.7f };
    float maxRadius = 100.0f;
    DirectX::XMFLOAT4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; // 半透明の水色

    // 頂点バッファ
    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(ID3D12Device* device, size_t size);

    // GPUへのデータ転送関数
    void TransferToGPU();

};

