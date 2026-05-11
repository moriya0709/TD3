#include "GrapeCameraController.h"

void GrapeCameraController::Initialize(Camera* camera) {
	pCamera = camera; // カメラを覚えておく
	moveSpeed = 10.0f;
	pCamera->SetRotate({0.0f, 0.0f, 0.0f}); // カメラの初期回転を設定

}

void GrapeCameraController::Update() { pCamera->SetTranslate(Vector3(pCamera->GetTranslate().x, pCamera->GetTranslate().y, pCamera->GetTranslate().z + moveSpeed)); }

void GrapeCameraController::ChangeStage(int stage) {
	currentStage = stage;
	switch (currentStage) {
	case 1:
		moveSpeed = 0.02f;
		break;
	case 2:
		moveSpeed = 0.05f;
		break;
	case 3:
		moveSpeed = 0.1f;
		break;
	default:
		moveSpeed = 0.02f;
		break;
	}
}
