#pragma once
#include "Camera.h"
#include "Input.h"
#include <string>
#include <vector>

// 録画データの一コマ
struct CameraState {
	float time;
	Vector3 velocity;
	Vector3 angularVelocity;
	Vector3 position;
	Vector3 rotation;
	float fov; // ★ 追加：ズーム（画角）
};

// カメラのトランスフォーム（FOV込み）
struct CameraTransform {
	Vector3 translate;
	Vector3 rotate;
	float fov;
};

class CameraController {
public:
	void Initialize(Camera* camera);
	void Update();
	void DrawImGui();
	void DrawDebugTrace();
	void StartReplay();
	Vector3 GetVelocity() const { return uiVelocity; }

	float GetCurrentReplayTime() const { return timer; }
	int GetCurrentStage() const { return currentStage; }

private:
	// 補間ヘルパー
	Vector3 CameraLerp(const Vector3& start, const Vector3& end, float t);
	Vector3 CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);
	float Lerp(float a, float b, float t);                              // ★ 追加
	float CatmullRomF(float v0, float v1, float v2, float v3, float t); // ★ 追加

	void RecordStateIfChanged(const Vector3& vel, const Vector3& angVel, const Vector3& pos, const Vector3& rat, float fov);
	void ApplyReplayState(Vector3& vel, Vector3& angVel, Vector3& pos, Vector3& rat, float& fov);
	void ApplyPhysics(const Vector3& vel, const Vector3& angVel, const Vector3& pos, const Vector3& rat, float fov);

	void SeekTo(float targetTime);
	void StartOverwriteRecording(); // パンチイン録画

	void SaveToJSON(const std::string& filename);
	void LoadFromJSON(const std::string& filename);
	std::string GetFilePath(int slot) const;

private:
	Camera* camera = nullptr;
	CameraTransform cameraTransform = {
	    {0, 0, 0},
        {0, 0, 0},
        45.0f
    };
	CameraTransform initialTransform = {
	    {0, 0, 0},
        {0, 0, 0},
        45.0f
    };
	// --- 定数設定 ---
	const float kMaxDuration = 180.0f;
	const float kMinFov = 1.0f;   // これ未満だとズームしすぎて破綻する
	const float kMaxFov = 50.0f; // 180度を超えると画面が反転するため制限
	std::vector<CameraState> stateHistory;

	float timer = 0.0f;
	bool isReplaying = false;
	bool isPaused = false;
	bool isRecording = false;
	bool isSmoothMode = true;
	bool showDebugTrace = true;

	int currentStage = 1;
	float playbackSpeed = 1.0f;

	// UI操作用
	Vector3 uiVelocity = {0.0f, 0.0f, 0.0f};
	Vector3 uiAngularVelocity = {0.0f, 0.0f, 0.0f};
	float uiFov = 45.0f; // ★ 追加

	// 変化検知用
	Vector3 lastRecordedVel = {-999.0f, -999.0f, -999.0f};
	Vector3 lastRecordedAngVel = {-999.0f, -999.0f, -999.0f};
	Vector3 lastRecordedPos = {-999.0f, -999.0f, -999.0f};
	Vector3 lastRecordedRot = {-999.0f, -999.0f, -999.0f};
	float lastRecordedFov = -999.0f; // ★ 追加
};