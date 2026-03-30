#include "StageSelect.h"
#include "ObjectCommon.h"
#include "SceneManager.h"
#include "SpriteCommon.h"


#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxcompiler.lib")

void StageSelect::Initialize()
{
	// カメラ初期化
	camera = std::make_unique<Camera>();
	camera->SetRotate({ cameraTransform.rotate });
	camera->SetTranslate({ cameraTransform.translate });

	// カメラマネージャ登録
	CameraManager::GetInstance()->AddCamera("main", camera.get());
	CameraManager::GetInstance()->SetActiveCamera("main");

	// スプライト
	sprite = std::make_unique <Sprite>();
	sprite->Initialize("Resource/monsterBall.png");

	// 3Dオブジェクト
	for (int i = 0; i < 2; i++) {
		object[i] = std::make_unique <Object>();
		object[i]->Initialize(camera.get());
	}

	// 初期化済みの3Dオブジェクトにモデルを紐づける
	object[0]->SetModel("emission.obj");
	object[1]->SetModel("skydome.obj");

	currentStyle = style;

	playerObject_ = std::make_unique<Object>();
	playerObject_->Initialize(camera_);
	switch (style) {
	case Player::normal:
		playerObject_->SetModel("normalMachine.obj");
		break;
	case Player::speed:
		playerObject_->SetModel("speedMachine.obj");
		break;
	case Player::power:
		playerObject_->SetModel("powerMachine.obj");
		break;
	case Player::sniper:
		playerObject_->SetModel("sniperMachine.obj");
		break;
	default:
		playerObject_->SetModel("normalMachine.obj");
		break;
	}

	playerObject_->SetTranslate(transform_.translate);

}

void StageSelect::Update()
{
	// 入力取得
	auto input = Input::GetInstance();
	// カメラ更新
	CameraManager::GetInstance()->Update();



	// ENTERキーを押したら
	if (input->TriggerKey(DIK_RETURN)) {
		// ゲームプレイシーン(次シーン)を生成
		SceneManager::GetInstance()->ChangeScene("GAMEPLAY");
		// 音声再生
		SoundManager::GetInstance()->Stop("bgm");
	}

	// * 3Dオブジェクト* //
	for (int i = 0; i < 2; i++) {
		object[i]->Update();
	}

	sprite->Update();

}
void StageSelect::Draw2D()
{
	// 2Dオブジェクトの描画準備
	SpriteCommon::GetInstance()->SetCommonPipelineState();
}

void StageSelect::Draw3D()
{
	// 3Dオブジェクトの描画準備
	ObjectCommon::GetInstance()->SetCommonPipelineState();


	// 3Dオブジェクト描画
	for (int i = 0; i < 2; i++) {
		object[i]->Draw();
	}

}

void StageSelect::Finalize()
{
	CameraManager::GetInstance()->RemoveCamera("main");
}