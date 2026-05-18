#include "HomingEnemy.h"
#include "../../../../engine/math/Calc.h"
#include "../Bullet/HomingEnemyBullet.h"
#include "../player/Player.h"

void HomingEnemy::Initialize(Camera* camera, Vector3 pos, int health)
{
    camera_ = camera;

    transform_.scale = { 2.0f, 2.0f, 2.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    transform_.translate = pos;
    localPos_ = pos;

    object_ = std::make_unique<Object>();
    object_->Initialize(camera_);
    object_->SetModel("pepa.obj");
    object_->SetScale(transform_.scale);
    object_->SetRotate(transform_.rotate);
    object_->SetTranslate(transform_.translate);

    health_ = health;
    isAvile = true;

    interval = maxInterval;
}

void HomingEnemy::Update()
{
    if (behaviorRequest_ != Behavior::kUnknown) {
        behavior_ = behaviorRequest_;

        switch (behavior_) {
        case HomingEnemy::Behavior::kDefeated:
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

    // 敵に対して向きを合わせる
    Vector3 playerPos = player_->GetPosition();
    Vector3 pToE = playerPos - transform_.translate;

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

    // オブジェクトのセット
    object_->SetTranslate(transform_.translate);

    // ここにIMGUI

    object_->Update();
}

void HomingEnemy::Draw3D()
{
    // 3Dオブジェクト描画
    object_->Draw();

    // 更新処理
    for (auto& bullet : enemyBullet_) {
        bullet->Draw3D();
    }
}

void HomingEnemy::OnCollision(int Damage, [[maybe_unused]] Vector3 bulletPos, [[maybe_unused]] Vector3 Velocity)
{
    if (behavior_ == Behavior::kDefeated) {
        return;
    }

    health_ -= Damage;

    if (health_ <= 0) {
        behaviorRequest_ = Behavior::kDefeated;
        deadTimer_ = kdeadTimer_;
    }
}

void HomingEnemy::SetWayPoints(const std::vector<WayPoint>& waypoints)
{
    wayPoints_ = waypoints;
    currentWayPointIndex_ = 0;
    wayPointTimer_ = 0.0f;

    // 初期座標
    startPos_ = transform_.translate;
}

void HomingEnemy::SetFleeWaypoint(const WayPoint& fleeWP, bool hasFleeData)
{
    fleeWaypoint_ = fleeWP;
    hasFleeData_ = hasFleeData;
}

void HomingEnemy::EnemyMove()
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

        behaviorRequest_ = Behavior::kAway;

        fleeTimer_ = 0.0f;
        fleeStartPos_ = transform_.translate;
    }
}

void HomingEnemy::BulletUpdate()
{
    // 弾を生成する時間を減らす
    if (behavior_ == Behavior::kWalk) {
        interval -= 1.0f / 60.0f;
    }

    if (interval <= 0.0f) {
        // 弾の生成
        std::unique_ptr<HomingEnemyBullet> newBulletEnemy = std::make_unique<HomingEnemyBullet>();
        newBulletEnemy->Initialize(camera_, transform_.translate);
        newBulletEnemy->SetBulletAcceleration(Vector3(0.0f, 0.0f, -0.08f));
        newBulletEnemy->SetTargetPosition(player_->GetPosition());

        enemyBullet_.push_back(std::move(newBulletEnemy));
        interval = maxInterval;
    }
    // 更新処理
    for (auto& bullet : enemyBullet_) {
        bullet->SetTargetPosition(player_->GetPosition());
        bullet->Update();
    }

    // 弾の削除
    std::erase_if(enemyBullet_, [](const std::unique_ptr<EnemyBullet>& bullet) {
        return !bullet->GetIsActive(); // GetIsActive が false なら削除
    });
}
void HomingEnemy::BehaviorWalk()
{
    // 移動
    EnemyMove();
}

void HomingEnemy::BehaviorAway()
{
    if (hasFleeData_) {
        float deltaTime = 1.0f / 60.0f;
        fleeTimer_ += deltaTime;

        float t = fleeTimer_ / fleeWaypoint_.timeToReach;
        if (t > 1.0f)
            t = 1.0f;

        // 逃走先へ向かってLerp
        localPos_ = fleeStartPos_ + (fleeWaypoint_.target - fleeStartPos_) * t;

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

void HomingEnemy::BehaviorDefeated()
{
    transform_.rotate.z += 0.15f;
    deadTimer_ -= 1.0f / 60.0f;

    // 上に断末のコードを角
    if (deadTimer_ <= 0.0f) {
        isDead_ = true;
        isAvile = true;
    }
}
