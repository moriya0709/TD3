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

void CameraController::Initialize(Camera* targetCamera) {
	this->camera = targetCamera;
	timer = 0.0f;
	isReplaying = false;
	isPaused = false;
	currentSlot = 1;

	if (camera) {
		initialTransform.rotate = camera->GetRotate();
		initialTransform.translate = camera->GetTranslate();
	}
	cameraTransform = initialTransform;

	LoadFromJSON(GetFilePath(currentSlot));
}

std::string CameraController::GetFilePath(int slot) const { return "Resource/Data/replay_" + std::to_string(slot) + ".json"; }

void CameraController::Update() {
	// ★ 数字キー 1〜5 によるスロット切り替え処理
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
		isReplaying = false;
		LoadFromJSON(GetFilePath(currentSlot));
		std::cout << "[System] Switched to Slot " << currentSlot << std::endl;
	}

	float deltaTime = 1.0f / 60.0f;
	Vector3 currentVel = {0, 0, 0};
	Vector3 currentAngVel = {0, 0, 0};

	if (isReplaying) {
		if (!isPaused) {
			timer += deltaTime;
			ApplyReplayState(currentVel, currentAngVel);
			ApplyPhysics(currentVel, currentAngVel);
		}
	} else {
		timer += deltaTime;
		currentVel = uiVelocity;
		currentAngVel = uiAngularVelocity;
		RecordStateIfChanged(currentVel, currentAngVel);
		ApplyPhysics(currentVel, currentAngVel);
	}

	if (camera) {
		camera->SetRotate(cameraTransform.rotate);
		camera->SetTranslate(cameraTransform.translate);
	}
}

void CameraController::DrawImGui() {
	ImGui::Begin("Camera Slot Editor");
	ImGui::Text("Press 1-5 to Switch Slots");

	// スロット選択
	for (int i = 1; i <= 5; ++i) {
		std::string label = "Slot " + std::to_string(i);
		bool isSelected = (currentSlot == i);
		bool fileExists = fs::exists(GetFilePath(i));

		if (!fileExists)
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
		if (ImGui::RadioButton(label.c_str(), isSelected)) {
			if (currentSlot != i) {
				currentSlot = i;
				isReplaying = false;
				LoadFromJSON(GetFilePath(currentSlot));
			}
		}
		if (!fileExists)
			ImGui::PopStyleColor();
		if (i < 5)
			ImGui::SameLine();
	}

	ImGui::Separator();

	if (isReplaying) {
		ImGui::TextColored(ImVec4(0, 1, 1, 1), "MODE: REPLAYING (Slot %d)", currentSlot);
		float maxTime = stateHistory.empty() ? 0.0f : stateHistory.back().time;
		float seekTime = timer;
		if (ImGui::SliderFloat("Seek Time", &seekTime, 0.0f, maxTime, "%.2f sec"))
			SeekTo(seekTime);
		if (ImGui::Button(isPaused ? "Play" : "Pause"))
			isPaused = !isPaused;
		ImGui::SameLine();
		if (ImGui::Button("Stop Replay"))
			isReplaying = false;
		if (isPaused) {
			ImGui::Text("--- Manual Override ---");
			bool changed = false;
			changed |= ImGui::DragFloat3("Edit Pos", &cameraTransform.translate.x, 0.1f);
			changed |= ImGui::DragFloat3("Edit Rot", &cameraTransform.rotate.x, 0.01f);
			if (changed)
				UpdateOrInsertKeyframe(timer, {0, 0, 0}, {0, 0, 0});
		}
		if (ImGui::Button("Save Changes to Slot"))
			SaveToJSON(GetFilePath(currentSlot));
	} else {
		ImGui::Text("MODE: RECORDING (Slot %d)", currentSlot);
		ImGui::DragFloat3("Move Velocity", &uiVelocity.x, 0.01f, -1.0f, 1.0f);
		ImGui::DragFloat3("Rotate Velocity", &uiAngularVelocity.x, 0.005f, -0.1f, 0.1f);
		if (ImGui::Button("Record & Play")) {
			SaveToJSON(GetFilePath(currentSlot));
			StartReplay();
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear")) {
			stateHistory.clear();
			timer = 0.0f;
		}
	}
	ImGui::End();
}

void CameraController::RecordStateIfChanged(const Vector3& vel, const Vector3& angVel) {
	if (vel.x != lastRecordedVel.x || vel.y != lastRecordedVel.y || vel.z != lastRecordedVel.z || angVel.x != lastRecordedAngVel.x || angVel.y != lastRecordedAngVel.y ||
	    angVel.z != lastRecordedAngVel.z) {
		stateHistory.push_back({timer, vel, angVel});
		lastRecordedVel = vel;
		lastRecordedAngVel = angVel;
	}
}

void CameraController::ApplyReplayState(Vector3& vel, Vector3& angVel) {
	while (replayIndex < stateHistory.size() && timer >= stateHistory[replayIndex].time) {
		activeVelocity = stateHistory[replayIndex].velocity;
		activeAngularVelocity = stateHistory[replayIndex].angularVelocity;
		replayIndex++;
	}
	if (replayIndex >= stateHistory.size() && !stateHistory.empty() && timer >= stateHistory.back().time)
		isPaused = true;
	vel = activeVelocity;
	angVel = activeAngularVelocity;
}

void CameraController::ApplyPhysics(const Vector3& vel, const Vector3& angVel) {
	cameraTransform.translate.x += vel.x;
	cameraTransform.translate.y += vel.y;
	cameraTransform.translate.z += vel.z;
	cameraTransform.rotate.x += angVel.x;
	cameraTransform.rotate.y += angVel.y;
	cameraTransform.rotate.z += angVel.z;
}

void CameraController::StartReplay() {
	if (stateHistory.empty())
		return;
	isReplaying = true;
	isPaused = false;
	timer = 0.0f;
	replayIndex = 0;
	cameraTransform = initialTransform;
	activeVelocity = {0, 0, 0};
	activeAngularVelocity = {0, 0, 0};
}

void CameraController::SeekTo(float targetTime) {
	timer = targetTime;
	cameraTransform = initialTransform;
	activeVelocity = {0, 0, 0};
	activeAngularVelocity = {0, 0, 0};
	replayIndex = 0;
	const float simDelta = 1.0f / 60.0f;
	for (float t = 0.0f; t < targetTime; t += simDelta) {
		while (replayIndex < stateHistory.size() && t >= stateHistory[replayIndex].time) {
			activeVelocity = stateHistory[replayIndex].velocity;
			activeAngularVelocity = stateHistory[replayIndex].angularVelocity;
			replayIndex++;
		}
		ApplyPhysics(activeVelocity, activeAngularVelocity);
	}
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