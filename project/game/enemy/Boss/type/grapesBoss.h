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

class grapesBoss {
public:
    struct CollisionVolume {
        Vector3 position;
        float radius;
        uint32_t partId;
    };

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

        // 突進に関するもの

        bool isDashschedule = false; // 突進予定か
        bool isDashing = false; // 突進中か
        bool isReturning = false; // 帰還中か
        Vector3 velocity; // 個別のベクトル
        float returnTimer = 0.0f; // 帰還アニメーション用
        Vector3 startReturnPos; // 帰還開始地点

        Vector3 targetTranslate; // 突進開始地点
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

    void WeakPointChange();

public:
    // Get
    Vector3 GetWorldPosition() const;
    float GetRadius() const;
    bool GetIsDead() const;
    bool GetIsAlive() const;
    std::vector<CollisionVolume> GetCollisionVolumes();
    const std::vector<std::unique_ptr<EnemyBullet>>& GetBullets() const { return enemyBullet_; }
    int GetScore() const { return score_; }
    int GetDameg() const { return Dameg_; }
    int GetDamegBullet() const { return DamegBullet_; }

    // Set
    void SetTargetPlayer(Player* target);
    bool OnCollision(const CollisionVolume& volume, PlayerBullet* bullet);

public:
    void BulletUpdate();
    void StartRush();

    void MoveUpdate();
    void RoatetUpdate();
    void MoveRush();

    void BehaviorStillness();
    void BehaviorAttack();
    void BehaviorShield();
    void BehaviorDefeated();

private:
    // スコア
    int score_ = 2500;
    int DamegBullet_ = 10; // 弾
    int Dameg_ = 15; // 突進

    Behavior behavior_ = Behavior::kStillness;
    Behavior behaviorRequest_ = Behavior::kUnknown;

    Camera* camera_ = nullptr; // カメラ―
    std::unique_ptr<Object> stemobject; // 茎
    Vector3 baseStem = { 0.0f, 8.0f, 0.0f };

    Transform baseTransform_; // ボス本体のベース（カメラ相対座標）
    Vector3 localPos_; // カメラから見た相対座標を保持する

    std::vector<BossPart> parts_; // 7つのパーツを格納するリスト（vector）

    int currentWeakPointIndex_ = 0; // 現在、何番目が「本体」か
    float WeakPointchangeTimer; // 弱点入れ替え時間
    static inline const float ktWeakPointchangeTimer = 10.0f;

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
    float moveTimer_ = 0.0f; // 浮遊運動用のタイマー

    // 回転シールドフラグ
    bool isShield = false;
    float shieldTimer_ = 0.0f; // シールド状態の残り時間タイマー
    static inline const float kMaxShieldTime = 5.0f; // シールドの継続時間（5秒間）

    float shieldShotInterval_ = 0.0f; // シールド中の弾発射間隔タイマー
    static inline const float kShieldShotMaxInterval = 0.6f; // 何秒ごとに弾を撃つか（0.6秒ごと）

    float shieldAngle_ = 0.0f;

    // 脂肪フラグ
    bool isDead_ = false;
    bool isAlive_ = true;
    static inline const float KAliveTimer = 60.0f;
    float AliveTimer = 0.0f;
    float deadTimer_;
    static inline const float kdeadTimer_ = 3.0f;

    std::vector<std::unique_ptr<EnemyBullet>> enemyBullet_;
};
