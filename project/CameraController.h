#pragma once
#include "Camera.h"
#include "Input.h"
#include <string>
#include <vector>

struct CameraState {
	float time;
	Vector3 velocity;
	Vector3 angularVelocity;
};


static void HelpMarker(const char* desc);
class CameraController {
public:
	Vector3 CameraLerp(const Vector3& start, const Vector3& end, float t);
	void Initialize(Camera* camera);
	void Update();
	void DrawImGui();

private:
	void RecordStateIfChanged(const Vector3& vel, const Vector3& angVel);
	void ApplyReplayState(Vector3& vel, Vector3& angVel);
	void ApplyPhysics(const Vector3& vel, const Vector3& angVel);
	void StartReplay();
	void SeekTo(float targetTime);
	void UpdateOrInsertKeyframe(float time, const Vector3& vel, const Vector3& angVel);
	void SetCurrentAsInitial();
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
	bool isRecording = false; // Åö ò^âÊíÜÉtÉâÉO
	bool isSmoothMode = true; // ääÇÁÇ©Ç…ï‚ä‘Ç∑ÇÈÇ©Ç«Ç§Ç©

	size_t replayIndex = 0;
	int currentSlot = 1;
	float playbackSpeed = 1.0f;



	Vector3 activeVelocity = {0, 0, 0};
	Vector3 activeAngularVelocity = {0, 0, 0};
	Vector3 uiVelocity = {0.0f, 0.0f, 0.0f};
	Vector3 uiAngularVelocity = {0.0f, 0.0f, 0.0f};

	Vector3 lastRecordedVel = {-1.0f, -1.0f, -1.0f};
	Vector3 lastRecordedAngVel = {-1.0f, -1.0f, -1.0f};
};