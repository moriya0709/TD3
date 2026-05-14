#define _CRT_SECURE_NO_WARNINGS // sprintf_s などの警告回避用 (念のため)

#include "StageCameraController.h"
#include "DirectXCommon.h"
#include "WindowAPI.h"
#include <ImGuizmo.h>
#include <algorithm>
#include <cmath>
#include <externals/nlohmann/json.hpp>
#include <fstream>
#include <imgui.h>

using json = nlohmann::json;

// =========================================================
// 数学処理 (ベジェ曲線)
// =========================================================
double StageCameraController::BinomialCoefficient(int n, int k) {
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

Vector3 StageCameraController::EvaluateBezier(float t) {
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
void StageCameraController::Initialize(Camera* targetCamera) {
	this->camera = targetCamera;

	// システム系の初期化
	dxCommon_ = DirectXCommon::GetInstance();
	windowAPI_ = std::make_unique<WindowAPI>();

	// 軌跡描画用のレールモデルを事前生成 (数は要件に合わせて調整)
	InitializeRailModels(200);
	camera->SetRotate({0.0f, 0.0f, 0.0f}); // カメラの初期回転を設定

	// ステージ1をロード
	ChangeStage(1);
	lastPosition = EvaluateBezier(0.0f);
}

void StageCameraController::Update() {
	float deltaTime = 1.0f / 60.0f;

	if (isPlaying) {
		// --- 1. タイマー更新 ---
		timer += deltaTime;
		if (timer >= totalDuration) {
			timer = totalDuration;
			isPlaying = false;
		}

		// --- 3. 位置の計算 (ベジェ曲線) ---
		float t = std::clamp(timer / totalDuration, 0.0f, 1.0f);
		Vector3 currentPos = EvaluateBezier(t);
		// --- 回転速度の補間計算 ---
		Vector3 targetSpeed = {0, 0, 0};
		Vector3 startSpeed = {0, 0, 0};
		float startTime = 0.0f;
		float duration = 0.0f;

		// 現在のtimerがどの履歴区間にいるか探す
		for (size_t i = 0; i < rotationHistory.size(); ++i) {
			if (timer >= rotationHistory[i].time) {
				// 現在のターゲット
				targetSpeed = rotationHistory[i].rotationSpeed;
				startTime = rotationHistory[i].time;
				duration = rotationHistory[i].transitionTime;

				// 開始時の速度（1つ前の履歴の速度。なければ0）
				startSpeed = (i > 0) ? rotationHistory[i - 1].rotationSpeed : Vector3{0, 0, 0};
			}
		}

		// 補間係数 (t) を計算: 0.0 (開始) ～ 1.0 (完了)
		float t_interp = 1.0f;
		if (duration > 0.001f) {
			t_interp = std::clamp((timer - startTime) / duration, 0.0f, 1.0f);
		}

		// 線形補間で現在の速度を算出
		currentRotationSpeed.x = startSpeed.x + (targetSpeed.x - startSpeed.x) * t_interp;
		currentRotationSpeed.y = startSpeed.y + (targetSpeed.y - startSpeed.y) * t_interp;
		currentRotationSpeed.z = startSpeed.z + (targetSpeed.z - startSpeed.z) * t_interp;

		stageStatus.rotate.x += currentRotationSpeed.x * deltaTime;
		stageStatus.rotate.y += currentRotationSpeed.y * deltaTime;
		stageStatus.rotate.z += currentRotationSpeed.z * deltaTime;

		// --- 5. カメラへの反映 ---
		if (camera) {
			camera->SetTranslate(currentPos);
			camera->SetRotate(stageStatus.rotate);
			camera->SetFovY(std::clamp(stageStatus.fov, kMinFov, kMaxFov));
		}

		lastPosition = currentPos;
	} else {
		currentSpeed = 0.0f;
	}
}
// =========================================================
// エディタUI・描画関連
// =========================================================
void StageCameraController::EditorUpdate() {
#ifdef USE_IMGUI
	ImGui::Begin("StageCameraController Editor");

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
		ImGui::DragFloat("Base FOV", &stageStatus.fov, 0.1f, kMinFov, kMaxFov);
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
	if (ImGui::CollapsingHeader("Initial Settings")) {
		if (ImGui::DragFloat3("Initial Rotate", &initialRotate.x, 0.1f)) {
			if (!isPlaying && timer == 0.0f) {
				camera->SetRotate(initialRotate);   // 編集時に見た目を更新
				stageStatus.rotate = initialRotate; // 再生開始後もこの角度からスタートするようにする
			}
		}
	}

	// Reset ボタンの処理
	if (ImGui::Button("Reset")) {
		timer = 0.0f;
		isPlaying = false;
		currentRotationSpeed = {0.0f, 0.0f, 0.0f}; // 速度も初期化
		if (camera && !points.empty()) {
			camera->SetTranslate(points[0].position);
			camera->SetRotate(initialRotate); // 初期の向きに戻す
		}
	}

	ImGui::Separator();
	ImGui::Text("Rotation Settings");

	// 一時停止中のみ変更可能にする
	if (!isPlaying) {
		if (ImGui::DragFloat3("Rotation Speed Scale", &currentRotationSpeed.x, 0.01f)) {
			speedChangedDuringPause = true;
		}
	} else {
		ImGui::Text("Pause to edit rotation speed");
	}
	// --- 履歴の表示 ---
	if (ImGui::CollapsingHeader("Rotation History")) {
		for (int i = 0; i < (int)rotationHistory.size(); ++i) {
			// 各履歴項目に固有のIDを割り振る（ボタンの衝突回避）
			ImGui::PushID(i);

			// 履歴情報の表示
			ImGui::DragFloat("time", &rotationHistory[i].time, 0.05f, 0.0f, totalDuration);
			ImGui::DragFloat3("speed", &rotationHistory[i].rotationSpeed.x, 0.01f);
			ImGui::SameLine(); // 同じ行にボタンを配置
			                   // 【追加】補間時間の編集
			ImGui::SetNextItemWidth(80);
			ImGui::DragFloat("Trans(s)", &rotationHistory[i].transitionTime, 0.05f, 0.0f, 10.0f);

			ImGui::SameLine();
			// 個別削除ボタン
			if (ImGui::Button("Delete")) {
				rotationHistory.erase(rotationHistory.begin() + i);
				// 要素を削除した後はインデックスを調整
				i--;
			}

			ImGui::PopID();
		}

		if (!rotationHistory.empty()) {
			ImGui::Separator();
			if (ImGui::Button("Clear All History")) {
				rotationHistory.clear();
			}
		} else {
			ImGui::Text("No history recorded.");
		}
	}
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
		if (speedChangedDuringPause) {
			// 変更を履歴に記録
			rotationHistory.push_back({timer, currentRotationSpeed});
			// 時間順にソート（念のため）
			std::sort(rotationHistory.begin(), rotationHistory.end(), [](const RotationSpeedKey& a, const RotationSpeedKey& b) { return a.time < b.time; });

			speedChangedDuringPause = false; // 変更を適用したらフラグをリセット
		}

		SaveToJSON(GetFilePath(currentStage));
	}
	Vector3 camPos = camera ? camera->GetTranslate() : Vector3{0, 0, 0};
	ImGui::DragFloat3("camera position", &camPos.x, 0.1f);
	ImGui::End();

	// ギズモの更新
	UpdateGizmo();
#endif
}

void StageCameraController::EditorDraw() {
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
void StageCameraController::AddPoint(const Vector3& pos) {
	points.push_back({pos});
	auto obj = std::make_unique<Object>();
	obj->Initialize(camera);
	obj->SetModel("cube.obj"); // 任意のモデル名に変更してください
	obj->SetScale({0.5f, 0.5f, 0.5f});
	obj->SetTranslate(pos);
	spheres.push_back(std::move(obj));
}

void StageCameraController::SyncSpheres() {
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

void StageCameraController::InitializeRailModels(int count) {
	railModels.clear();
	for (int i = 0; i < count; i++) {
		auto obj = std::make_unique<Object>();
		obj->Initialize(camera);
		obj->SetModel("rail.obj");
		railModels.push_back(std::move(obj));
	}
}

void StageCameraController::DrawRailModels() {
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

void StageCameraController::UpdateGizmo() {
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

void StageCameraController::SelectPointByMouse() {
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
void StageCameraController::ChangeStage(int newStage) {
	currentStage = newStage;
	LoadFromJSON(GetFilePath(currentStage));
	camera->SetRotate(initialRotate); // 初期の向きにリセット

	timer = 0.0f;
	isPlaying = false;
	selectedPoint = -1;
	if (!points.empty())
		lastPosition = EvaluateBezier(0.0f);
}

std::string StageCameraController::GetFilePath(int slot) const { return "Resource/Data/camera_stage_" + std::to_string(slot) + ".json"; }

void StageCameraController::SaveToJSON(const std::string& filename) {
	json j;
	j["totalDuration"] = totalDuration;
	j["rotate"] = {
	    {"x", stageStatus.rotate.x},
        {"y", stageStatus.rotate.y},
        {"z", stageStatus.rotate.z}
    };
	j["fov"] = stageStatus.fov;

	// --- 制御点の保存 (既存) ---
	json ptsJson = json::array();
	for (const auto& p : points) {
		ptsJson.push_back({
		    {"x", p.position.x},
            {"y", p.position.y},
            {"z", p.position.z}
        });
	}
	j["controlPoints"] = ptsJson;
	j["initialRotate"] = {
	    {"x", initialRotate.x},
        {"y", initialRotate.y},
        {"z", initialRotate.z}
    };
	// --- 【追加】回転速度履歴の保存 ---
	json historyJson = json::array();
	for (const auto& log : rotationHistory) {
		historyJson.push_back({
		    {"time",       log.time		                                                                    },
            {"speed",      {{"x", log.rotationSpeed.x}, {"y", log.rotationSpeed.y}, {"z", log.rotationSpeed.z}}},
            {"transition", log.transitionTime                                                                  }
        });
	}
	j["rotationHistory"] = historyJson;

	// ファイル書き出し
	std::ofstream file(filename);
	if (file.is_open()) {
		file << j.dump(4);
	}
}
void StageCameraController::LoadFromJSON(const std::string& filename) {
	std::ifstream file(filename);

	// ファイルが開けない場合は初期化して終了
	if (!file.is_open()) {
		ResetToDefaults(); // 全てを0やデフォルト値にする補助関数（後述）
		return;
	}

	try {
		json j;
		file >> j;

		// --- 基本パラメータ (value関数で、なければ第2引数の値を代入) ---
		totalDuration = j.value("totalDuration", 0.0f);
		stageStatus.fov = j.value("fov", 0.0f);

		// --- 回転 (rotate) の読み込み ---
		if (j.contains("rotate")) {
			stageStatus.rotate.x = j["rotate"].value("x", 0.0f);
			stageStatus.rotate.y = j["rotate"].value("y", 0.0f);
			stageStatus.rotate.z = j["rotate"].value("z", 0.0f);
		} else {
			stageStatus.rotate = {0.0f, 0.0f, 0.0f};
		}

		// --- 初期回転 (initialRotate) の読み込み ---
		if (j.contains("initialRotate")) {
			initialRotate.x = j["initialRotate"].value("x", 0.0f);
			initialRotate.y = j["initialRotate"].value("y", 0.0f);
			initialRotate.z = j["initialRotate"].value("z", 0.0f);
		} else {
			initialRotate = {0.0f, 0.0f, 0.0f};
		}

		// --- 制御点 (controlPoints) の読み込み ---
		points.clear();
		if (j.contains("controlPoints") && j["controlPoints"].is_array()) {
			for (const auto& p : j["controlPoints"]) {
				points.push_back({
				    {p.value("x", 0.0f), p.value("y", 0.0f), p.value("z", 0.0f)}
                });
			}
		}

		// --- 回転速度履歴 (rotationHistory) の読み込み ---
		rotationHistory.clear();
		if (j.contains("rotationHistory") && j["rotationHistory"].is_array()) {
			for (const auto& item : j["rotationHistory"]) {
				RotationSpeedKey log;
				log.time = item.value("time", 0.0f);
				if (item.contains("speed")) {
					log.rotationSpeed.x = item["speed"].value("x", 0.0f);
					log.rotationSpeed.y = item["speed"].value("y", 0.0f);
					log.rotationSpeed.z = item["speed"].value("z", 0.0f);
				} else {
					log.rotationSpeed = {0.0f, 0.0f, 0.0f};
				}
				log.transitionTime = item.value("transition", 0.0f);
				rotationHistory.push_back(log);
			}
		}

		SyncSpheres();

	} catch (...) {
		// 解析エラーが起きても止まらないように安全策
		ResetToDefaults();
	}
}

// 安全のためにデフォルト値へリセットする関数
void StageCameraController::ResetToDefaults() {
	totalDuration = 5.0f;
	stageStatus.rotate = {0, 0, 0};
	initialRotate = {0, 0, 0};
	stageStatus.fov = 45.0f;
	points.clear();
	rotationHistory.clear();
	// 制御点がないとエラーになる場合は、ここで1点追加するなどの処理を
}
Vector3 StageCameraController::GetForward(float distance) {
	float look = 0.01f;
	Vector3 p0 = Evaluate(distance);
	Vector3 p1 = Evaluate(distance + look);
	return Normalize(p1 - p0);
}

Vector3 StageCameraController::Evaluate(float distance) {
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

Vector3 StageCameraController::CatmullRom(Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3, float t) {
	float t2 = t * t;
	float t3 = t2 * t;
	return ((p1 * 2.0f) + (-p0 + p2) * t + (p0 * 2 - p1 * 5 + p2 * 4 - p3) * t2 + (-p0 + p1 * 3 - p2 * 3 + p3) * t3) * 0.5f;
}
