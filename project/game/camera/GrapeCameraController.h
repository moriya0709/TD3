#pragma once
#define NOMINMAX
#include "CameraController.h"

class GrapeCameraController : public CameraController {
public:
	void Initialize(Camera* camera) override;
	void Update() override;


	// --- ゲッター ---
	int GetCurrentStage() const override { return currentStage; }
	void SetCurrentStage(int stage) override { ChangeStage(stage); }
	void StartReplay() override {
		timer = 0.0f;
		isPlaying = true;
	}
	void SetTargetPosition(const Vector3& position) override { bananaPosition = position; }
	float GetElapsedTime() const override { return timer; }

private:
	float timer = 0.0f;
	bool isPlaying = false;
	int currentStage = 2;
	void ChangeStage(int stage);

	Vector3 bananaPosition = {0.0f, 0.0f, 0.0f}; // バナナの位置

	// --- ここから新しく追加した変数 ---
	Camera* pCamera = nullptr;   // 操作するカメラを覚えておくための変数
	float radius = 10.0f;        // バナナからカメラまでの距離（半径）
	float moveSpeed = 0.02f; // カメラが回転するスピード
	float cameraHeight = 5.0f;   // バナナよりどれくらい高い位置から見下ろすか
};