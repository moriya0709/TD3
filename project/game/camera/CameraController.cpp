#include "CameraController.h"
#include <algorithm>
#include <externals/nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <iostream>

using json = nlohmann::json;
namespace fs = std::filesystem;

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

// 線形補間のヘルパー
Vector3 CameraController::CameraLerp(const Vector3& start, const Vector3& end, float t) { return {start.x + (end.x - start.x) * t, start.y + (end.y - start.y) * t, start.z + (end.z - start.z) * t}; }

void CameraController::Initialize(Camera* targetCamera) {
	this->camera = targetCamera;
	timer = 0.0f;
	isReplaying = false;
	isPaused = false;
	isRecording = false;
	currentSlot = 1;
	playbackSpeed = 1.0f;

	if (camera) {
		initialTransform.rotate = camera->GetRotate();
		initialTransform.translate = camera->GetTranslate();
	}
	cameraTransform = initialTransform;
	LoadFromJSON(GetFilePath(currentSlot));
}

std::string CameraController::GetFilePath(int slot) const { return "Resource/Data/replay_" + std::to_string(slot) + ".json"; }

void CameraController::Update() {
	auto input = Input::GetInstance();
	int newSlot = -1;
	if (input->TriggerKey(DIK_1))
		newSlot = 1;
	else if (input->TriggerKey(DIK_2))
		newSlot = 2;
	else if (input->TriggerKey(DIK_3))
		newSlot = 3;
	else if (input->TriggerKey(DIK_4))
		newSlot = 4;
	else if (input->TriggerKey(DIK_5))
		newSlot = 5;

	if (newSlot != -1 && newSlot != currentSlot) {
		currentSlot = newSlot;
		isReplaying = isRecording = false;
		LoadFromJSON(GetFilePath(currentSlot));
		if (!stateHistory.empty())
			StartReplay();
	}

	float deltaTime = 1.0f / 60.0f;
	Vector3 currentVel = {0, 0, 0};
	Vector3 currentAngVel = {0, 0, 0};

	if (isReplaying) {
		if (!isPaused) {
			timer += deltaTime * playbackSpeed;
			ApplyReplayState(currentVel, currentAngVel);
			ApplyPhysics(
			    {currentVel.x * playbackSpeed, currentVel.y * playbackSpeed, currentVel.z * playbackSpeed},
			    {currentAngVel.x * playbackSpeed, currentAngVel.y * playbackSpeed, currentAngVel.z * playbackSpeed});
		}
	} else if (isRecording) {
		timer += deltaTime;
		currentVel = uiVelocity;
		currentAngVel = uiAngularVelocity;
		RecordStateIfChanged(currentVel, currentAngVel);
		ApplyPhysics(currentVel, currentAngVel);
	} else {
		// ★ 非録画・非再生中（待機中）：常に初期座標を適用する
		cameraTransform = initialTransform;
		// ただし、UIで操作感を確認したい場合は以下を有効にする（今回は初期値固定を優先）
		// cameraTransform.translate.x += uiVelocity.x; // 必要なら
	}

	if (camera) {
		camera->SetRotate(cameraTransform.rotate);
		camera->SetTranslate(cameraTransform.translate);
	}
}

