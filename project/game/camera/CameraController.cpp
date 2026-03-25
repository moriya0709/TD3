#include "CameraController.h"
#include <algorithm>
#include <externals/nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

using json = nlohmann::json;
namespace fs = std::filesystem;

const float kMaxDuration = 180.0f;

// --- 描画関数 (環境に合わせて実装してください) ---
void DrawLine(const Vector3& s, const Vector3& e, uint32_t color) { /* 描画処理の実装 */ }

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
        {"angVel", s.angularVelocity},
        {"pos",    s.position       },
        {"rot",    s.rotation       }
    };
}
void from_json(const json& j, CameraState& s) {
	j.at("time").get_to(s.time);
	j.at("vel").get_to(s.velocity);
	j.at("angVel").get_to(s.angularVelocity);
	j.at("pos").get_to(s.position);
	j.at("rot").get_to(s.rotation);
}

// --- 補間ヘルパー ---
Vector3 CameraController::CameraLerp(const Vector3& start, const Vector3& end, float t) { return {start.x + (end.x - start.x) * t, start.y + (end.y - start.y) * t, start.z + (end.z - start.z) * t}; }

Vector3 CameraController::CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
	float t2 = t * t;
	float t3 = t2 * t;
	auto calc = [&](float v0, float v1, float v2, float v3) { return 0.5f * (2.0f * v1 + (-v0 + v2) * t + (2.0f * v0 - 5.0f * v1 + 4.0f * v2 - v3) * t2 + (-v0 + 3.0f * v1 - 3.0f * v2 + v3) * t3); };
	return {calc(p0.x, p1.x, p2.x, p3.x), calc(p0.y, p1.y, p2.y, p3.y), calc(p0.z, p1.z, p2.z, p3.z)};
}

void CameraController::Initialize(Camera* targetCamera) {
	this->camera = targetCamera;
	timer = 0.0f;
	isReplaying = isPaused = isRecording = false;
	currentStage = 1;
	playbackSpeed = 1.0f;
	if (camera) {
		initialTransform.rotate = camera->GetRotate();
		initialTransform.translate = camera->GetTranslate();
	}
	cameraTransform = initialTransform;
	LoadFromJSON(GetFilePath(currentStage));
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

	if (newSlot != -1 && newSlot != currentStage) {
		currentStage = newSlot;
		isReplaying = isRecording = false;
		LoadFromJSON(GetFilePath(currentStage));
		if (!stateHistory.empty())
			StartReplay();
	}

	float deltaTime = 1.0f / 60.0f;
	Vector3 currentVel = {0, 0, 0};
	Vector3 currentAngVel = {0, 0, 0};
	Vector3 currentPos = cameraTransform.translate;
	Vector3 currentRot = cameraTransform.rotate;

	if (isReplaying) {
		if (!isPaused) {
			timer += deltaTime * playbackSpeed;
			if (timer >= kMaxDuration) {
				timer = kMaxDuration;
				isPaused = true;
			}

			// 再生中は補間座標を取得して適用
			ApplyReplayState(currentVel, currentAngVel, currentPos, currentRot);
			ApplyPhysics(currentVel, currentAngVel, currentPos, currentRot);
		}
		// ★ 一時停止中は ApplyPhysics を呼ばないことで、ImGuiからの手動座標変更を維持する
	} else if (isRecording) {
		timer += deltaTime;
		if (timer >= kMaxDuration) {
			timer = kMaxDuration;
			isRecording = false;
			SaveToJSON(GetFilePath(currentStage));
		}
		currentVel = uiVelocity;
		currentAngVel = uiAngularVelocity;

		RecordStateIfChanged(currentVel, currentAngVel, cameraTransform.translate, cameraTransform.rotate);
		ApplyPhysics(currentVel, currentAngVel, cameraTransform.translate, cameraTransform.rotate);
	} else {
		cameraTransform = initialTransform;
	}

	if (camera) {
		camera->SetRotate(cameraTransform.rotate);
		camera->SetTranslate(cameraTransform.translate);
	}
}

