#pragma once
#include "BaseScene.h"
#include "Camera.h"
#include "CameraManager.h"
#include "ImGuiManager.h"
#include "Input.h"
#include "ModelManager.h"
#include "Object.h"
#include "ParticleEmitter.h"
#include "ParticleManager.h"
#include "PostEffect.h"
#include "SoundManager.h"
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include "Sprite.h"

// カメラの状態（速度）を記録する構造体
struct CameraState {
	float time;              // 変化が起きた時間
	Vector3 velocity;        // 移動速度 (x, y, z)
	Vector3 angularVelocity; // 回転速度 (rx, ry, rz)
};

class CameraController {
public:
	/// <summary>
	/// 初期化：カメラの登録と、既存リプレイファイルの自動読み込み
	/// </summary>
	void Initialize(Camera* camera);

	/// <summary>
	/// 毎フレーム更新：通常操作またはリプレイの実行
	/// </summary>
	void Update();

private:
	// --- 内部処理メソッド ---

	// 通常時の入力処理と速度計算
	void CalculateVelocityFromInput(Vector3& vel, Vector3& angVel);

	// 速度に変化があった場合のみ履歴に記録
	void RecordStateIfChanged(const Vector3& vel, const Vector3& angVel);

	// リプレイ時の速度適用
	void ApplyReplayState(Vector3& vel, Vector3& angVel);

	// 物理移動（速度を座標に反映）
	void ApplyPhysics(const Vector3& vel, const Vector3& angVel);

	// リプレイ開始処理
	void StartReplay();

	// JSON保存・読み込み
	void SaveToJSON(const std::string& filename);
	void LoadFromJSON(const std::string& filename);

private:
	// --- メンバ変数 ---

	Camera* camera = nullptr;

	// 現在のトランスフォーム（計算用）
	Transform cameraTransform = {
	    {0, 0, 0},
        {0, 0, 0}
    };
	// 録画開始時のトランスフォーム（リプレイ開始地点）
	Transform initialTransform = {
	    {0, 0, 0},
        {0, 0, 0}
    };

	// 履歴データ
	std::vector<CameraState> stateHistory;

	// リプレイ制御用
	float timer = 0.0f;
	bool isReplaying = false;
	size_t replayIndex = 0;

	// リプレイ中に現在適用されている速度
	Vector3 activeVelocity = {0, 0, 0};
	Vector3 activeAngularVelocity = {0, 0, 0};

	// キーのトリガー判定用
	bool lastRKey = false;

	// 変化検知用の前フレーム速度
	Vector3 lastRecordedVel = {-1.0f, -1.0f, -1.0f};
	Vector3 lastRecordedAngVel = {-1.0f, -1.0f, -1.0f};
};