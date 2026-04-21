#pragma once
#include "BaseScene.h"
#include "Camera.h"
#include "CameraManager.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "ModelManager.h"
#include "Object.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "PostEffect.h"
#include "SoundManager.h"
#include "Sprite.h"

#include "../../EnemyBullet.h"

class Player;
class PlayerBullet;

class banana {
public:
    enum class CollisionShape {
        kPlane, // 面
        kBox // 矩形
    };

    struct CollisionVolume {
        CollisionShape shape; // 形状の判定
        Vector3 position; // 面の中心座標
        Vector3 normal; // 面の正面方向（法線）
        Vector3 right; // 面の横方向（幅の計算用）
        Vector3 up; // 面の縦方向（高さの計算用）
        float width; // 横幅の半分
        float height;
        uint32_t partId;
    };

    // 各パーツの情報
    struct BossPart {
        Transform transform; // モデル用座標
        Vector3 collisionLocalPos; // 当たり判定のローカル座標

        bool isAnimating = false;

        // 当たり判定のサイズ
        static inline const float radiusX = 3.0f;
        static inline const float radiusY = 20.0f;

        // 体力
        int PartsHp = 100;
        static inline const int kPartsHp = 100;

        // --- 以下を追加 ---
        float repairTimer = 0.0f; // 修理までの時間
        static inline const float kRepairTime = 300.0f; // 修理にかかる時間（60FPSなら5秒）
        // ------------------

        bool isWeakPoint;
        std::unique_ptr<Object> object;
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
    void Initialize(Camera* camera, Vector3 pos, int health);

    void Update();

    void Draw3D();

    void BulletMirror(const CollisionVolume& volume, PlayerBullet* bullet);

public:
    // Get
    Vector3 GetWorldPosition() const;
    float GetRadius() const;
    bool GetIsDead() const;
    std::vector<CollisionVolume> GetCollisionVolumes();
    const std::vector<std::unique_ptr<EnemyBullet>>& GetBullets() const { return enemyBullet_; }

    // Set
    void SetTargetPlayer(Player* target);
    bool OnCollision(const CollisionVolume& volume, PlayerBullet* bullet);

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

    std::vector<std::unique_ptr<EnemyBullet>> enemyBullet_;
};
