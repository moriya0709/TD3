#pragma once
#include "../../EnemyBullet.h"

class NormalEnemyBullet : public EnemyBullet {
public:
    void Initialize(Camera* camera, Vector3 Pos) override;

    void Update() override;

    void Draw3D() override;

    // Get
    bool GetIsActive() const override { return isAvile; };
    Vector3 GetWorldPosition() const override { return transform_.translate; };
    float GetRadius() const override { return radius; };

    // Set
    void SetPosition(Vector3 Pos) override { transform_.translate = Pos; }
    void SetBulletAcceleration(Vector3 num) override { acceleration = num; }
    void SetactiveTimer(float num) override { activeTimer = num; }
    void SetTargetPosition(Vector3 Pos) { Pos; }
    void OnCollision() override;

private:
    Transform transform_; // 座標系
    bool isAvile = true; // 生存しているか
    Vector3 acceleration; // 弾の速さ(個別で設定)
    Vector3 velocity_; // ベクトル(速さ)
    static inline const float maxSpeed = 1.00f; // 弾の最高速度(青天井でおk)
    float activeTimer; // 弾の持続時間
    static inline const float maxactiveTimer = 3.0f; // 弾の最大持続時間
    float homingPower = 0.015f; // ダミー用

    // キャラクターの当たり判定サイズ
    static inline const float radius = 2.0f;

    Camera* camera_ = nullptr; // カメラ
    std::unique_ptr<Object> object_; // オブジェ

    /* ウェイポイント移動の変数 */
};
