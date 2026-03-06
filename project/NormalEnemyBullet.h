#pragma once
#include "EnemyBullet.h"

class NormalEnemyBullet : public EnemyBullet {
public:
    void Initialize(Camera* camera) override;

    void Update() override;

    void Draw3D() override;

    // Get
    bool GetIsActive() const override { return isAvile; };

    // Set
    void SetBulletAcceleration(float num) { acceleration = num; }
    void SetactiveTimer(float num) { activeTimer = num; }

private:
    Transform transform_; // 座標系
    bool isAvile = true; // 生存しているか
    float acceleration; // 弾の速さ(個別で設定)
    float vector; // ベクトル(速さ)
    static inline const float maxSpeed = 0.50f; // 弾の最高速度(青天井でおk)
    float activeTimer; // 弾の持続時間
    static inline const float maxactiveTimer = 5.0f; // 弾の最大持続時間

    Camera* camera_ = nullptr; // カメラ
    std::unique_ptr<Object> object_; // オブジェ
};
