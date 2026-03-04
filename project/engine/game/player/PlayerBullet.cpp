#include "PlayerBullet.h"
#include "player.h"
#include "ObjectCommon.h"
#include "SceneManager.h"
#include "SpriteCommon.h"



void PlayerBullet::Initialize(const Vector3& position, Camera* camera) {
	transform_.scale = {0.2f, 0.2f, 0.2f};
	transform_.rotate = {0.0f, 0.0f, 0.0f};
	transform_.translate = position;
	object_ = std::make_unique<Object>();
	object_->Initialize(camera);
	object_->SetModel("plane.obj");
	object_->SetScale(transform_.scale);
	object_->SetRotate(transform_.rotate);
	object_->SetTranslate(transform_.translate);
}

void PlayerBullet::Update() {

	object_->SetTranslate(transform_.translate);
	object_->Update();
};
void PlayerBullet::Draw3D() { object_->Draw(); };
void PlayerBullet::Draw2D() {};
