#pragma once
#include "../BossEnemy.h"

class grapesBoss : public BossEnemy {
public:
    // 各パーツの情報
    struct BossPart {
        Transform transform; // ローカル座標（ボスの中心からのオフセット）
        bool isWeakPoint; // 当たり判定の属性（本体かダミーか）
        std::unique_ptr<Object> object; // 描画オブジェクトをパーツが持つ
    };

    enum class Behavior {
        kUnknown = -1,
        kAppearance, // 出現中(無敵にするため)
        kStillness, // 無
        kAttack, // 攻撃
        kShield, // シールド状態
        kDefeated, // ﾀﾋ
    };

public:
    void Initialize(Camera* camera, Vector3 pos, int health) override;

    void Update() override;

    void Draw3D() override;

    void BulletMirror(Vector3 bulletPos, Vector3 Velocity);

public:
    // Get
    Vector3 GetWorldPosition() const override;
    float GetRadius() const override;
    bool GetIsDead() const override;
    bool OnHit(const CollisionVolume& volume, PlayerBullet* bullet) override;
    std::vector<CollisionVolume> GetCollisionVolumes() override;

    // Set
    void SetTargetPlayer(Player* target) override;
    void OnCollision(int Damage, [[maybe_unused]] Vector3 bulletPos, [[maybe_unused]] Vector3 Velocity) override;

public:
    void BulletUpdate();

    void BehaviorStillness();
    void BehaviorAttack();
    void BehaviorShield();
    void BehaviorDefeated();

private:
    Behavior behavior_ = Behavior::kAppearance;
    Behavior behaviorRequest_ = Behavior::kUnknown;

    Camera* camera_ = nullptr; // カメラ―

    Transform baseTransform_; // ボス本体のベース（カメラ相対座標）
    std::vector<BossPart> parts_; // 7つのパーツを格納するリスト（vector）

    int currentWeakPointIndex_ = 0; // 現在、何番目が「本体」か

    int health_; // 体力
    float isAvile; // 生存しているか

    // プレイヤーの情報
    Player* player_ = nullptr;

    float BehaviorchangeTimer;
    static inline const float kBehaviorchangeTimer = 5.0f;

    float interval; // 弾を発射する間隔
    Vector3 startRotate;
    static inline const float maxInterval = 2.0f; // 間隔

    // 脂肪フラグ
    bool isDead_ = false;
    float deadTimer_;
    static inline const float kdeadTimer_ = 10.0f;

    // キャラクターの個々の当たり判定サイズ
    static inline const float radius = 1.0f;
};
