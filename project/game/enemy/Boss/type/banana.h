#pragma once
#include "../BossEnemy.h"

class banana : public BossEnemy {
public:
    // 各パーツの情報
    struct BossPart {
        Transform transform; // ローカル座標（ボスの中心からのオフセット）

        // --- アニメーション用に追加 ---
        Vector3 targetRotate; // 目標の角度
        bool isAnimating = false; // 現在回転中かどうかのフラグ

        // キャラクターの個々の当たり判定サイズ
        static inline const float radius = 3.0f;

        bool isWeakPoint; // 当たり判定の属性（本体かダミーか）
        std::unique_ptr<Object> object; // 描画オブジェクトをパーツが持つ
    };

    enum class Behavior {
        kUnknown = -1,
        kAppearance, // 出現中(無敵にするため)
        kStillness, // 無
        kAttack, // 突進
        kShield, // シールド状態
        kDefeated, // ﾀﾋ
    };

public:
    void Initialize(Camera* camera, Vector3 pos, int health) override;

    void Update() override;

    void Draw3D() override;

    void BulletMirror(const CollisionVolume& volume, PlayerBullet* bullet);

public:
    // Get
    Vector3 GetWorldPosition() const override;
    float GetRadius() const override;
    bool GetIsDead() const override;
    std::vector<CollisionVolume> GetCollisionVolumes() override;

    // Set
    void SetTargetPlayer(Player* target) override;
    bool OnCollision(const CollisionVolume& volume, PlayerBullet* bullet) override;

public:
    void BulletUpdate();

    void MoveUpdate();
    void RoatetUpdate();
    void MoveRush();

    void BehaviorStillness();
    void BehaviorAttack();
    void BehaviorShield();
    void BehaviorDefeated();

private:
    Behavior behavior_ = Behavior::kStillness;
    Behavior behaviorRequest_ = Behavior::kUnknown;

    Camera* camera_ = nullptr; // カメラ―
    std::unique_ptr<Object> stemobject; // 身
    Vector3 baseStem = { 0.0f, 8.0f, 0.0f };

    // バナナの皮
    Transform baseTransform_; // ボス本体のベース（カメラ相対座標）
    std::vector<BossPart> parts_; // 4つのパーツを格納するリスト（vector）

    int health_; // 体力
    float isAvile; // 生存しているか

    // プレイヤーの情報
    Player* player_ = nullptr;

    // 通常の移動速度
    Vector3 baseMove = { 1.0f, 1.0f, 0.0f };

    static inline const float khomingPower = 0.0075f; // デフォルトホーミング性能
    float homingPower = 0.0075f; // ホーミング性能

    // 突進の移動速度
    Vector3 acceleration_;
    Vector3 velocity_ = { 0.0f, 0.0f, 0.0f };
    static inline const float maxSpeed = 0.75f; // 突進の速度

    float dashTimer = 1.0f;
    static inline const float kdashTimer = 1.0f;

    float BehaviorchangeTimer;
    static inline const float kBehaviorchangeTimer = 5.0f;

    float interval; // 弾を発射する間隔
    Vector3 startRotate;
    static inline const float maxInterval = 2.0f; // 間隔
    int intervalCount = 0;

    // 脂肪フラグ
    bool isDead_ = false;
    float deadTimer_;
    static inline const float kdeadTimer_ = 10.0f;
};
