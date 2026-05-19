#include "NormalEnemyBullet.h"

void NormalEnemyBullet::Initialize(Camera* camera, Vector3 Pos)
{
    camera_ = camera;

    transform_.scale = { 1.0f, 1.0f, 1.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    transform_.translate = Pos;

    object_ = std::make_unique<Object>();
    object_->Initialize(camera_);
    object_->SetModel("anpleBullet.obj");
    object_->SetScale({ 0.5f, 0.5f, 0.5f });
    object_->SetRotate(transform_.rotate);
    object_->SetTranslate(transform_.translate);

    activeTimer = maxactiveTimer;
    acceleration.z = 0.1f;
}

void NormalEnemyBullet::Update()
{

    activeTimer -= 1.0f / 60.0f;
    if (activeTimer <= 0.0f) {
        isAvile = false;
    }

    velocity_ += acceleration;

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

void NormalEnemyBullet::Draw3D()
{
    // 3Dオブジェクト描画
    object_->Draw();
}

void NormalEnemyBullet::OnCollision()
{
    isAvile = false;
}
