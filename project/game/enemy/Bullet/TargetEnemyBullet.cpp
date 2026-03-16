#include "TargetEnemyBullet.h"

void TargetEnemyBullet::Initialize(Camera* camera, Vector3 Pos)
{
    camera_ = camera;

    transform_.scale = { 1.0f, 1.0f, 1.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    transform_.translate = Pos;

    object_ = std::make_unique<Object>();
    object_->Initialize(camera_);
    object_->SetModel("plane.obj");
    object_->SetScale({ 0.5f, 0.5f, 0.5f });
    object_->SetRotate(transform_.rotate);
    object_->SetTranslate(transform_.translate);

    activeTimer = maxactiveTimer;
    acceleration_.z = 0.1f;
}

void TargetEnemyBullet::SetTargetPosition(Vector3 Pos)
{
    // 狙う場所を設定
    targetPos_ = Pos;

    // ターゲットへのベクトル ＝ 目的地の座標 － 現在の座標
    Vector3 direction;
    direction.x = targetPos_.x - transform_.translate.x;
    direction.y = targetPos_.y - transform_.translate.y;
    direction.z = targetPos_.z - transform_.translate.z;

    // directionを「長さが1のベクトル（正規化ベクトル）」にする
    direction = Normalize(direction);

    velocity_ = { 0.0f, 0.0f, 0.0f };

    // 加速度の「大きさ」を決め、それにターゲットの方向を掛け合わせる
    acceleration_ = direction * accelerationScalar;
}

void TargetEnemyBullet::OnCollision()
{
    isAvile = false;
}

void TargetEnemyBullet::Update()
{

    activeTimer -= 1.0f / 60.0f;
    if (activeTimer <= 0.0f) {
        isAvile = false;
    }

    velocity_ += acceleration_;

    float currentSpeed = sqrtf(velocity_.x * velocity_.x + velocity_.y * velocity_.y + velocity_.z * velocity_.z);

    // 弾の速さが最高速度を超えていたら、最高速度に制限する
    if (currentSpeed >= maxSpeed) {
        // 現在の進行方向（長さ1）を計算し、それに最高速度を掛ける
        Vector3 currentDir = Normalize(velocity_);
        velocity_ = currentDir * maxSpeed;
    }

    transform_.translate += velocity_;
    object_->SetTranslate(transform_.translate);

    // 更新
    object_->Update();
}

void TargetEnemyBullet::Draw3D()
{
    // 3Dオブジェクト描画
    object_->Draw();
}
