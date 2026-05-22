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
        kBox // 矩形
    };

    struct CollisionVolume {
        Vector3 position;
        Vector3 width; // 各軸の半径 (x=幅/2, y=高さ/2, z=厚み/2)
        Vector3 axes[3]; // OBBの方向ベクトル (0:右, 1:上, 2:前)
        CollisionShape shape;
        uint32_t partId;
        Vector3 normal; // 反射用
    };

    // 各パーツの情報
    struct BossPart {
        Transform transform;
        Vector3 collisionLocalPos; // TransformからVector3へ変更（座標のみ保持）
        float baseAngle = 0.0f;
        float collisionRotationY = 0.0f; // 当たり判定の回転専用変数

        bool isAnimating = false;

        // 当たり判定のサイズ
        static inline const float radiusX = 1.0f;
        static inline const float radiusY = 3.0f;

        // 体力
        int PartsHp = 100;
        static inline const int kPartsHp = 200;

        // --- 以下を追加 ---
        float repairTimer = 0.0f; // 修理までの時間
        static inline const float kRepairTime = 20.0f; // 修理にかかる時間
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

    CollisionVolume CreateVolumeFromPart(uint32_t i, Vector3 bossPos);

public:
    // Get
    Vector3 GetWorldPosition() const;
    float GetRadius() const;
    bool GetIsDead() const;
    bool GetIsAlive() const;
    std::vector<CollisionVolume> GetCollisionVolumes();
    const std::vector<std::unique_ptr<EnemyBullet>>& GetBullets() const { return enemyBullet_; }
    int GetScore() const { return score_; }
    Vector3 GetBodyWorldPosition() const;
    int GetDameg() const { return Dameg_; }
    int GetDamegBullet() const { return DamegBullet_; }

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
    void DrawDebugCollision();

     bool showDebugCollision_ = true;
    // パーツ数分のデバッグ表示用オブジェクト
    std::vector<std::unique_ptr<Object>> debugCollisionObjects_;

    // スコア
    int score_ = 2500;
    int DamegBullet_ = 10; // 弾
    int Dameg_ = 15; // 突進


    Behavior behavior_ = Behavior::kStillness;
    Behavior behaviorRequest_ = Behavior::kUnknown;

    Camera* camera_ = nullptr; // カメラ―

    // バナナの皮
    Transform baseTransform_; // ボス本体のベース（カメラ相対座標）
    std::vector<BossPart> parts_; // 4つのパーツを格納するリスト（vector）

    int health_; // 体力

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
    bool isAlive_ = true;
    static inline const float KAliveTimer = 60.0f;
    float AliveTimer = 0.0f;
    float deadTimer_;
    static inline const float kdeadTimer_ = 10.0f;

    std::vector<std::unique_ptr<EnemyBullet>> enemyBullet_;
};
