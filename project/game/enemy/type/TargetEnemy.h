#pragma once
#include "../Enemy.h"
#include "../EnemyBullet.h"

class Player;

class TargetEnemy : public Enemy {
public:
    enum class Behavior {
        kUnknown = -1,
        kWalk, // 戦闘状態
        kAway, // 逃走状態
        kDefeated, // ﾀﾋ
    };

    void Initialize(Camera* camera, Vector3 pos) override;

    void Update() override;

    void Draw3D() override;

    // Set
    void SetTargetPlayer(Player* target) override { player_ = target; }

    // Get
    Vector3 GetWorldPosition() const override { return transform_.translate; }
    float GetRadius() const override { return kHeight; }

    // 弾リストへの読み取り専用アクセスを提供する関数を追加
    const std::vector<std::unique_ptr<EnemyBullet>>& GetBullets() const { return enemyBullet_; }

private:
    std::unique_ptr<Object> object_; // オブジェ
    Camera* camera_ = nullptr; // カメラ―

    Transform transform_; // 座標系
    static inline const float kwalkSpeed = 0.02f; // 歩きの速さ
    Vector3 velocity_ = {}; // 速度
    Vector3 acceleration; // 弾の速さ(個別で設定)

    float activeTimer; // 存在する時間
    float isAvile; // 生存しているか
    float health; // 体力
    float interval; // 弾を発射する間隔
    static inline const float maxInterval = 2.0f; // 間隔

    // フラグ
    bool isDead_ = false;

    // キャラクターの当たり判定サイズ
    static inline const float kWidth = 1.0f;
    static inline const float kHeight = 1.0f;

    // 弾
    std::vector<std::unique_ptr<EnemyBullet>> enemyBullet_;
    // プレイヤーの情報
    Player* player_ = nullptr;
};
