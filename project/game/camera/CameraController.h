#pragma once
#include "Camera.h"
#include "Input.h"
#include <string>
#include <vector>

// 録画データの一コマ
struct CameraState {
	float time;
	Vector3 velocity;
	Vector3 angularVelocity;
	Vector3 position;
	Vector3 rotation;
	float fov; // ★ 追加：ズーム（画角）
};

// カメラのトランスフォーム（FOV込み）
struct CameraTransform {
	Vector3 translate;
	Vector3 rotate;
	float fov;
};

class CameraController {
public:
	void Initialize(Camera* camera);
	void Update(bool isActive,int bossType);
	void DrawImGui();
	void DrawDebugTrace();
	void StartReplay();
	Vector3 GetVelocity() const { return uiVelocity; }

	float GetCurrentReplayTime() const { return timer; }
	int GetCurrentStage() const { return currentStage; }
	void SetCurrentStage(int stage) { currentStage = stage; }

private:
	// 補間ヘルパー
	Vector3 CameraLerp(const Vector3& start, const Vector3& end, float t);
	Vector3 CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t);
	float Lerp(float a, float b, float t);                              // ★ 追加
	float CatmullRomF(float v0, float v1, float v2, float v3, float t); // ★ 追加

	void RecordStateIfChanged(const Vector3& vel, const Vector3& angVel, const Vector3& pos, const Vector3& rat, float fov);
	void ApplyReplayState(Vector3& vel, Vector3& angVel, Vector3& pos, Vector3& rat, float& fov);
	void ApplyPhysics(const Vector3& vel, const Vector3& angVel, const Vector3& pos, const Vector3& rat, float fov);

	void SeekTo(float targetTime);
	void StartOverwriteRecording(); // パンチイン録画

	void SaveToJSON(const std::string& filename);
	void LoadFromJSON(const std::string& filename);
	std::string GetFilePath(int slot) const;

private:
	Camera* camera = nullptr;
	CameraTransform cameraTransform = {
	    {0, 0, 0},
        {0, 0, 0},
        45.0f
    };
	CameraTransform initialTransform = {
	    {0, 0, 0},
        {0, 0, 0},
        45.0f
    };
	// --- 定数設定 ---
	const float kMaxDuration = 180.0f;
	const float kMinFov = 1.0f;   // これ未満だとズームしすぎて破綻する
	const float kMaxFov = 50.0f; // 180度を超えると画面が反転するため制限
	std::vector<CameraState> stateHistory;

	float timer = 0.0f;
	bool isReplaying = false;
	bool isPaused = false;
	bool isRecording = false;
	bool isSmoothMode = true;
	bool showDebugTrace = true;

	int currentStage = 1;
	float playbackSpeed = 1.0f;

	// UI操作用
	Vector3 uiVelocity = {0.0f, 0.0f, 0.0f};
	Vector3 uiAngularVelocity = {0.0f, 0.0f, 0.0f};
	float uiFov = 45.0f; // ★ 追加

	// 変化検知用
	Vector3 lastRecordedVel = {-999.0f, -999.0f, -999.0f};
	Vector3 lastRecordedAngVel = {-999.0f, -999.0f, -999.0f};
	Vector3 lastRecordedPos = {-999.0f, -999.0f, -999.0f};
	Vector3 lastRecordedRot = {-999.0f, -999.0f, -999.0f};
	float lastRecordedFov = -999.0f; // ★ 追加

	void GreapesCameraUpdate();
	void bananaCameraUpdate();

};


// --- 以下はRailCameraクラスのコード ---

