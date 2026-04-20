#define _CRT_SECURE_NO_WARNINGS // sprintf_s などの警告回避用 (念のため)

#include "CameraController.h"
#include "DirectXCommon.h"
#include "WindowAPI.h"
#include <imgui.h>
#include <ImGuizmo.h>
#include <algorithm>
#include <cmath>
#include <externals/nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

// =========================================================
// 数学処理 (ベジェ曲線)
// =========================================================
double CameraController::BinomialCoefficient(int n, int k) {
	if (k < 0 || k > n)
		return 0;
	if (k == 0 || k == n)
		return 1;
	if (k > n / 2)
		k = n - k;
	double res = 1;
	for (int i = 1; i <= k; ++i)
		res = res * (n - i + 1) / i;
	return res;
}

Vector3 CameraController::EvaluateBezier(float t) {
	int n = static_cast<int>(points.size()) - 1;
	if (n < 0)
		return {0, 0, 0};
	if (n == 0)
		return points[0].position;

	Vector3 result = {0, 0, 0};
	for (int i = 0; i <= n; ++i) {
		double b = BinomialCoefficient(n, i) * std::pow(1.0 - t, n - i) * std::pow(t, i);
		result.x += static_cast<float>(points[i].position.x * b);
		result.y += static_cast<float>(points[i].position.y * b);
		result.z += static_cast<float>(points[i].position.z * b);
	}
	return result;
}

// =========================================================
// 初期化・更新処理
// =========================================================
void CameraController::Initialize(Camera* targetCamera) {
	this->camera = targetCamera;

	// システム系の初期化
	dxCommon_ = DirectXCommon::GetInstance();
	windowAPI_ = std::make_unique<WindowAPI>();

	// 軌跡描画用のレールモデルを事前生成 (数は要件に合わせて調整)
	InitializeRailModels(200);

	// ステージ1をロード
	ChangeStage(1);
	lastPosition = EvaluateBezier(0.0f);
}

void CameraController::Update() {
	float deltaTime = 1.0f / 60.0f;

	if (isPlaying) {
		timer += deltaTime;
		if (timer >= totalDuration) {
			timer = totalDuration;
			isPlaying = false;
		}

		float t = std::clamp(timer / totalDuration, 0.0f, 1.0f);
		Vector3 currentPos = EvaluateBezier(t);

		// 速度計算
		float dx = currentPos.x - lastPosition.x;
		float dy = currentPos.y - lastPosition.y;
		float dz = currentPos.z - lastPosition.z;
		float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
		currentSpeed = dist / deltaTime;
		lastPosition = currentPos;
		// 進行方向を向かせる
		Vector3 forward = GetForward(t);
		// forwardからオイラー角を計算
		float yaw = atan2f(forward.x, forward.z);
		float pitch = atan2f(-forward.y, sqrtf(forward.x * forward.x + forward.z * forward.z));
	
		stageStatus.rotate = {pitch, yaw, 0.0f};
		// カメラ回転を更新
		if (camera) {
			camera->SetTranslate(currentPos);
			camera->SetRotate(stageStatus.rotate);
			camera->SetFovY(std::clamp(stageStatus.fov, kMinFov, kMaxFov));
		}
	} else {
		currentSpeed = 0.0f;
	}
}

