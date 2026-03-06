#include "NormalEnemyBullet.h"

void NormalEnemyBullet::Initialize(Camera* camera, Vector3 Pos)
{
    camera_ = camera;

    transform_.scale = { 1.0f, 1.0f, 1.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    transform_.translate = Pos;

    object_ = std::make_unique<Object>();
    object_->Initialize(camera_);
    object_->SetModel("player.obj");
    object_->SetScale(transform_.scale);
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

    vector += acceleration;

    if (vector.z >= maxSpeed) {
        vector.z = maxSpeed;
    }

    transform_.translate += vector;
    object_->SetTranslate(transform_.translate);

    // 更新
    object_->Update();
}

void NormalEnemyBullet::Draw3D()
{
    // 3Dオブジェクト描画
    object_->Draw();
}