//// カメラの更新
//void RailCamera::Update() {
//	// カメラ更新
//	CameraManager::GetInstance()->Update();
//	if (isRail) {
//		float tEnd = (float)(points.size() - 2);
//
//		// tを進める
//		railT += railSpeed * deltaTime;
//
//		// 終端クランプ
//		if (railT >= tEnd) {
//			railT = tEnd - 0.001f;
//		}
//
//		// カメラ位置を更新
//		cameraTransform.translate = Evaluate(railT);
//		camera->SetTranslate(cameraTransform.translate);
//
//		// 進行方向を向かせる
//		Vector3 forward = GetForward(railT);
//		// forwardからオイラー角を計算
//		float yaw = atan2f(forward.x, forward.z);
//		float pitch = atan2f(-forward.y, sqrtf(forward.x * forward.x + forward.z * forward.z));
//		cameraTransform.rotate = { pitch, yaw ,0.0f };
//		// カメラ回転を更新
//		camera->SetRotate(cameraTransform.rotate);
//	}
//}
//
//// エディタ用の更新（ImGuiで点の追加や編集）
//void RailCamera::EditorUpdate() {
//#ifdef USE_IMGUI 
//	if (ImGui::Begin("RailEditor")) {
//		if (ImGui::Button("Add Point")) {
//			AddPoint(camera->GetTranslate());
//		}
//		ImGui::Text("PointCount : %d", (int)points.size());
//
//		// 選択中の点の情報表示
//		if (selectedPoint >= 0 && selectedPoint < (int)points.size()) {
//			ImGui::Separator();
//			ImGui::Text("Selected: Point[%d]", selectedPoint);
//			ImGui::DragFloat3("Position", &points[selectedPoint].position.x, 0.1f);
//			if (ImGui::Button("Delete Point")) {
//				points.erase(points.begin() + selectedPoint);
//				spheres.erase(spheres.begin() + selectedPoint);
//				selectedPoint = -1;
//			}
//		}
//
//		ImGui::DragFloat("time", &railT, 0.01f);
//		ImGui::Checkbox("isRail", &isRail);
//		// カメラ
//		ImGui::DragFloat3("cameraTranslate", &cameraTransform.translate.x, 0.01f, -1000.0f, 1000.0f);
//		ImGui::DragFloat3("cameraRotate", &cameraTransform.rotate.x, 0.01f, -180.0f, 180.0f);
//		camera->SetTranslate({ cameraTransform.translate });
//		camera->SetRotate({ cameraTransform.rotate });
//
//	}
//
//	ImGui::End();
//
//	// ギズモ更新(BlenderやUnityのように制御点を選択して移動できる)
//	UpdateGizmo();
//#endif
//}
//
//// レール上の位置からカメラの位置を計算
//Vector3 RailCamera::Evaluate(float distance) {
//	if (points.size() < 4) {
//		return Vector3{ 0,0,0 };
//	}
//	float maxT = (float)(points.size() - 3);
//	// distance制限 
//	if (distance < 0.0f) distance = 0.0f;
//	if (distance > maxT - 0.001f) distance = maxT - 0.001f;
//	int i = (int)distance;
//	float localT = distance - i;
//	Vector3 p0 = points[i].position;
//	Vector3 p1 = points[i + 1].position;
//	Vector3 p2 = points[i + 2].position;
//	Vector3 p3 = points[i + 3].position;
//	return CatmullRom(p0, p1, p2, p3, localT);
//}
//
//// ギズモ更新(BlenderやUnityのように制御点を選択して移動できる)無くてもいいけどあると便利
//void RailCamera::UpdateGizmo() {
//	if (selectedPoint < 0 || selectedPoint >= (int)points.size()) return;
//
//	// ビュー・プロジェクション行列をカメラから取得
//	Matrix4x4 view = camera->GetViewMatrix();
//	Matrix4x4 proj = camera->GetProjectionMatrix();
//
//	// 選択点のワールド行列（平行移動のみ）
//	Matrix4x4 world = MakeTranslateMatrix(points[selectedPoint].position);
//
//#ifdef USE_IMGUI
//
//	ImGuizmo::SetOrthographic(false);
//	ImGuizmo::BeginFrame();
//
//	// ウィンドウ全体をギズモの描画範囲に
//	ImGuizmo::SetRect(0, 0,
//		(float)windowAPI_->kClientWidth,
//		(float)windowAPI_->kClientHeight);
//
//	ImGuizmo::Manipulate(
//		&view.m[0][0],   // ビュー行列（row-major float[16]）
//		&proj.m[0][0],   // プロジェクション行列
//		ImGuizmo::TRANSLATE,  // 移動ギズモ
//		ImGuizmo::WORLD,      // ワールド空間
//		&world.m[0][0]        // 操作対象のワールド行列
//	);
//
//	// 操作後に位置を反映
//	if (ImGuizmo::IsUsing()) {
//		points[selectedPoint].position = {
//			world.m[3][0],
//			world.m[3][1],
//			world.m[3][2]
//		};
//	}
//
//#endif 
//}