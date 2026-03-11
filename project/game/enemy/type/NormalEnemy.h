#pragma once
#include "../Enemy.h"
#include "../EnemyBullet.h"

class NormalEnemy : public Enemy {
public:
    enum class Behavior {
        kUnknown = -1,
        kWalk, // 戦闘状態
        kAway, // 逃走状態
        kDefeated, // ﾀﾋ
    };

    void Initialize(Camera* camera, Vector3 pos, int health) override;

    void Update() override;

    void Draw3D() override;

    // Set
    void OnCollision(int Damage) override;

    // Get
    Vector3 GetWorldPosition() const override { return transform_.translate; }
    float GetRadius() const override { return radius; }
    bool GetIsDead() const override { return isDead_; }

private:
    std::unique_ptr<Object> object_; // オブジェ
    Camera* camera_ = nullptr; // カメラ―

    Transform transform_; // 座標系
    static inline const float kwalkSpeed = 0.02f; // 歩きの速さ
    Vector3 velocity_ = {}; // 速度
    Vector3 acceleration; // 弾の速さ(個別で設定)

    float activeTimer; // 存在する時間
    float isAvile; // 生存しているか
    int health_; // 体力
    float interval; // 弾を発射する間隔
    static inline const float maxInterval = 2.0f; // 間隔

    // フラグ
    bool isDead_ = false;

    // キャラクターの当たり判定サイズ
    static inline const float radius = 1.0f;
};
