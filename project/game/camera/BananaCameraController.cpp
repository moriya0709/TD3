#include "BananaCameraController.h"
#include <cmath> // sin, cos, atan2, sqrt を使うために必要です

void BananaCameraController::Initialize(Camera* camera) { pCamera = camera; }

void BananaCameraController::Update() {
	if (pCamera == nullptr)
		return;

	// ==========================================
	// 1. カメラの新しい位置を計算する（前回と同じ）
	// ==========================================
	float deltaTime = 1.0f / 60.0f; // 1フレームの時間（秒）。実際のフレームレートに合わせて調整してください。
	timer += deltaTime;
	float angle = timer * rotationSpeed; // 時間に基づいて回転角度を計算します

	float cameraX = bananaPosition.x + (radius * std::sin(angle));
	float cameraZ = bananaPosition.z + (radius * std::cos(angle));
	float cameraY = bananaPosition.y + cameraHeight;

	Vector3 newCameraPos = {cameraX, cameraY, cameraZ};
	pCamera->SetTranslate(newCameraPos);

	// ==========================================
	// 2. バナナの方向を向くための回転角度を計算する
	// ==========================================

	// カメラからバナナへの「方向（距離の差）」を計算します
	float dirX = bananaPosition.x - cameraX;
	float dirY = bananaPosition.y - cameraY;
	float dirZ = bananaPosition.z - cameraZ;

	// [Y軸の回転 (左右: Yaw)]
	// XとZの方向から、水平方向の角度を計算します
	float rotY = std::atan2(dirX, dirZ);

	// [X軸の回転 (上下: Pitch)]
	// まず、XとZの平面上の直線距離（水平距離）を三平方の定理(ピタゴラスの定理)で求めます
	float horizontalDistance = std::sqrt((dirX * dirX) + (dirZ * dirZ));

	// 高さの差(dirY)と水平距離から、見下ろす/見上げる角度を計算します
	// ※プロジェクトの座標系（上方向が+か-か等）によって、dirY にマイナスをつける場合があります
	float rotX = 0.0f;

	// [Z軸の回転 (傾き: Roll)]
	// カメラを左右に傾けることはしないので0にします
	float rotZ = 0.0f;

	// ==========================================
	// 3. 計算した角度をカメラにセットする
	// ==========================================
	Vector3 newCameraRot = {rotX, rotY, rotZ};
	pCamera->SetRotate(newCameraRot);
}

void BananaCameraController::EditorUpdate() {}
void BananaCameraController::EditorDraw() {}
void BananaCameraController::ChangeStage(int stage) {}