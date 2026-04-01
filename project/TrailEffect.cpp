#include "TrailEffect.h"

void TrailEffect::Initialize(const std::string& textureName, const Transform& transform, float width, float maxLifetime) {
    // 渡された引数を使ってメンバ変数を初期化
    m_TextureName = textureName;
    m_Width = width;
    MAX_LIFETIME = maxLifetime;

    // Transformから初期座標をセット（Transform構造体の中身に合わせて .translate 等に変更してください）
    translate_ = transform.translate;
}

void TrailEffect::Update(float deltaTime, const Vector3& currentEmitterPos) {
    // 既存のポイントのAgeを更新し、寿命切れを削除
    for (auto& pt : m_Points) {
        pt.Age += deltaTime;
    }
    while (!m_Points.empty() && m_Points.back().Age > MAX_LIFETIME) {
        m_Points.pop_back();

    }

    // エミッターが一定距離動いていたら新しいポイントを追加
    if (m_Points.empty() || Distance(m_Points.front().Position, currentEmitterPos) > MIN_DISTANCE) {
        m_Points.push_front({ currentEmitterPos, 0.0f });
    }
}

void TrailEffect::GenerateVertices(const Vector3& cameraPos, std::vector<TrailVertex>& outVertices) {
    if (m_Points.size() < 2) return;

    float totalLength = static_cast<float>(m_Points.size()); // UV計算用（距離ベースにするとなお良し）

    for (size_t i = 0; i < m_Points.size(); ++i) {
        const auto& pt = m_Points[i];

        // 進行方向ベクトルの計算
        Vector3 dir;
        if (i < m_Points.size() - 1) {
            dir = Normalize(m_Points[i].Position - m_Points[i + 1].Position);
        } else {
            dir = Normalize(m_Points[i - 1].Position - m_Points[i].Position); // 終端の処理
        }

        // カメラへのベクトル
        Vector3 toCamera = Normalize(cameraPos - pt.Position);

        // 外積で右方向（幅を広げる方向）のベクトルを算出
        Vector3 right = Normalize(Cross(dir, toCamera));

        // Ageに応じたアルファ値や幅の減衰を計算（古いほど細く、透明にするなど）
        float lifeRatio = pt.Age / MAX_LIFETIME;
        float currentWidth = m_Width * (1.0f - lifeRatio);
        Vector4 currentColor = m_Color;
        currentColor.w *= (1.0f - lifeRatio); // アルファフェード

        // 左右の頂点を生成
        TrailVertex vLeft, vRight;
        vLeft.Position = pt.Position + (right * currentWidth * 0.5f);
        vRight.Position = pt.Position - (right * currentWidth * 0.5f);

        // UVのV座標は進行度（0.0 ~ 1.0）、U座標は左右（0.0, 1.0）
        float vCoord = static_cast<float>(i) / (totalLength - 1.0f);
        vLeft.UV = Vector2(0.0f, vCoord);
        vRight.UV = Vector2(1.0f, vCoord);

        vLeft.Color = currentColor;
        vRight.Color = currentColor;

        outVertices.push_back(vLeft);
        outVertices.push_back(vRight);
    }
}
