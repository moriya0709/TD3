#pragma once
#include "Camera.h"
#include "Input.h"
#include <string>
#include <vector>

struct CameraState {
	float time;
	Vector3 velocity;
	Vector3 angularVelocity;
	Vector3 position;
	Vector3 rotation;
};

class CameraController {
public:
	void Initialize(Camera* camera);
	void Update();
	void DrawImGui();
	void DrawDebugTrace();

	float GetCurrentReplayTime() const { return timer; }
	int GetCurrentStage() const { return currentStage; }
	Camera* GetCamera() const { return camera; }

private:
	Vector3 CameraLerp(const Vector3& start, const Vector3& end, float t);
	Vector3 CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);

	void RecordStateIfChanged(const Vector3& vel, const Vector3& angVel, const Vector3& pos, const Vector3& rat);
	void ApplyReplayState(Vector3& vel, Vector3& angVel, Vector3& pos, Vector3& rat);
	void ApplyPhysics(const Vector3& vel, const Vector3& angVel, const Vector3& pos, const Vector3& rat);
	void StartReplay();
	void SeekTo(float targetTime);

	// ★ 追加：パンチイン（途中上書き）録画開始
	void StartOverwriteRecording();

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
	bool isRecording = false;
	bool isSmoothMode = true;
	bool showDebugTrace = true;

	int currentStage = 1;
	float playbackSpeed = 1.0f;

	Vector3 uiVelocity = {0.0f, 0.0f, 0.0f};
	Vector3 uiAngularVelocity = {0.0f, 0.0f, 0.0f};

	Vector3 lastRecordedVel = {-999.0f, -999.0f, -999.0f};
	Vector3 lastRecordedAngVel = {-999.0f, -999.0f, -999.0f};
	Vector3 lastRecordedPos = {-999.0f, -999.0f, -999.0f};
	Vector3 lastRecordedRot = {-999.0f, -999.0f, -999.0f};
};