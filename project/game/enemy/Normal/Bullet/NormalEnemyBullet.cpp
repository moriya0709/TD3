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
    Vector3 camRot = camera_->GetRotate(); // ※関数名は環境に合わせて変更してください
    // 度数法（0〜360度）ならラジアンに変換。元からラジアンなら * 3.14... の行は不要です
    float rotX = camRot.x * (3.141592f / 180.0f);
    float rotY = camRot.y * (3.141592f / 180.0f);

    // 回転角から正面を向くベクトルを計算
    Vector3 forward;
    forward.x = sinf(rotY) * cosf(rotX);
    forward.y = -sinf(rotX);
    forward.z = cosf(rotY) * cosf(rotX);
    forward = Normalize(forward);

    acceleration = forward * 0.1f; // 正面方向へ加速
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
