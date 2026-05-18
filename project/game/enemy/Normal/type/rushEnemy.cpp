#include "rushEnemy.h"
#include "../../../../engine/math/Calc.h"
#include "../Bullet/rushEnemyBullet.h"
#include "Player.h"

void rushEnemy::Initialize(Camera* camera, Vector3 pos, int health)
{
    camera_ = camera;

    transform_.scale = { 2.0f, 2.0f, 2.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    transform_.translate = pos;
    localPos_ = pos;

    object_ = std::make_unique<Object>();
    object_->Initialize(camera_);
    object_->SetModel("itugo.obj");
    object_->SetScale(transform_.scale);
    object_->SetRotate(transform_.rotate);
    object_->SetTranslate(transform_.translate);

    velocity_ = { 0.0f, 0.0f, 0.0f };
    acceleration_ = { 0.0f, 0.0f, 0.0f };

    health_ = health;
    isAvile = true;

    interval = maxInterval;
}

void rushEnemy::Update()
{
    if (behaviorRequest_ != Behavior::kUnknown) {
        behavior_ = behaviorRequest_;

        switch (behavior_) {
        case rushEnemy::Behavior::kDefeated:
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

    BulletUpdate();

    // 生きていないならやられモーション処理を入れる
    if (!isAvile) {
        behaviorRequest_ = Behavior::kDefeated;
    }

    // カメラの位置に応じて変換
    const Matrix4x4& camMat = camera_->GetWorldMatrix();

    transform_.translate = TransformCoord(localPos_, camMat);

    BulletUpdate();

    // 敵に対して向きを合わせる
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

void rushEnemy::Draw3D()
{
    // 3Dオブジェクト描画
    object_->Draw();

    // 更新処理
    for (auto& bullet : enemyBullet_) {
        bullet->Draw3D();
    }
}

void rushEnemy::OnCollision(int Damage, [[maybe_unused]] Vector3 bulletPos, [[maybe_unused]] Vector3 Velocity)
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

void rushEnemy::SetWayPoints(const std::vector<WayPoint>& waypoints)
{
    wayPoints_ = waypoints;
    currentWayPointIndex_ = 0;
    wayPointTimer_ = 0.0f;

    // 初期座標
    startPos_ = transform_.translate;
}

void rushEnemy::SetFleeWaypoint(const WayPoint& fleeWP, bool hasFleeData)
{
    fleeWaypoint_ = fleeWP;
    hasFleeData_ = hasFleeData;
}

void rushEnemy::EnemyMove()
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
    }
}

void rushEnemy::BehaviorWalk()
{
    // 移動
    EnemyMove();
}

void rushEnemy::BulletUpdate()
{
    // 弾を生成する時間を減らす
    if (behavior_ == Behavior::kWalk) {
        interval -= 1.0f / 60.0f;
    }

    if (interval <= 0.0f) {
        // 弾の生成
        std::unique_ptr<rushEnemyBullet> newBulletEnemy = std::make_unique<rushEnemyBullet>();
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

void rushEnemy::BehaviorAway()
{
    // awayだけど突進状態

    CheckCameraCulling();

    targetPos_ = player_->GetPosition();

    // ターゲットへのベクトル ＝ 目的地の座標 － 現在の座標
    Vector3 direction;
    direction.x = targetPos_.x - transform_.translate.x;
    direction.y = targetPos_.y - transform_.translate.y;
    direction.z = targetPos_.z - transform_.translate.z;

    // directionを「長さが1のベクトル（正規化ベクトル）」にする
    direction = Normalize(direction);

    // 加速度をターゲット方向に向ける
    acceleration_ = direction * homingPower;

    velocity_ += acceleration_;

    float currentSpeed = sqrtf(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);
    // 弾の速さが最高速度を超えていたら、最高速度に制限する
    if (currentSpeed >= maxSpeed) {
        // 現在の進行方向（長さ1）を計算し、それに最高速度を掛ける
        Vector3 currentDir = Normalize(velocity_);
        velocity_ = currentDir * maxSpeed;
    }

    Vector3 rotate;
    rotate.y = atan2(velocity_.x, velocity_.z);
    // 横軸方向の長さを求める
    float hypotXZ = std::hypot(velocity_.x, velocity_.z);
    rotate.x = atan2(-velocity_.y, hypotXZ);
    rotate.z = 0.0f;
    object_->SetRotate(rotate);

    transform_.translate += velocity_;
    object_->SetTranslate(transform_.translate);

    // 更新
    object_->Update();
}

void rushEnemy::CheckCameraCulling()
{
    Vector3 cameraPos = camera_->GetTranslate();

    const Matrix4x4& worldMat = camera_->GetWorldMatrix();

    Vector3 cameraForward = { worldMat.m[2][0], worldMat.m[2][1], worldMat.m[2][2] };

    // 正規化
    cameraForward = Normalize(cameraForward);

    // べ黒る
    Vector3 toBullet;
    toBullet.x = transform_.translate.x - cameraPos.x;
    toBullet.y = transform_.translate.y - cameraPos.y;
    toBullet.z = transform_.translate.z - cameraPos.z;

    // 内積
    float dotProduct = cameraForward.x * toBullet.x + cameraForward.y * toBullet.y + cameraForward.z * toBullet.z;

    // カメラから離れているなら
    if (dotProduct < -1.0f) {
        isAvile = false;
    }
}

void rushEnemy::BehaviorDefeated()
{
    transform_.rotate.z += 0.15f;
    deadTimer_ -= 1.0f / 60.0f;

    // 上に断末のコードを角
    if (deadTimer_ <= 0.0f) {
        isDead_ = true;
        isAvile = true;
    }
}