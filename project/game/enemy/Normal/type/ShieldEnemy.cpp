#include "ShieldEnemy.h"
#include "../../../player/Player.h"
#include "../../engine/math/Calc.h"
#include "../Bullet/NormalEnemyBullet.h"
#include "../Bullet/TargetEnemyBullet.h"

void ShieldEnemy::Initialize(Camera* camera, Vector3 pos, int health)
{

    camera_ = camera;

    transform_.scale = { 2.0f, 2.0f, 2.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    transform_.translate = pos;
    localPos_ = pos;

    object_ = std::make_unique<Object>();
    object_->Initialize(camera_);
    object_->SetModel("suican.obj");
    object_->SetScale(transform_.scale);
    object_->SetRotate(transform_.rotate);
    object_->SetTranslate(transform_.translate);

    health_ = health;
    isAvile = true;
    BehaviorchangeTimer = kBehaviorchangeTimer;

    interval = maxInterval;
}

void ShieldEnemy::Update()
{

    if (behaviorRequest_ != Behavior::kUnknown) {
        behavior_ = behaviorRequest_;

        switch (behavior_) {
        case ShieldEnemy::Behavior::kDefeated:
        default:

            break;
        }

        behaviorRequest_ = Behavior::kUnknown;
    }

    switch (behavior_) {
    case Behavior::kWalk:
        BehaviorWalk();
        break;
    case Behavior::kAway:
        BehaviorAway();
        break;
    case Behavior::kShield:
        BehaviorShield();
        break;
    case Behavior::kDefeated:
        BehaviorDefeated();
        break;
    default:
        break;
    }

    // 生きていないならやられモーション処理を入れる
    if (!isAvile) {
        behaviorRequest_ = Behavior::kDefeated;
    }

    // カメラの位置に応じて変換
    const Matrix4x4& camMat = camera_->GetWorldMatrix();

    transform_.translate = TransformCoord(localPos_, camMat);
    BulletUpdate();

    // オブジェクトのセット
    object_->SetTranslate(transform_.translate);
    object_->SetRotate(transform_.rotate);
    object_->Update();

    // ここにIMGUI
}

void ShieldEnemy::Draw3D()
{
    // 3Dオブジェクト描画
    object_->Draw();

    // 更新処理
    for (auto& bullet : enemyBullet_) {
        bullet->Draw3D();
    }
}

void ShieldEnemy::OnCollision(int Damage, [[maybe_unused]] Vector3 bulletPos, [[maybe_unused]] Vector3 Velocity)
{
    if (behavior_ == Behavior::kDefeated) {
        return;
    }

    if (behavior_ == Behavior::kShield) {
        // 反射する
        BulletMirror(bulletPos, Velocity);
        return;
    }

    health_ -= Damage;

    if (health_ <= 0) {
        behaviorRequest_ = Behavior::kDefeated;
        deadTimer_ = kdeadTimer_;
    }
}

void ShieldEnemy::SetWayPoints(const std::vector<WayPoint>& waypoints)
{
    wayPoints_ = waypoints;
    currentWayPointIndex_ = 0;
    wayPointTimer_ = 0.0f;

    // 初期座標
    startPos_ = transform_.translate;
}

void ShieldEnemy::SetFleeWaypoint(const WayPoint& fleeWP, bool hasFleeData)
{
    fleeWaypoint_ = fleeWP;
    hasFleeData_ = hasFleeData;
}

void ShieldEnemy::EnemyMove()
{
    float deltaTime = 1.0f / 60.0f;

    if (isStop_) {
        wayStopTimer_ -= deltaTime;
        if (wayStopTimer_ <= 0.0f) {
            isStop_ = false;

            currentWayPointIndex_++;
            wayPointTimer_ = 0.0f;

            startPos_ = localPos_;
        }
    }

    Vector3 cameraPos = camera_->GetTranslate();

    if (currentWayPointIndex_ < wayPoints_.size()) {
        wayPointTimer_ += deltaTime;
        const WayPoint& currentWP = wayPoints_[currentWayPointIndex_];
        float t = wayPointTimer_ / currentWP.timeToReach;
        if (t > 1.0f)
            t = 1.0f;

        localPos_ = startPos_ + (currentWP.target - startPos_) * t;

        if (t >= 1.0f && !isStop_) {
            wayStopTimer_ = currentWP.timeToStop;
            isStop_ = true;
        }
    }

    if (currentWayPointIndex_ >= static_cast<int>(wayPoints_.size())) {
        // 逃走状態に移行
        behaviorRequest_ = Behavior::kAway;

        fleeTimer_ = 0.0f;
        fleeStartPos_ = transform_.translate; // 現在地を逃走のスタート地点にする
    }
}
void ShieldEnemy::BulletUpdate()
{
    // 弾を生成する時間を減らす
    if (behavior_ == Behavior::kWalk) {
        interval -= 1.0f / 60.0f;
    }

    if (interval <= 0.0f) {
        // 弾の生成
        std::unique_ptr<TargetEnemyBullet> newBulletEnemy = std::make_unique<TargetEnemyBullet>();
        newBulletEnemy->Initialize(camera_, transform_.translate);
        newBulletEnemy->SetBulletAcceleration(Vector3(0.0f, 0.0f, -0.08f));
        newBulletEnemy->SetTargetPosition(player_->GetPosition());

        enemyBullet_.push_back(std::move(newBulletEnemy));
        interval = maxInterval;
    }
    // 更新処理
    for (auto& bullet : enemyBullet_) {
        bullet->Update();
    }

    // 弾の削除
    std::erase_if(enemyBullet_, [](const std::unique_ptr<EnemyBullet>& bullet) {
        return !bullet->GetIsActive(); // GetIsActive が false なら削除
    });
}

void ShieldEnemy::BulletMirror(Vector3 bulletPos, Vector3 Velocity)
{
    Vector3 bulletPos_ = bulletPos;
    Vector3 enemyPos = transform_.translate;

    // ベクトル
    Vector3 normal = bulletPos - enemyPos;

    // 正規化
    normal.x *= 0.1f;
    normal.y *= 0.1f;
    normal.z *= 1.0f;

    normal = Normalize(normal);

    // 弾のベクトルを入手
    Vector3 bulletVelocity = Velocity;

    // 反射ベクトルの計算
    float dot = bulletVelocity.x * normal.x + bulletVelocity.y * normal.y + bulletVelocity.z * normal.z;

    Vector3 reflectVelocity;
    reflectVelocity.x = bulletVelocity.x - 2.0f * dot * normal.x;
    reflectVelocity.y = bulletVelocity.y - 2.0f * dot * normal.y;
    reflectVelocity.z = bulletVelocity.z - 2.0f * dot * normal.z;

    reflectVelocity.z *= 1.3f;

    // 弾を追加
    std::unique_ptr<NormalEnemyBullet> newBulletEnemy = std::make_unique<NormalEnemyBullet>();
    newBulletEnemy->Initialize(camera_, transform_.translate);
    newBulletEnemy->SetBulletAcceleration(reflectVelocity);
    newBulletEnemy->Update();

    enemyBullet_.push_back(std::move(newBulletEnemy));
}

void ShieldEnemy::BehaviorAway()
{
    if (hasFleeData_) {
        float deltaTime = 1.0f / 60.0f;
        fleeTimer_ += deltaTime;

        float t = fleeTimer_ / fleeWaypoint_.timeToReach;
        if (t > 1.0f)
            t = 1.0f;

        // 逃走先へ向かってLerp
        transform_.translate.x = fleeStartPos_.x + (fleeWaypoint_.target.x - fleeStartPos_.x) * t;
        transform_.translate.y = fleeStartPos_.y + (fleeWaypoint_.target.y - fleeStartPos_.y) * t;
        transform_.translate.z = fleeStartPos_.z + (fleeWaypoint_.target.z - fleeStartPos_.z) * t;

        // 逃走地点に完全に到着したら、存在を消去する
        if (t >= 1.0f) {
            isDead_ = true;
        }
    } else {
        // JSONに逃走先が書かれていなかった場合のデフォルト動作（保険）
        localPos_.z += 0.5f;
        localPos_.y += 0.2f;
        if (localPos_.z > 200.0f)
            isDead_ = true;
    }
}

void ShieldEnemy::BehaviorShield()
{
    // 振り向き処理
    leap += 0.05f;
    if (leap >= 1.0f) {
        leap = 1.0f;
    }
    float targetRotate = startRotate.y + (float)std::numbers::pi;
    transform_.rotate.y = Lerp(startRotate.y, targetRotate, leap);

    // 移動
    EnemyMove();

    // 切り替えまで
    BehaviorchangeTimer -= 1.0f / 60.0f;
    if (BehaviorchangeTimer <= 0.0f) {
        behaviorRequest_ = Behavior::kWalk;
        BehaviorchangeTimer = kBehaviorchangeTimer;
        leap = 0.0f;
        startRotate = transform_.rotate;
    }
}

void ShieldEnemy::BehaviorWalk()
{
    // 振り向き処理
    leap += 0.05f;
    if (leap >= 1.0f) {
        leap = 1.0f;

        // 敵に対して向きを合わせる
        Vector3 playerPos = player_->GetPosition();

        Vector3 pToE = playerPos - transform_.translate;
        transform_.rotate.y = std::atan2(pToE.x, pToE.z);

        float heightDifference = std::sqrt(pToE.x * pToE.x + pToE.z * pToE.z);
        transform_.rotate.x = std::atan2(-pToE.y, heightDifference);

        Vector3 cameRat = camera_->GetRotate();

        float xFlip = 1.0f;
        if (std::abs(transform_.rotate.y - (float)std::numbers::pi) < 0.1f) {
            xFlip = -1.0f;
        }

        Vector3 finalRot = {
            cameRat.x * xFlip + transform_.rotate.x,
            cameRat.y + transform_.rotate.y,
            cameRat.z + transform_.rotate.z
        };
        object_->SetRotate(finalRot);
    } else {
        float targetRotate = 0.0f;
        transform_.rotate.y = Lerp(startRotate.y, targetRotate, leap);
    }

    // 移動
    EnemyMove();

    // 切り替えまで
    BehaviorchangeTimer -= 1.0f / 60.0f;
    if (BehaviorchangeTimer <= 0.0f) {
        behaviorRequest_ = Behavior::kShield;
        BehaviorchangeTimer = kBehaviorchangeTimer;
        leap = 0.0f;
        startRotate = transform_.rotate;
    }
}

void ShieldEnemy::BehaviorDefeated()
{
    transform_.rotate.z += 0.15f;
    deadTimer_ -= 1.0f / 60.0f;

    // 上に断末のコードを角
    if (deadTimer_ <= 0.0f) {
        isDead_ = true;
        isAvile = true;
    }
}