void CameraController::DrawImGui() {
	ImGui::Begin("Camera Recording Studio");

	ImGui::Text("Slot: %d", currentSlot);
	for (int i = 1; i <= 5; ++i) {
		if (ImGui::RadioButton(std::to_string(i).c_str(), currentSlot == i)) {
			currentSlot = i;
			isReplaying = isRecording = false;
			LoadFromJSON(GetFilePath(currentSlot));
			if (!stateHistory.empty())
				StartReplay();
		}
		if (i < 5)
			ImGui::SameLine();
	}

	if (ImGui::CollapsingHeader("Initial Transform Setup", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat3("Start Pos", &initialTransform.translate.x, 0.1f);
		ImGui::DragFloat3("Start Rot", &initialTransform.rotate.x, 0.01f);
		if (ImGui::Button("Set Current Pos as Initial"))
			initialTransform = cameraTransform;
	}

	ImGui::Separator();

	if (isReplaying) {
		ImGui::TextColored(ImVec4(0, 1, 1, 1), "STATUS: REPLAYING");
		ImGui::Checkbox("Smooth Interpolation", &isSmoothMode); // 補間のON/OFF
		ImGui::SliderFloat("Speed", &playbackSpeed, 0.0f, 3.0f, "%.1fx");

		float maxTime = stateHistory.empty() ? 0.0f : stateHistory.back().time;
		float seekTime = timer;
		if (ImGui::SliderFloat("Seek", &seekTime, 0.0f, maxTime))
			SeekTo(seekTime);

		if (ImGui::Button(isPaused ? "Play" : "Pause"))
			isPaused = !isPaused;
		ImGui::SameLine();
		if (ImGui::Button("Stop"))
			isReplaying = false;

	} else {
		ImGui::Text("STATUS: %s", isRecording ? "RECORDING..." : "WAITING (Initial Applied)");
		ImGui::DragFloat3("Input Vel", &uiVelocity.x, 0.01f, -1.0f, 1.0f);
		ImGui::DragFloat3("Input Rot", &uiAngularVelocity.x, 0.005f, -0.05f, 0.05f);

		if (!isRecording) {
			if (ImGui::Button("● Start Recording", ImVec2(200, 30))) {
				stateHistory.clear();
				timer = 0.0f;
				isRecording = true;
				cameraTransform = initialTransform;
				RecordStateIfChanged(uiVelocity, uiAngularVelocity);
			}
		} else {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
			if (ImGui::Button("■ Stop & Save", ImVec2(200, 30))) {
				isRecording = false;
				SaveToJSON(GetFilePath(currentSlot));
			}
			ImGui::PopStyleColor();
		}
	}
	ImGui::End();
}

void CameraController::ApplyReplayState(Vector3& vel, Vector3& angVel) {
	if (stateHistory.empty())
		return;

	// 現在のタイマー位置の前後のキーフレームを探す
	size_t nextIdx = 0;
	while (nextIdx < stateHistory.size() && timer > stateHistory[nextIdx].time) {
		nextIdx++;
	}

	if (nextIdx == 0) {
		vel = stateHistory[0].velocity;
		angVel = stateHistory[0].angularVelocity;
	} else if (nextIdx >= stateHistory.size()) {
		vel = stateHistory.back().velocity;
		angVel = stateHistory.back().angularVelocity;
		isPaused = true;
	} else {
		// ★ 滑らかにするための補間処理
		if (isSmoothMode) {
			const auto& prev = stateHistory[nextIdx - 1];
			const auto& next = stateHistory[nextIdx];
			float t = (timer - prev.time) / (next.time - prev.time);
			vel = CameraLerp(prev.velocity, next.velocity, t);
			angVel = CameraLerp(prev.angularVelocity, next.angularVelocity, t);
		} else {
			vel = stateHistory[nextIdx - 1].velocity;
			angVel = stateHistory[nextIdx - 1].angularVelocity;
		}
	}
	replayIndex = nextIdx;
}

// --- 他の物理、保存、Seek等の関数は変更なしのため維持 ---
void CameraController::ApplyPhysics(const Vector3& vel, const Vector3& angVel) {
	cameraTransform.translate.x += vel.x;
	cameraTransform.translate.y += vel.y;
	cameraTransform.translate.z += vel.z;
	cameraTransform.rotate.x += angVel.x;
	cameraTransform.rotate.y += angVel.y;
	cameraTransform.rotate.z += angVel.z;
}
void CameraController::RecordStateIfChanged(const Vector3& vel, const Vector3& angVel) {
	if (vel.x != lastRecordedVel.x || vel.y != lastRecordedVel.y || vel.z != lastRecordedVel.z || angVel.x != lastRecordedAngVel.x || angVel.y != lastRecordedAngVel.y ||
	    angVel.z != lastRecordedAngVel.z) {
		stateHistory.push_back({timer, vel, angVel});
		lastRecordedVel = vel;
		lastRecordedAngVel = angVel;
	}
}
void CameraController::StartReplay() {
	if (stateHistory.empty())
		return;
	isReplaying = true;
	isPaused = false;
	isRecording = false;
	timer = 0.0f;
	cameraTransform = initialTransform;
	activeVelocity = {0, 0, 0};
	activeAngularVelocity = {0, 0, 0};
}
void CameraController::SeekTo(float targetTime) {
	timer = targetTime;
	cameraTransform = initialTransform;
	const float simDelta = 1.0f / 60.0f;
	for (float t = 0.0f; t < targetTime; t += simDelta) {
		Vector3 v, av;
		float oldTimer = timer;
		timer = t;
		ApplyReplayState(v, av);
		timer = oldTimer;
		ApplyPhysics(v, av);
	}
	timer = targetTime;
}
void CameraController::UpdateOrInsertKeyframe(float time, const Vector3& vel, const Vector3& angVel) {
	auto it = std::find_if(stateHistory.begin(), stateHistory.end(), [time](const CameraState& s) { return std::abs(s.time - time) < 0.001f; });
	if (it != stateHistory.end()) {
		it->velocity = vel;
		it->angularVelocity = angVel;
	} else {
		stateHistory.push_back({time, vel, angVel});
		std::sort(stateHistory.begin(), stateHistory.end(), [](const CameraState& a, const CameraState& b) { return a.time < b.time; });
	}
}
void CameraController::SaveToJSON(const std::string& filename) {
	fs::create_directories(fs::path(filename).parent_path());
	json j;
	j["initialTransform"] = {
	    {"translate", initialTransform.translate},
        {"rotate",    initialTransform.rotate   }
    };
	j["states"] = stateHistory;
	std::ofstream file(filename);
	if (file.is_open())
		file << j.dump(4);
}
void CameraController::LoadFromJSON(const std::string& filename) {
	stateHistory.clear();
	if (!fs::exists(filename))
		return;
	std::ifstream file(filename);
	if (file.is_open()) {
		try {
			json j;
			file >> j;
			initialTransform.translate = j["initialTransform"]["translate"].get<Vector3>();
			initialTransform.rotate = j["initialTransform"]["rotate"].get<Vector3>();
			stateHistory = j["states"].get<std::vector<CameraState>>();
		} catch (...) {
		}
	}
}