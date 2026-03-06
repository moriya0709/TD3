#pragma once
#include "Camera.h"
#include "Input.h" // ★ キー入力判定のために追加
#include <string>
#include <vector>

// カメラの状態（速度）を記録する構造体
struct CameraState {
	float time;              // 変化が起きた時間
	Vector3 velocity;        // 移動速度 (x, y, z)
	Vector3 angularVelocity; // 回転速度 (rx, ry, rz)
};

class CameraController {
public:
	void Initialize(Camera* camera);
	void Update();
	void DrawImGui();

private:
	// 内部ロジック
	void RecordStateIfChanged(const Vector3& vel, const Vector3& angVel);
	void ApplyReplayState(Vector3& vel, Vector3& angVel);
	void ApplyPhysics(const Vector3& vel, const Vector3& angVel);
	void StartReplay();
	void SeekTo(float targetTime);
	void UpdateOrInsertKeyframe(float time, const Vector3& vel, const Vector3& angVel);

	// ファイル入出力
	void SaveToJSON(const std::string& filename);
	void LoadFromJSON(const std::string& filename);
	std::string GetFilePath(int slot) const;

private:
	Camera* camera = nullptr;

	Transform cameraTransform = {
	    {0, 0, 0},
        {0, 0, 0}
    };
	Transform initialTransform = {
	    {0, 0, 0},
        {0, 0, 0}
    };

	std::vector<CameraState> stateHistory;

	float timer = 0.0f;
	bool isReplaying = false;
	bool isPaused = false;
	size_t replayIndex = 0;
	int currentSlot = 1;

	Vector3 activeVelocity = {0, 0, 0};
	Vector3 activeAngularVelocity = {0, 0, 0};

	Vector3 uiVelocity = {0.0f, 0.0f, 0.0f};
	Vector3 uiAngularVelocity = {0.0f, 0.0f, 0.0f};

	Vector3 lastRecordedVel = {-1.0f, -1.0f, -1.0f};
	Vector3 lastRecordedAngVel = {-1.0f, -1.0f, -1.0f};
};