#pragma once
#define NOMINMAX

#include "Calc.h" // VectorTransform, RaySphereIntersect, Inverse, MakeTranslateMatrix, Normalize など
#include "Camera.h"
#include "Object.h"
#include <memory>
#include <string>
#include <vector>

class DirectXCommon;
class WindowAPI;

// 制御点の構造体
struct ControlPoint {
	Vector3 position;
};
struct RotationSpeedKey {
	float time;            // 変化した瞬間の時間(timer)
	Vector3 rotationSpeed; // その時の回転速度
	float transitionTime;  // その速度に達するまでの時間（秒）
};
class CameraController {
public:
	virtual void Initialize(Camera* camera);
	virtual void Update();
	virtual void EditorUpdate();
	virtual void EditorDraw();

	// --- ゲッター ---
	virtual float GetElapsedTime() const { return timer; }
	virtual float GetTotalDuration() const { return totalDuration; }
	virtual float GetSpeed() const { return currentSpeed; }
	virtual int GetCurrentStage() const { return currentStage; }

	virtual void SetCurrentStage(int stage) {}
	virtual void StartReplay() {
		timer = 0.0f;
		isPlaying = true;
	}
	virtual void SetTargetPosition(const Vector3& position) { lastPosition = position; }

private:



	Camera* camera = nullptr;

	// --- ステージ依存データ ---
	std::vector<ControlPoint> points;
	float totalDuration = 5.0f;
	struct {
		Vector3 rotate = {0, 0, 0};
		float fov = 45.0f;
	} stageStatus;

	// --- 実行時データ ---
	float timer = 0.0f;
	float currentSpeed = 0.0f;
	Vector3 lastPosition;
	bool isPlaying = false;
	int selectedPoint = -1;
	int currentStage = 1;

	const float kMinFov = 1.0f;
	const float kMaxFov = 170.0f;

	// --- 描画・エディタ操作用オブジェクト ---
	std::vector<std::unique_ptr<Object>> spheres;
	std::vector<std::unique_ptr<Object>> railModels;

	DirectXCommon* dxCommon_ = nullptr;
	std::unique_ptr<WindowAPI> windowAPI_ = nullptr;
	Vector3 currentRotationSpeed = {1.0f, 1.0f, 1.0f}; // 現在の回転倍率
	std::vector<RotationSpeedKey> rotationHistory;     // 履歴
	bool speedChangedDuringPause = false;              // 一時停止中に変更されたか
};