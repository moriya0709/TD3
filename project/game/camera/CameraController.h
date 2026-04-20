#pragma once
#define NOMINMAX

#include "Calc.h" // VectorTransform, RaySphereIntersect, Inverse, MakeTranslateMatrix, Normalize など
#include "Camera.h"
#include "Object.h"
#include <memory>
#include <string>
#include <vector>

class DirectXCommon;
class WindowAPI;

// 制御点の構造体
struct ControlPoint {
	Vector3 position;
};

class CameraController {
public:
	void Initialize(Camera* camera);
	void Update();
	void EditorUpdate();
	void EditorDraw();

	// --- ゲッター ---
	float GetElapsedTime() const { return timer; }
	float GetTotalDuration() const { return totalDuration; }
	float GetSpeed() const { return currentSpeed; }
	int GetCurrentStage() const { return currentStage; }

	void SetCurrentStage(int stage) { ChangeStage(stage); }
	void StartReplay() {
		timer = 0.0f;
		isPlaying = true;
	}

private:
	// ベジェ曲線計算
	Vector3 EvaluateBezier(float t);
	double BinomialCoefficient(int n, int k);

	// データ保存・読み込み
	void SaveToJSON(const std::string& filename);
	void LoadFromJSON(const std::string& filename);
	Vector3 GetForward(float distance);
	Vector3 Evaluate(float distance);
	Vector3 CatmullRom(Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3, float t);
	std::string GetFilePath(int slot) const;
	void ChangeStage(int newStage);

	// エディタ・描画用ヘルパー
	void AddPoint(const Vector3& pos);
	void SyncSpheres(); // 点の数とスフィアのオブジェクトを同期
	void InitializeRailModels(int count);
	void DrawRailModels();
	void UpdateGizmo();
	void SelectPointByMouse();

private:
	Camera* camera = nullptr;

	// --- ステージ依存データ ---
	std::vector<ControlPoint> points;
	float totalDuration = 5.0f;
	struct {
		Vector3 rotate = {0, 0, 0};
		float fov = 45.0f;
	} stageStatus;

	// --- 実行時データ ---
	float timer = 0.0f;
	float currentSpeed = 0.0f;
	Vector3 lastPosition;
	bool isPlaying = false;
	int selectedPoint = -1;
	int currentStage = 1;

	const float kMinFov = 1.0f;
	const float kMaxFov = 170.0f;

	// --- 描画・エディタ操作用オブジェクト ---
	std::vector<std::unique_ptr<Object>> spheres;
	std::vector<std::unique_ptr<Object>> railModels;

	DirectXCommon* dxCommon_ = nullptr;
	std::unique_ptr<WindowAPI> windowAPI_ = nullptr;
};