void CameraController::DrawImGui() {
#ifdef USE_IMGUI
	ImGui::Begin("Camera Recording Studio");
	ImGui::Checkbox("Show Trace", &showDebugTrace);
	ImGui::SameLine();
	ImGui::Checkbox("Spline Mode", &isSmoothMode);
	ImGui::Separator();

	ImGui::Text("Stage: %d", currentStage);
	for (int i = 1; i <= 5; ++i) {
		if (ImGui::RadioButton(std::to_string(i).c_str(), currentStage == i)) {
			currentStage = i;
			isReplaying = isRecording = false;
			LoadFromJSON(GetFilePath(currentStage));
			if (!stateHistory.empty())
				StartReplay();
		}
		if (i < 5)
			ImGui::SameLine();
	}

	if (ImGui::CollapsingHeader("Initial Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat3("Start Pos", &initialTransform.translate.x, 0.1f);
		ImGui::DragFloat3("Start Rot", &initialTransform.rotate.x, 0.01f);
		if (ImGui::Button("Set Current as Initial"))
			initialTransform = cameraTransform;
	}

	ImGui::Separator();

	if (isReplaying) {
		ImGui::TextColored(ImVec4(0, 1, 1, 1), "STATUS: REPLAYING %s", isPaused ? "(PAUSED)" : "");
		ImGui::SliderFloat("Speed", &playbackSpeed, 0.0f, 3.0f, "%.1fx");
		if (ImGui::SliderFloat("Seek", &timer, 0.0f, kMaxDuration, "%.2f / 180s"))
			SeekTo(timer);

		if (ImGui::Button(isPaused ? "Play" : "Pause"))
			isPaused = !isPaused;
		ImGui::SameLine();
		if (ImGui::Button("Stop"))
			isReplaying = false;

		// ★ 一時停止中のみ表示される上書き録画メニュー
		if (isPaused) {
			ImGui::Separator();
			ImGui::TextColored(ImVec4(1, 0.8f, 0, 1), "Edit Mode: Manual Tweak Available");
			ImGui::DragFloat3("Tweak Vel", &uiVelocity.x, 0.01f, -1.0f, 1.0f);
			ImGui::DragFloat3("Tweak Rot Vel", &uiAngularVelocity.x, 0.005f, -0.05f, 0.05f);
			ImGui::DragFloat3("Tweak Pos", &cameraTransform.translate.x, 0.1f);
			ImGui::DragFloat3("Tweak Rot", &cameraTransform.rotate.x, 0.01f);
			if (ImGui::Button("● Start Overwrite Recording from Here", ImVec2(-1, 30))) {
				StartOverwriteRecording();
			}
		}
	} else {
		ImGui::Text("STATUS: %s", isRecording ? "RECORDING..." : "WAITING");
		if (isRecording)
			ImGui::ProgressBar(timer / kMaxDuration);

		ImGui::DragFloat3("Input Vel", &uiVelocity.x, 0.01f, -1.0f, 1.0f);
		ImGui::DragFloat3("Input Rot", &uiAngularVelocity.x, 0.005f, -0.05f, 0.05f);
		ImGui::DragFloat3("Current Pos", &cameraTransform.translate.x, 0.1f);
		ImGui::DragFloat3("Current Rot", &cameraTransform.rotate.x, 0.01f);

		if (!isRecording) {
			ImGui::Checkbox("Recording new Data ", &newRecordingStarted);
			if (newRecordingStarted) {
				// ★ 既存データがある場合は警告表示
				if (!stateHistory.empty()) {
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "Warning: Existing replay data will be overwritten!");
				}
				if (ImGui::Button("● Start New Recording", ImVec2(-1, 30))) {
					stateHistory.clear();
					timer = 0.0f;
					isRecording = true;
					cameraTransform = initialTransform;
					RecordStateIfChanged(uiVelocity, uiAngularVelocity, cameraTransform.translate, cameraTransform.rotate);
				}
			} else {
				ImGui::Text("Starting a new recording will overwrite the existing replay data for this stage.");
				
				// 保存されているデータを再生
				if (ImGui::Button("Play Existing Recording", ImVec2(-1, 30))) {
					if (!stateHistory.empty()) {
						StartReplay();
					} else {
						ImGui::TextColored(ImVec4(1, 0, 0, 1), "No replay data found for this stage!");
					}
				}
			}
			
		} else {
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
			if (ImGui::Button("■ Stop & Save", ImVec2(-1, 30))) {
				isRecording = false;
				SaveToJSON(GetFilePath(currentStage));
			}
			ImGui::PopStyleColor();
		}
	}
	ImGui::End();

#endif 
}

void CameraController::DrawDebugTrace() {
	if (!showDebugTrace || stateHistory.empty())
		return;
	Vector3 prevPos = initialTransform.translate;
	Vector3 currentPos = initialTransform.translate;
	float simTime = 0.0f;
	float maxDataTime = stateHistory.back().time;
	float step = 1.0f / 60.0f;
	float backupTimer = timer;

	while (simTime <= maxDataTime && simTime <= kMaxDuration) {
		timer = simTime;
		Vector3 v, av, p, r;
		ApplyReplayState(v, av, p, r);
		currentPos.x += v.x;
		currentPos.y += v.y;
		currentPos.z += v.z;
		DrawLine(prevPos, currentPos, 0x00FF00FF);
		prevPos = currentPos;
		simTime += step;
	}
	timer = backupTimer;
}

