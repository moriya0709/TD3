#include "NormalEnemy.h"
#include "../Bullet/NormalEnemyBullet.h"
#include "NormalEnemy.h"

void NormalEnemy::Initialize(Camera* camera, Vector3 pos, int health)
{
    camera_ = camera;

    transform_.scale = { 1.0f, 1.0f, 1.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    transform_.translate = pos;

    object_ = std::make_unique<Object>();
    object_->Initialize(camera_);
    object_->SetModel("player.obj");
    object_->SetScale(transform_.scale);
    object_->SetRotate(transform_.rotate);
    object_->SetTranslate(transform_.translate);

    health_ = health;
    isAvile = true;

    interval = maxInterval;
}

void NormalEnemy::Update()
{
    if (behaviorRequest_ != Behavior::kUnknown) {
        behavior_ = behaviorRequest_;

        switch (behavior_) {
        case NormalEnemy::Behavior::kDefeated:
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

    // 弾の処理
    BulletUpdate();

    // 生きていないならやられモーション処理を入れる
    if (!isAvile) {
        behaviorRequest_ = Behavior::kDefeated;
    }

    // オブジェクトのセット
    object_->SetTranslate(transform_.translate);
    object_->SetRotate(transform_.rotate);

    // ここにIMGUI

    object_->Update();
}

void NormalEnemy::Draw3D()
{
    // 3Dオブジェクト描画
    object_->Draw();

    // 更新処理
    for (auto& bullet : enemyBullet_) {
        bullet->Draw3D();
    }
}

void NormalEnemy::OnCollision(int Damage, [[maybe_unused]] Vector3 bulletPos)
{
    health_ -= Damage;

    if (health_ <= 0) {
        behaviorRequest_ = Behavior::kDefeated;
        deadTimer_ = kdeadTimer_;
    }
}

void NormalEnemy::SetWayPoints(const std::vector<WayPoint>& waypoints)
{
    wayPoints_ = waypoints;
    currentWayPointIndex_ = 0;
    wayPointTimer_ = 0.0f;

    // 初期座標
    startPos_ = transform_.translate;
}

void NormalEnemy::SetFleeWaypoint(const WayPoint& fleeWP, bool hasFleeData)
{
    fleeWaypoint_ = fleeWP;
    hasFleeData_ = hasFleeData;
}

void NormalEnemy::EnemyMove()
{
    float deltaTime = 1.0f / 60.0f;

    if (isStop_) {
        wayStopTimer_ -= deltaTime;
        if (wayStopTimer_ <= 0.0f) {
            isStop_ = false;

            currentWayPointIndex_++; // 次の地点へ
            wayPointTimer_ = 0.0f; // タイマーリセット
            startPos_ = transform_.translate; // 現在地を次の「出発点」にする
        }
    }

    if (currentWayPointIndex_ < wayPoints_.size()) {

        // タイマーを進める
        wayPointTimer_ += deltaTime;

        // 現在目指しているウェイポイントの情報を取得
        const WayPoint& currentWP = wayPoints_[currentWayPointIndex_];

        float t = wayPointTimer_ / currentWP.timeToReach;

        if (t > 1.0f) {
            t = 1.0f;
        }

        transform_.translate = startPos_ + (currentWP.target - startPos_) * t;

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

void NormalEnemy::BulletUpdate()
{
    // 弾を生成する時間を減らす
    if (behavior_ == Behavior::kWalk) {
        interval -= 1.0f / 60.0f;
    }

    if (interval <= 0.0f) {
        // 弾の生成
        std::unique_ptr<NormalEnemyBullet> newBulletEnemy = std::make_unique<NormalEnemyBullet>();
        newBulletEnemy->Initialize(camera_, transform_.translate);
        newBulletEnemy->SetBulletAcceleration(Vector3(0.0f, 0.0f, -0.1f));

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

void NormalEnemy::BehaviorWalk()
{
    // 移動
    EnemyMove();
}

void NormalEnemy::BehaviorAway()
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
        // JSONに逃走先が書かれていなかった場合のデフォルト動作
        transform_.translate.z += 0.5f;
        transform_.translate.y += 0.2f;

        float cameraZ = camera_->GetTranslate().z;
        if (transform_.translate.z > cameraZ + 200.0f) {
            isDead_ = true;
        }
    }
}

void NormalEnemy::BehaviorDefeated()
{
    transform_.rotate.z += 0.15f;
    deadTimer_ -= 1.0f / 60.0f;

    // 上に断末のコードを角
    if (deadTimer_ <= 0.0f) {
        isDead_ = true;
    }
}
