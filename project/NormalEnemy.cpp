#include "NormalEnemy.h"

void NormalEnemy::Initialize(Camera* camera)
{
    camera_ = camera;

    transform_.scale = { 1.0f, 1.0f, 1.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    transform_.translate = { 2.0f, 0.0f, 0.0f };

    object_ = std::make_unique<Object>();
    object_->Initialize(camera_);
    object_->SetModel("player.obj");
    object_->SetScale(transform_.scale);
    object_->SetRotate(transform_.rotate);
    object_->SetTranslate(transform_.translate);
}

void NormalEnemy::Update()
{
    // カメラ更新
    camera_->Update();

    object_->Update();
}

void NormalEnemy::Draw3D()
{
    // 3Dオブジェクト描画
    object_->Draw();
}