void CameraController::ApplyReplayState(Vector3& vel, Vector3& angVel, Vector3& pos, Vector3& rat) {
	if (stateHistory.empty())
		return;
	size_t n = stateHistory.size();
	size_t i1 = 0;
	while (i1 < n && timer > stateHistory[i1].time)
		i1++;

	if (i1 == 0) {
		vel = stateHistory[0].velocity;
		angVel = stateHistory[0].angularVelocity;
		pos = stateHistory[0].position;
		rat = stateHistory[0].rotation;
	} else if (i1 >= n) {
		vel = stateHistory.back().velocity;
		angVel = stateHistory.back().angularVelocity;
		pos = stateHistory.back().position;
		rat = stateHistory.back().rotation;
	} else {
		size_t i0 = i1 - 1;
		float t = (timer - stateHistory[i0].time) / (stateHistory[i1].time - stateHistory[i0].time);
		if (isSmoothMode && n >= 4) {
			size_t im1 = (i0 == 0) ? i0 : i0 - 1;
			size_t i2 = (i1 + 1 >= n) ? i1 : i1 + 1;
			vel = CatmullRom(stateHistory[im1].velocity, stateHistory[i0].velocity, stateHistory[i1].velocity, stateHistory[i2].velocity, t);
			angVel = CatmullRom(stateHistory[im1].angularVelocity, stateHistory[i0].angularVelocity, stateHistory[i1].angularVelocity, stateHistory[i2].angularVelocity, t);
			pos = CatmullRom(stateHistory[im1].position, stateHistory[i0].position, stateHistory[i1].position, stateHistory[i2].position, t);
			rat = CatmullRom(stateHistory[im1].rotation, stateHistory[i0].rotation, stateHistory[i1].rotation, stateHistory[i2].rotation, t);
		} else {
			vel = CameraLerp(stateHistory[i0].velocity, stateHistory[i1].velocity, t);
			angVel = CameraLerp(stateHistory[i0].angularVelocity, stateHistory[i1].angularVelocity, t);
			pos = CameraLerp(stateHistory[i0].position, stateHistory[i1].position, t);
			rat = CameraLerp(stateHistory[i0].rotation, stateHistory[i1].rotation, t);
		}
	}
}

void CameraController::ApplyPhysics(const Vector3& vel, const Vector3& angVel, const Vector3& pos, const Vector3& rat) {
	if (isReplaying) {
		cameraTransform.translate = pos;
		cameraTransform.rotate = rat;
	} else {
		cameraTransform.translate.x += vel.x;
		cameraTransform.translate.y += vel.y;
		cameraTransform.translate.z += vel.z;
		cameraTransform.rotate.x += angVel.x;
		cameraTransform.rotate.y += angVel.y;
		cameraTransform.rotate.z += angVel.z;
	}
}

void CameraController::RecordStateIfChanged(const Vector3& vel, const Vector3& angVel, const Vector3& pos, const Vector3& rat) {
	bool changed = vel.x != lastRecordedVel.x || vel.y != lastRecordedVel.y || vel.z != lastRecordedVel.z || angVel.x != lastRecordedAngVel.x || angVel.y != lastRecordedAngVel.y ||
	               angVel.z != lastRecordedAngVel.z || pos.x != lastRecordedPos.x || pos.y != lastRecordedPos.y || pos.z != lastRecordedPos.z || rat.x != lastRecordedRot.x ||
	               rat.y != lastRecordedRot.y || rat.z != lastRecordedRot.z;

	if (changed) {
		stateHistory.push_back({timer, vel, angVel, pos, rat});
		lastRecordedVel = vel;
		lastRecordedAngVel = angVel;
		lastRecordedPos = pos;
		lastRecordedRot = rat;
	}
}

void CameraController::StartOverwriteRecording() {
	if (!isReplaying || !isPaused)
		return;

	// 現在のタイマー以降のデータを削除
	stateHistory.erase(std::remove_if(stateHistory.begin(), stateHistory.end(), [this](const CameraState& s) { return s.time > timer; }), stateHistory.end());

	isReplaying = isPaused = false;
	isRecording = true;

	// 強制的に初動を記録させるためにリセット
	lastRecordedVel = {-999, -999, -999};
	lastRecordedPos = {-999, -999, -999};
	RecordStateIfChanged(uiVelocity, uiAngularVelocity, cameraTransform.translate, cameraTransform.rotate);
}

void CameraController::StartReplay() {
	if (stateHistory.empty())
		return;
	isReplaying = true;
	isPaused = isRecording = false;
	timer = 0.0f;
	cameraTransform = initialTransform;
}

void CameraController::SeekTo(float targetTime) {
	timer = targetTime;
	if (stateHistory.empty()) {
		cameraTransform = initialTransform;
		return;
	}
	Vector3 v, av, p, r;
	ApplyReplayState(v, av, p, r);
	cameraTransform.translate = p;
	cameraTransform.rotate = r;
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