// =========================================================
// エディタUI・描画関連
// =========================================================
void CameraController::EditorUpdate() {
#ifdef USE_IMGUI
	ImGui::Begin("CameraController Editor");

	// --- ステージ切り替え ---
	ImGui::Text("Active Stage: %d", currentStage);
	for (int i = 1; i <= 5; ++i) {
		std::string labelStr = "Stage " + std::to_string(i);
		if (ImGui::RadioButton(labelStr.c_str(), currentStage == i)) {
			if (currentStage != i)
				SaveToJSON(GetFilePath(i));

				ChangeStage(i);
		}
		if (i < 5)
			ImGui::SameLine();
	}

	ImGui::Separator();

	// --- パラメータ調整 ---
	if (ImGui::CollapsingHeader("Stage Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::DragFloat("Duration (s)", &totalDuration, 0.1f, 0.1f, 180.0f);
		ImGui::DragFloat3("Base Rotate", &stageStatus.rotate.x, 0.1f);
		ImGui::SliderFloat("Base FOV", &stageStatus.fov, kMinFov, kMaxFov);
	}

	ImGui::Separator();

	// --- 再生制御 ---
	ImGui::Text("Playback: %.2f / %.2f s", timer, totalDuration);
	if (ImGui::SliderFloat("Seek", &timer, 0.0f, totalDuration)) {
		float t = std::clamp(timer / totalDuration, 0.0f, 1.0f);
		Vector3 p = EvaluateBezier(t);
		if (camera)
			camera->SetTranslate(p);
	}

	if (ImGui::Button(isPlaying ? "Pause" : "Play Path"))
		isPlaying = !isPlaying;
	ImGui::SameLine();
	if (ImGui::Button("Reset")) {
		timer = 0.0f;
		isPlaying = false;
	}

	ImGui::Separator();

	// --- 制御点管理 ---
	if (ImGui::Button("Add Point")) {
		Vector3 pos = camera ? camera->GetTranslate() : Vector3{0, 0, 0};
		AddPoint(pos);
	}

	// 無限ループ・ラベルバグを修正したループ
	for (int i = 0; i < (int)points.size(); ++i) {
		std::string labelStr = "CP[" + std::to_string(i) + "]";
		if (ImGui::Selectable(labelStr.c_str(), selectedPoint == i))
			selectedPoint = i;
	}

	if (selectedPoint >= 0 && selectedPoint < (int)points.size()) {
		ImGui::DragFloat3("Selected CP Pos", &points[selectedPoint].position.x, 0.1f);
		if (ImGui::Button("Delete Point") && points.size() > 2) {
			points.erase(points.begin() + selectedPoint);
			spheres.erase(spheres.begin() + selectedPoint);
			selectedPoint = -1;
		}
	}

	ImGui::Separator();

	// --- セーブ ---
	if (ImGui::Button("Save Current Stage", ImVec2(-1, 30))) {
		SaveToJSON(GetFilePath(currentStage));
	}
	Vector3 camPos = camera ? camera->GetTranslate() : Vector3{0, 0, 0};
	ImGui::DragFloat3("camera position", &camPos.x, 0.1f);
	ImGui::End();

	// ギズモの更新
	UpdateGizmo();
#endif
}

void CameraController::EditorDraw() {
#ifdef USE_IMGUI
	if (!isPlaying) {
		// マウスによる制御点の選択
		// ImGuiウィンドウ上での操作時は、3D側の選択判定を無視する
		if (ImGui::IsMouseClicked(0) && !ImGuizmo::IsOver() && !ImGui::GetIO().WantCaptureMouse) {
			SelectPointByMouse();
		}

		// スフィアの描画
		size_t count = std::min(points.size(), spheres.size());
		for (size_t i = 0; i < count; i++) {
			spheres[i]->SetTranslate(points[i].position);
			spheres[i]->Update();
			spheres[i]->Draw();
		}

		// 軌跡の描画
		DrawRailModels();
	}
#endif
}

// =========================================================
// 描画・ギズモ操作ヘルパー
// =========================================================
void CameraController::AddPoint(const Vector3& pos) {
	points.push_back({pos});
	auto obj = std::make_unique<Object>();
	obj->Initialize(camera);
	obj->SetModel("cube.obj"); // 任意のモデル名に変更してください
	obj->SetScale({0.5f, 0.5f, 0.5f});
	obj->SetTranslate(pos);
	spheres.push_back(std::move(obj));
}

void CameraController::SyncSpheres() {
	spheres.clear();
	for (const auto& p : points) {
		auto obj = std::make_unique<Object>();
		obj->Initialize(camera);
		obj->SetModel("cube.obj");
		obj->SetScale({0.5f, 0.5f, 0.5f});
		obj->SetTranslate(p.position);
		spheres.push_back(std::move(obj));
	}
}

void CameraController::InitializeRailModels(int count) {
	railModels.clear();
	for (int i = 0; i < count; i++) {
		auto obj = std::make_unique<Object>();
		obj->Initialize(camera);
		obj->SetModel("rail.obj");
		railModels.push_back(std::move(obj));
	}
}

void CameraController::DrawRailModels() {
	if (points.size() < 2)
		return;

	// t=0.0 から t=1.0 までをレールモデルの数で等分
	float step = 1.0f / (float)railModels.size();
	int index = 0;

	for (float t = 0.0f; t <= 1.0f; t += step) {
		if (index >= (int)railModels.size())
			break;

		Vector3 pos = EvaluateBezier(t);
		railModels[index]->SetTranslate(pos);
		railModels[index]->SetScale({0.1f, 0.1f, 0.1f});
		railModels[index]->Update();
		railModels[index]->Draw();
		index++;
	}
}

void CameraController::UpdateGizmo() {
#ifdef USE_IMGUI
	if (selectedPoint < 0 || selectedPoint >= (int)points.size())
		return;

	Matrix4x4 view = camera->GetViewMatrix();
	Matrix4x4 proj = camera->GetProjectionMatrix();
	Matrix4x4 world = MakeTranslateMatrix(points[selectedPoint].position);

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::BeginFrame();
	ImGuizmo::SetRect(0, 0, (float)windowAPI_->kClientWidth, (float)windowAPI_->kClientHeight);

	ImGuizmo::Manipulate(&view.m[0][0], &proj.m[0][0], ImGuizmo::TRANSLATE, ImGuizmo::WORLD, &world.m[0][0]);

	if (ImGuizmo::IsUsing()) {
		points[selectedPoint].position = {world.m[3][0], world.m[3][1], world.m[3][2]};
	}
#endif
}

void CameraController::SelectPointByMouse() {
#ifdef USE_IMGUI
	ImVec2 mousePos = ImGui::GetMousePos();
	float mx = mousePos.x / windowAPI_->kClientWidth * 2.0f - 1.0f;
	float my = -(mousePos.y / windowAPI_->kClientHeight * 2.0f - 1.0f);

	Matrix4x4 invVP = Inverse(camera->GetViewProjectionMatrix());
	Vector3 rayNear = VectorTransform({mx, my, 0.0f}, invVP);
	Vector3 rayFar = VectorTransform({mx, my, 1.0f}, invVP);
	Vector3 rayDir = Normalize(rayFar - rayNear);

	float minDist = FLT_MAX;
	int hit = -1;
	for (int i = 0; i < (int)points.size(); i++) {
		float dist = RaySphereIntersect(rayNear, rayDir, points[i].position, 0.5f);
		if (dist > 0 && dist < minDist) {
			minDist = dist;
			hit = i;
		}
	}
	selectedPoint = hit;
#endif
}

// =========================================================
// データ保存・読み込み (JSON)
// =========================================================
void CameraController::ChangeStage(int newStage) {
	currentStage = newStage;
	LoadFromJSON(GetFilePath(currentStage));

	timer = 0.0f;
	isPlaying = false;
	selectedPoint = -1;
	if (!points.empty())
		lastPosition = EvaluateBezier(0.0f);
}

std::string CameraController::GetFilePath(int slot) const { return "Resource/Data/camera_stage_" + std::to_string(slot) + ".json"; }

void CameraController::SaveToJSON(const std::string& filename) {
	json j;
	j["totalDuration"] = totalDuration;
	j["rotate"] = {
	    {"x", stageStatus.rotate.x},
        {"y", stageStatus.rotate.y},
        {"z", stageStatus.rotate.z}
    };
	j["fov"] = stageStatus.fov;

	json ptsJson = json::array();
	for (const auto& p : points) {
		ptsJson.push_back({
		    {"x", p.position.x},
            {"y", p.position.y},
            {"z", p.position.z}
        });
	}
	j["controlPoints"] = ptsJson;

	std::ofstream file(filename);
	if (file.is_open())
		file << j.dump(4);
}

void CameraController::LoadFromJSON(const std::string& filename) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		points = {{{0, 0, 0}}, {{0, 0, 10}}};
		totalDuration = 5.0f;
		stageStatus.rotate = {0, 0, 0};
		stageStatus.fov = 45.0f;
		SyncSpheres(); // デフォルト作成時もスフィアを生成
		return;
	}
	try {
		json j;
		file >> j;
		totalDuration = j.value("totalDuration", 5.0f);
		stageStatus.rotate = {j["rotate"]["x"], j["rotate"]["y"], j["rotate"]["z"]};
		stageStatus.fov = j.value("fov", 45.0f);
		points.clear();
		for (const auto& p : j["controlPoints"]) {
			points.push_back({
			    {p["x"], p["y"], p["z"]}
            });
		}
		SyncSpheres(); // JSONロード後にスフィアを再生成
	} catch (...) {
	}
}


Vector3 CameraController::GetForward(float distance) {
	float look = 0.01f;
	Vector3 p0 = Evaluate(distance);
	Vector3 p1 = Evaluate(distance + look);
	return Normalize(p1 - p0);
}

Vector3 CameraController::Evaluate(float distance) {
	if (points.size() < 4) {
		return Vector3{0, 0, 0};
	}
	float maxT = (float)(points.size() - 3);
	// distance制限
	if (distance < 0.0f)
		distance = 0.0f;
	if (distance > maxT - 0.001f)
		distance = maxT - 0.001f;
	int i = (int)distance;
	float localT = distance - i;
	Vector3 p0 = points[i].position;
	Vector3 p1 = points[i + 1].position;
	Vector3 p2 = points[i + 2].position;
	Vector3 p3 = points[i + 3].position;
	return CatmullRom(p0, p1, p2, p3, localT);
}

Vector3 CameraController::CatmullRom(Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3, float t) {
	float t2 = t * t;
	float t3 = t2 * t;
	return ((p1 * 2.0f) + (-p0 + p2) * t + (p0 * 2 - p1 * 5 + p2 * 4 - p3) * t2 + (-p0 + p1 * 3 - p2 * 3 + p3) * t3) * 0.5f;
}
