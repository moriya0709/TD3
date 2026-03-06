#pragma once
#include "BaseScene.h"
#include "Camera.h"
#include "CameraManager.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "ModelManager.h"
#include "Object.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "PostEffect.h"
#include "SoundManager.h"
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include "Sprite.h"

struct InputEvent {
	float time;
	std::string key;
	bool isPressed;
};

class CameraController {
public:
	void Initialize(Camera* camera);
	void Update();

private:
	class CameraInterface {
	public:
		virtual void SetRotate(Vector3 r) = 0;
		virtual void SetTranslate(Vector3 t) = 0;
	};

	// メンバ変数
	Camera* camera; // 実際のカメラクラスの型に書き換えてください
	Transform cameraTransform = {
	    {0, 0, 0},
        {0, 0, 0}
    };
	Transform initialTransform = {
	    {0, 0, 0},
        {0, 0, 0}
    };

	std::vector<InputEvent> inputHistory;
	std::map<std::string, bool> replayKey;
	std::map<std::string, bool> lastKeyStates;

	float timer = 0.0f;
	bool isReplaying = false;
	size_t replayIndex = 0;
	bool lastRKey = false;
	void ProcessNormalInput();
	void ProcessReplay();
	void ApplyPhysics(bool w, bool a, bool s, bool d, bool up, bool down, bool left, bool right);
	void CheckAndRecord(std::string keyName, bool pressed);
	void StartReplay();
	void SaveToJSON(const std::string& filename);
	void LoadFromJSON(const std::string& filename);
};
