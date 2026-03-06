#pragma once
#include "EnemyBullet.h"

class Player;

class TargetEnemyBullet : public EnemyBullet {
public:
    void Initialize(Camera* camera, Vector3 Pos) override;

    void Update() override;

    void Draw3D() override;

    // Get
    bool GetIsActive() const override { return isAvile; };
    Vector3 GetWorldPosition() const override { return transform_.translate; };
    float GetRadius() const override { return kWidth; };

    // Set
    void SetPosition(Vector3 Pos) override { transform_.translate = Pos; }
    void SetBulletAcceleration(Vector3 num) override { acceleration = num; }
    void SetactiveTimer(float num) override { activeTimer = num; }

private:
    Transform transform_; // 座標系
    bool isAvile = true; // 生存しているか
    Vector3 acceleration; // 弾の速さ(個別で設定)
    Vector3 vector; // ベクトル(速さ)
    static inline const float maxSpeed = 0.15f; // 弾の最高速度(青天井でおk)
    float activeTimer; // 弾の持続時間
    static inline const float maxactiveTimer = 3.0f; // 弾の最大持続時間

    // キャラクターの当たり判定サイズ
    static inline const float kWidth = 1.0f;
    static inline const float kHeight = 1.0f;

    Camera* camera_ = nullptr; // カメラ
    std::unique_ptr<Object> object_; // オブジェ
};
