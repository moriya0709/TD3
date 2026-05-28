#pragma once
#include "../../Enemy.h"
#include "../../EnemyBullet.h"
#include <vector>
#include "ParticleEmitter.h"

class NormalEnemy : public Enemy {
public:
    enum class Behavior {
        kUnknown = -1,
        kWalk, // 戦闘状態
        kAway, // 逃走状態
        kDefeated, // ﾀﾋ
    };

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="camera">カメラ</param>
    /// <param name="pos">初期座標</param>
    /// <param name="health">体力</param>
    void Initialize(Camera* camera, Vector3 pos, int health) override;

    /// <summary>
    /// 更新
    /// </summary>
    void Update() override;

    /// <summary>
    /// オブジェクト表示
    /// </summary>
    void Draw3D() override;

    // Set
    void OnCollision(int Damage, [[maybe_unused]] Vector3 bulletPos, [[maybe_unused]] Vector3 Velocity) override;
    void SetWayPoints(const std::vector<WayPoint>& waypoints) override;
    void SetFleeWaypoint(const WayPoint& fleeWP, bool hasFleeData) override;

    // Get
    Vector3 GetWorldPosition() const override { return transform_.translate; }
    float GetRadius() const override { return radius; }
    bool GetIsDead() const override { return isDead_; }
    bool GetIsAlive() const override { return isAvile; }
    int GetScore() const override { return score_; }
    int GetDameg() const override { return Dameg_; }

private:
    void EnemyMove();
    void BulletUpdate();

    void BehaviorWalk();
    void BehaviorAway();
    void BehaviorDefeated();

private:
    // スコア
    int score_ = 100;

    int Dameg_ = 10;

    Behavior behavior_ = Behavior::kWalk;
    Behavior behaviorRequest_ = Behavior::kUnknown;

    std::unique_ptr<Object> object_; // オブジェ
    Camera* camera_ = nullptr; // カメラ―

    Vector3 localPos_; // カメラから見た相対座標を保持する
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
    float deadTimer_;
    static inline const float kdeadTimer_ = 0.1f;

    // キャラクターの当たり判定サイズ
    static inline const float radius = 1.5f;

    /* ウェイポイント移動の変数 */
    std::vector<WayPoint> wayPoints_;
    int currentWayPointIndex_ = 0;
    float wayPointTimer_ = 0.0f;
    float wayStopTimer_ = 0.0f;
    bool isStop_ = false;
    Vector3 startPos_;

    // --- 逃走用の変数 ---
    WayPoint fleeWaypoint_;
    bool hasFleeData_ = false;
    float fleeTimer_ = 0.0f;
    Vector3 fleeStartPos_; // 逃走を開始した瞬間の座標

    // デスエフェクト
    std::unique_ptr <ParticleEmitter> deathEffect[4] = {};
    const int deathEffectCount = 4; // 数


};
