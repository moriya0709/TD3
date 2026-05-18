#include "GrapeCameraController.h"

void GrapeCameraController::Initialize(Camera* camera) {
	pCamera = camera; // カメラを覚えておく
	moveSpeed = 10.0f;
	pCamera->SetRotate({0.0f, 0.0f, 0.0f}); // カメラの初期回転を設定

}

void GrapeCameraController::Update() { 
	Vector3 cmrTranslate = pCamera->GetTranslate();
	cmrTranslate.z += moveSpeed * 1.0f / 60.0f;
	pCamera->SetTranslate(cmrTranslate);
	float deltaTime = 1.0f / 60.0f;
	timer += deltaTime;

}

void GrapeCameraController::ChangeStage(int stage) {
	currentStage = stage;
	switch (currentStage) {
	case 1:
		moveSpeed = 0.02f;
		break;
	case 2:
		moveSpeed = 10.0f;
		break;
	case 3:
		moveSpeed = 0.1f;
		break;
	default:
		moveSpeed = 0.02f;
		break;
	}
}
