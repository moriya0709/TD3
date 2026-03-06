#include "CameraContoroller.h"
#include "ObjectCommon.h"
#include "SceneManager.h"
#include "SpriteCommon.h"
#include <iostream>

#include <externals/nlohmann/json.hpp>
#include <filesystem> // ファイル存在確認用
#include <fstream>

using json = nlohmann::json;

// --- nlohmann/json 用の変換定義 ---

// 構造体 -> JSON
void to_json(json& j, const InputEvent& e) {
	j = json{
	    {"time",    e.time     },
        {"key",     e.key      },
        {"pressed", e.isPressed}
    };
}

// JSON -> 構造体 (読み込みに必要)
void from_json(const json& j, InputEvent& e) {
	j.at("time").get_to(e.time);
	j.at("key").get_to(e.key);
	j.at("pressed").get_to(e.isPressed);
}

void CameraController::Initialize(Camera* targetCamera) {
	this->camera = targetCamera;

	// 初期値を保存
	if (camera) {
		initialTransform.rotate = camera->GetRotate();
		initialTransform.translate = camera->GetTranslate();
	}

	cameraTransform = initialTransform;

	// --- 起動時の自動読み込みとお手本再生 ---
	const std::string filePath = "Resources/Data/replay.json";
	if (std::filesystem::exists(filePath)) {
		LoadFromJSON(filePath);
		if (!inputHistory.empty()) {
			std::cout << "Replay file loaded. Starting demo..." << std::endl;
			StartReplay();
		}
	}
}

void CameraController::Update() {
	// デルタタイムの取得（60fps固定想定）
	float deltaTime = 1.0f / 60.0f;
	timer += deltaTime;

	if (isReplaying) {
		ProcessReplay();
	} else {
		ProcessNormalInput();
	}

	// カメラへの反映
	if (camera) {
		camera->SetRotate({cameraTransform.rotate});
		camera->SetTranslate({cameraTransform.translate});
	}
}

void CameraController::ProcessNormalInput() {
	auto input = Input::GetInstance();

	bool w = input->PushKey(DIK_W);
	bool a = input->PushKey(DIK_A);
	bool s = input->PushKey(DIK_S);
	bool d = input->PushKey(DIK_D);
	bool up = input->PushKey(DIK_UP);
	bool down = input->PushKey(DIK_DOWN);
	bool left = input->PushKey(DIK_LEFT);
	bool right = input->PushKey(DIK_RIGHT);

	CheckAndRecord("W", w);
	CheckAndRecord("A", a);
	CheckAndRecord("S", s);
	CheckAndRecord("D", d);
	CheckAndRecord("UP", up);
	CheckAndRecord("DOWN", down);
	CheckAndRecord("LEFT", left);
	CheckAndRecord("RIGHT", right);

	bool currentR = input->PushKey(DIK_R);
	if (currentR && !lastRKey) {
		SaveToJSON("Resources/Data/replay.json");
		StartReplay();
	}
	lastRKey = currentR;

	ApplyPhysics(w, a, s, d, up, down, left, right);
}

void CameraController::ProcessReplay() {
	while (replayIndex < inputHistory.size() && timer >= inputHistory[replayIndex].time) {
		replayKey[inputHistory[replayIndex].key] = inputHistory[replayIndex].isPressed;
		replayIndex++;
	}

	if (replayIndex >= inputHistory.size()) {
		isReplaying = false;
		std::cout << "Replay finished. Switched to manual control." << std::endl;
	}

	ApplyPhysics(replayKey["W"], replayKey["A"], replayKey["S"], replayKey["D"], replayKey["UP"], replayKey["DOWN"], replayKey["LEFT"], replayKey["RIGHT"]);
}

void CameraController::ApplyPhysics(bool w, bool a, bool s, bool d, bool up, bool down, bool left, bool right) {
	const float moveSpeed = 0.5f;
	const float rotateSpeed = 0.05f;

	if (w)
		cameraTransform.translate.z += moveSpeed;
	if (s)
		cameraTransform.translate.z -= moveSpeed;
	if (a)
		cameraTransform.translate.x -= moveSpeed;
	if (d)
		cameraTransform.translate.x += moveSpeed;

	if (up)
		cameraTransform.rotate.x -= rotateSpeed;
	if (down)
		cameraTransform.rotate.x += rotateSpeed;
	if (left)
		cameraTransform.rotate.y -= rotateSpeed;
	if (right)
		cameraTransform.rotate.y += rotateSpeed;
}

void CameraController::CheckAndRecord(std::string keyName, bool pressed) {
	if (pressed != lastKeyStates[keyName]) {
		inputHistory.push_back({timer, keyName, pressed});
		lastKeyStates[keyName] = pressed;
	}
}

void CameraController::StartReplay() {
	if (inputHistory.empty())
		return;

	isReplaying = true;
	timer = 0.0f;
	replayIndex = 0;
	cameraTransform = initialTransform;
	replayKey.clear();
}

void CameraController::SaveToJSON(const std::string& filename) {
	json j = inputHistory;
	std::ofstream file(filename);
	if (file.is_open()) {
		file << j.dump(4);
		std::cout << "Data saved to " << filename << std::endl;
	}
}

// 追加：JSONからデータをロードする関数
void CameraController::LoadFromJSON(const std::string& filename) {
	std::ifstream file(filename);
	if (file.is_open()) {
		try {
			json j;
			file >> j;
			inputHistory = j.get<std::vector<InputEvent>>();
		} catch (const json::exception& e) {
			std::cerr << "JSON Load Error: " << e.what() << std::endl;
		}
	}
}