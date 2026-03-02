#include "Player.h"
#include "ObjectCommon.h"
#include "SpriteCommon.h"
#include "SceneManager.h"

void Player::Initialize(Camera* camera) {
	// カメラの生成
	camera_ = camera;
	transform_.scale = {1.0f, 1.0f, 1.0f};
	transform_.rotate = {0.0f, 0.0f, 0.0f};
	transform_.translate = {0.0f, 0.0f, 0.0f};

	// モデルの生成
	object_ = std::make_unique<Object>();
	object_->Initialize(camera_);
	object_->SetModel("player");
	object_->SetScale(transform_.scale);
	object_->SetRotate(transform_.rotate);
	object_->SetTranslate(transform_.translate);

	statas_.hp = 100;
	statas_.attack = 10;
	statas_.speed = 0.1f;

	


}

void Player::Update() {
	// 入力取得
	auto input = Input::GetInstance();
	// カメラ更新
	camera_->Update();
	// * 3Dオブジェクト* //
	if (input->PushKey(DIK_W)) {
		transform_.translate.y += statas_.speed;
	} else if (input->PushKey(DIK_S)) {
		transform_.translate.y -= statas_.speed;
	}
	if (input->PushKey(DIK_A)) {
		transform_.translate.x -= statas_.speed;
	} else if (input->PushKey(DIK_D)) {
		transform_.translate.x += statas_.speed;
	}
	object_->SetTranslate(transform_.translate);

	object_->Update();
}

void Player::Draw2D() {
}

void Player::Draw3D() {
	// 3Dオブジェクト描画
	object_->Draw();
}

