#include "CameraContoroller.h"
#include <externals/nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>

using json = nlohmann::json;
namespace fs = std::filesystem;

const std::string kFilePath = "Resource/Data/replay.json";

// --- nlohmann/json 変換定義 ---
void to_json(json& j, const Vector3& v) {
	j = json{
	    {"x", v.x},
        {"y", v.y},
        {"z", v.z}
    };
}
void from_json(const json& j, Vector3& v) {
	j.at("x").get_to(v.x);
	j.at("y").get_to(v.y);
	j.at("z").get_to(v.z);
}

// CameraStateの変換
void to_json(json& j, const CameraState& s) {
	j = json{
	    {"time",   s.time           },
        {"vel",    s.velocity       },
        {"angVel", s.angularVelocity}
    };
}
void from_json(const json& j, CameraState& s) {
	j.at("time").get_to(s.time);
	j.at("vel").get_to(s.velocity);
	j.at("angVel").get_to(s.angularVelocity);
}

// --- メイン実装 ---

void CameraController::Initialize(Camera* targetCamera) {
	this->camera = targetCamera;
	timer = 0.0f;

	if (camera) {
		initialTransform.rotate = camera->GetRotate();
		initialTransform.translate = camera->GetTranslate();
	}
	cameraTransform = initialTransform;

	if (fs::exists(kFilePath)) {
		LoadFromJSON(kFilePath);
		if (!stateHistory.empty()) {
			StartReplay();
		}
	}
}

void CameraController::Update() {
	float deltaTime = 1.0f / 60.0f; // 実測のDeltaTimeを推奨
	timer += deltaTime;

	Vector3 currentVel = {0, 0, 0};
	Vector3 currentAngVel = {0, 0, 0};

	if (isReplaying) {
		// リプレイ中：記録された速度を再現
		ApplyReplayState(currentVel, currentAngVel);
	} else {
		// 通常時：入力から速度を計算して記録
		CalculateVelocityFromInput(currentVel, currentAngVel);
		RecordStateIfChanged(currentVel, currentAngVel);
	}

	// 計算された速度を座標に適用（物理移動）
	cameraTransform.translate.x += currentVel.x;
	cameraTransform.translate.y += currentVel.y;
	cameraTransform.translate.z += currentVel.z;
	cameraTransform.rotate.x += currentAngVel.x;
	cameraTransform.rotate.y += currentAngVel.y;
	cameraTransform.rotate.z += currentAngVel.z;

	if (camera) {
		camera->SetRotate(cameraTransform.rotate);
		camera->SetTranslate(cameraTransform.translate);
	}
}

void CameraController::CalculateVelocityFromInput(Vector3& vel, Vector3& angVel) {
	auto input = Input::GetInstance();
	const float moveSpeed = 0.5f;
	const float rotateSpeed = 0.05f;

	if (input->PushKey(DIK_W))
		vel.z += moveSpeed;
	if (input->PushKey(DIK_S))
		vel.z -= moveSpeed;
	if (input->PushKey(DIK_A))
		vel.x -= moveSpeed;
	if (input->PushKey(DIK_D))
		vel.x += moveSpeed;

	if (input->PushKey(DIK_UP))
		angVel.x -= rotateSpeed;
	if (input->PushKey(DIK_DOWN))
		angVel.x += rotateSpeed;
	if (input->PushKey(DIK_LEFT))
		angVel.y -= rotateSpeed;
	if (input->PushKey(DIK_RIGHT))
		angVel.y += rotateSpeed;

	// Rキーで保存
	bool currentR = input->PushKey(DIK_R);
	if (currentR && !lastRKey) {
		SaveToJSON(kFilePath);
		StartReplay();
	}
	lastRKey = currentR;
}

void CameraController::RecordStateIfChanged(const Vector3& vel, const Vector3& angVel) {
	static Vector3 lastVel = {-1, -1, -1}; // 初期比較用
	static Vector3 lastAngVel = {-1, -1, -1};

	// 速度が変わった瞬間だけ記録
	if (vel.x != lastVel.x || vel.y != lastVel.y || vel.z != lastVel.z || angVel.x != lastAngVel.x || angVel.y != lastAngVel.y || angVel.z != lastAngVel.z) {

		stateHistory.push_back({timer, vel, angVel});
		lastVel = vel;
		lastAngVel = angVel;
	}
}

void CameraController::ApplyReplayState(Vector3& vel, Vector3& angVel) {
	// 現在のタイマーに該当する速度設定を探す
	while (replayIndex < stateHistory.size() && timer >= stateHistory[replayIndex].time) {
		activeVelocity = stateHistory[replayIndex].velocity;
		activeAngularVelocity = stateHistory[replayIndex].angularVelocity;
		replayIndex++;
	}

	if (replayIndex >= stateHistory.size() && timer > stateHistory.back().time + 0.1f) {
		isReplaying = false; // 全行程終了
	}

	vel = activeVelocity;
	angVel = activeAngularVelocity;
}

void CameraController::StartReplay() {
	if (stateHistory.empty())
		return;

	isReplaying = true;
	timer = 0.0f;
	replayIndex = 0;
	cameraTransform = initialTransform;
	activeVelocity = {0, 0, 0};
	activeAngularVelocity = {0, 0, 0};
}

void CameraController::SaveToJSON(const std::string& filename) {
	fs::create_directories(fs::path(filename).parent_path());

	json j;
	j["initialTransform"] = {
	    {"translate", initialTransform.translate},
        {"rotate",    initialTransform.rotate   }
    };
	j["states"] = stateHistory; // 速度の変化履歴

	std::ofstream file(filename);
	if (file.is_open()) {
		file << j.dump(4);
		std::cout << "[CameraLog] Velocity data saved to " << filename << std::endl;
	}
}

void CameraController::LoadFromJSON(const std::string& filename) {
	std::ifstream file(filename);
	if (file.is_open()) {
		try {
			json j;
			file >> j;
			initialTransform.translate = j["initialTransform"]["translate"].get<Vector3>();
			initialTransform.rotate = j["initialTransform"]["rotate"].get<Vector3>();
			stateHistory = j["states"].get<std::vector<CameraState>>();
		} catch (const std::exception& e) {
			std::cerr << "[Error] Load failed: " << e.what() << std::endl;
		}
	}
}