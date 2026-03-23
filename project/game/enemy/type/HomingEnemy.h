#pragma once
#include "../Enemy.h"
#include "../EnemyBullet.h"

class Player;

class HomingEnemy : public Enemy {
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
    void SetTargetPlayer(Player* target) override { player_ = target; }
    void OnCollision(int Damage) override;
    void SetWayPoints(const std::vector<WayPoint>& waypoints) override;
    void SetFleeWaypoint(const WayPoint& fleeWP, bool hasFleeData) override;

    // Get
    Vector3 GetWorldPosition() const override { return transform_.translate; }
    float GetRadius() const override { return radius; }
    bool GetIsDead() const override { return isDead_; }

private:
    void EnemyMove();
    void BulletUpdate();

    void BehaviorWalk();
    void BehaviorAway();
    void BehaviorDefeated();

private:
    Behavior behavior_ = Behavior::kWalk;
    Behavior behaviorRequest_ = Behavior::kUnknown;

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
    float deadTimer_;
    static inline const float kdeadTimer_ = 0.1f;

    // キャラクターの当たり判定サイズ
    static inline const float radius = 5.0f;

    // プレイヤーの情報
    Player* player_ = nullptr;

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
};
