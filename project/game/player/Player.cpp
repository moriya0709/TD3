#include "Player.h"
#include "../enemy/Enemy.h"
#include "PlayerChargeBullet.h"
#include "PlayerNormalBullet.h"
#include "engine/base/WindowAPI.h"
#include <algorithm>
#include <cmath>
#include <externals/nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

#include <list>

// ベクトルの回転用関数
Vector3 TransformNormal(const Vector3& v, const Matrix4x4& m) {
	Vector3 result;
	result.x = v.x * m.m[0][0] + v.y * m.m[1][0] + v.z * m.m[2][0];
	result.y = v.x * m.m[0][1] + v.y * m.m[1][1] + v.z * m.m[2][1];
	result.z = v.x * m.m[0][2] + v.y * m.m[1][2] + v.z * m.m[2][2];
	return result;
}

void to_json(nlohmann::json& j, const Player::Statas& statas) {
	j = nlohmann::json{
	    {"Style",
	     {"hp", statas.hp},
	     {"attack", statas.attack},
	     {"speed", statas.speed},
	     {"hommingAccuracy", statas.hommingAccuracy},
	     {"renge", statas.renge},
	     {"chargeTime", statas.chargeTime},
	     {"haste", statas.haste},
	     {"cosr", statas.cost}}
    };
}

void from_json(const nlohmann::json& j, Player::Statas& statas) {

	j.at("hp").get_to(statas.hp);
	j.at("attack").get_to(statas.attack);
	j.at("speed").get_to(statas.speed);
	j.at("hommingAccuracy").get_to(statas.hommingAccuracy);
	j.at("renge").get_to(statas.renge);
	j.at("chargeTime").get_to(statas.chargeTime);
	j.at("haste").get_to(statas.haste);
	j.at("cost").get_to(statas.cost);
}

std::string Player::GetFilePath() const { return "Resource/Data/playerStatas.json"; }

void Player::LoadStatas(const std::string& filePath) {

	std::ifstream inFile(filePath);
	nlohmann::json j;
	if (!inFile.is_open() || inFile.peek() == std::ifstream::traits_type::eof()) {
		j = nlohmann::json{
		    {"Statas",
		     {{{"hp", statas_[0].hp},
		       {"attack", statas_[0].attack},
		       {"chargeAttack", statas_[0].chargeAttack},
		       {"chargeSize", statas_[0].chargeSize},
		       {"speed", statas_[0].speed},
		       {"hommingAccuracy", statas_[0].hommingAccuracy},
		       {"renge", statas_[0].renge},
		       {"chargeTime", statas_[0].chargeTime},
		       {"haste", statas_[0].haste},
		       {"cost", statas_[0].cost}},
		      {{"hp", statas_[1].hp},
		       {"attack", statas_[1].attack},
		       {"chargeAttack", statas_[1].chargeAttack},
		       {"chargeSize", statas_[1].chargeSize},
		       {"speed", statas_[1].speed},
		       {"hommingAccuracy", statas_[1].hommingAccuracy},
		       {"renge", statas_[1].renge},
		       {"chargeTime", statas_[1].chargeTime},
		       {"haste", statas_[1].haste},
		       {"cost", statas_[1].cost}},
		      {{"hp", statas_[2].hp},
		       {"attack", statas_[2].attack},
		       {"chargeAttack", statas_[2].chargeAttack},
		       {"chargeSize", statas_[2].chargeSize},
		       {"speed", statas_[2].speed},
		       {"hommingAccuracy", statas_[2].hommingAccuracy},
		       {"renge", statas_[2].renge},
		       {"chargeTime", statas_[2].chargeTime},
		       {"haste", statas_[2].haste}},
		      {{"hp", statas_[3].hp},
		       {"attack", statas_[3].attack},
		       {"chargeAttack", statas_[3].chargeAttack},
		       {"chargeSize", statas_[3].chargeSize},
		       {"speed", statas_[3].speed},
		       {"hommingAccuracy", statas_[3].hommingAccuracy},
		       {"renge", statas_[3].renge},
		       {"chargeTime", statas_[3].chargeTime},
		       {"haste", statas_[3].haste},
		       {"cost", statas_[3].cost}}}}
        };
		std::ofstream outFile(filePath);
		if (!outFile.is_open()) {
			// ファイルが開けない場合のエラーハンドリング
			return;
		}
		outFile << j.dump(4); // インデントを4スペースにして保存

		return;
	}

	inFile >> j;
	for (int i = 0; i < 4; ++i) {
		statas_[i].hp = j["Statas"][i]["hp"];
		statas_[i].attack = j["Statas"][i]["attack"];
		statas_[i].chargeAttack = j["Statas"][i]["chargeAttack"];
		statas_[i].chargeSize = j["Statas"][i]["chargeSize"];
		statas_[i].speed = j["Statas"][i]["speed"];
		statas_[i].hommingAccuracy = j["Statas"][i]["hommingAccuracy"];
		statas_[i].renge = j["Statas"][i]["renge"];
		statas_[i].chargeTime = j["Statas"][i]["chargeTime"];
		statas_[i].haste = j["Statas"][i]["haste"];
		statas_[i].cost = j["Statas"][i]["cost"];
	}
}

void Player::SaveStatas(const std::string& filePath) const {
	std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());

	nlohmann::json j;
	for (int i = 0; i < 4; ++i) {
		j["Statas"][i]["hp"] = statas_[i].hp;
		j["Statas"][i]["attack"] = statas_[i].attack;
		j["Statas"][i]["chargeAttack"] = statas_[i].chargeAttack;
		j["Statas"][i]["chargeSize"] = statas_[i].chargeSize;
		j["Statas"][i]["speed"] = statas_[i].speed;
		j["Statas"][i]["hommingAccuracy"] = statas_[i].hommingAccuracy;
		j["Statas"][i]["renge"] = statas_[i].renge;
		j["Statas"][i]["chargeTime"] = statas_[i].chargeTime;
		j["Statas"][i]["haste"] = statas_[i].haste;
		j["Statas"][i]["cost"] = statas_[i].cost;
	}
	std::ofstream outFile(filePath);
	if (!outFile.is_open()) {
		// ファイルが開けない場合のエラーハンドリング
		return;
	}
	outFile << j.dump(4); // インデントを4スペースにして保存
}

void Player::Initialize(Camera* camera, Style style) {
	camera_ = camera;
	transform_.scale = {1.0f, 1.0f, 1.0f};
	transform_.rotate = {0.0f, 0.0f, 0.0f};

	// 1. 相対位置の初期化 (カメラの正面 10.0f)
	relativePos_ = {0.0f, 0.0f, 10.0f};

	// 初期状態のワールド座標を計算
	Vector3 camPos = camera_->GetTranslate();
	Matrix4x4 camRotMat = MakeRotateMatrix(camera_->GetRotate());
	Vector3 rotatedOffset = TransformNormal(relativePos_, camRotMat);

	transform_.translate.x = camPos.x + rotatedOffset.x;
	transform_.translate.y = camPos.y + rotatedOffset.y;
	transform_.translate.z = camPos.z + rotatedOffset.z;

	// Spriteの生成 (照準)
	reticle_ = std::make_unique<Sprite>();
	reticle_->Initialize("Resource/reticle/reticle.png");
	reticlePosition_ = {640.0f, 360.0f};
	reticle_->SetPosition(reticlePosition_);

	chargeReticle_ = std::make_unique<Sprite>();
	chargeReticle_->Initialize("Resource/reticle/chargeReticle.png");
	chargeReticle_->SetPosition(reticlePosition_);

	currentStyle = style;
	// モデルの生成
	playerObject_ = std::make_unique<Object>();
	playerObject_->Initialize(camera_);
	switch (style) {
	case normal:
		playerObject_->SetModel("normalMachine.obj");
		break;
	case speed:
		playerObject_->SetModel("speedMachine.obj");
		break;
	case power:
		playerObject_->SetModel("powerMachine.obj");
		break;
	case sniper:
		playerObject_->SetModel("sniperMachine.obj");
		break;
	default:
		playerObject_->SetModel("normalMachine.obj");
		break;
	}

	playerObject_->SetTranslate(transform_.translate);

	// machineObject_ = std::make_unique<Object>();
	// machineObject_->Initialize(camera_);
	// machineObject_->SetModel("playerH.obj");
	// machineObject_->SetTranslate(transform_.translate);

	statas_[currentStyle].hp = 100;
	statas_[currentStyle].attack = 20;
	statas_[currentStyle].speed = 0.2f;
	statas_[currentStyle].haste = 10;
	statas_[currentStyle].chargeTime = 60;
	statas_[currentStyle].hommingAccuracy = 0.01f;
	statas_[currentStyle].renge = 80.0f;

	LoadStatas(GetFilePath());
	specialAttackCounter = specialAttackMaxCounter;

	velocity_ = {0.0f, 0.0f, 0.0f};
	coolTime = 0;
	chargeTimer = 0;
	isCharging = false;
	ishit = false;
	damageTimer = 0;
}

// 攻撃モーション
void Player::UpdateAttackMove() {
	if (0 < coolTime) {
		if (maxHaste / 2 < coolTime) {
			attackRotate += 0.01f;
		} else {
			attackRotate -= 0.01f;
		}
	} else {
		attackRotate = 0;
	}
}

void Player::Update(const std::list<std::shared_ptr<Enemy>>& enemies, float cmrvel) {
	UpdateAttackMove();
	InputMove();
	chargeReticle_->SetPosition(reticlePosition_);
	chargeReticle_->Update();
	reticle_->SetPosition(reticlePosition_);
	reticle_->Update();
	Attack(enemies);
	UpdateBullets(cmrvel);
	if (inbincileTimer > 0) {
		inbincileTimer--;
	}
	UpdateImGui();
}

void Player::Draw2D() {
	if (isCharging) {
		chargeReticle_->Draw();
	} else {

		reticle_->Draw();
	}
}

void Player::Draw3D() {
	for (const auto& bullet : bullets) {
		bullet->Draw3D();
	}
	playerObject_->Draw();
	// machineObject_->Draw();
}

Player::~Player() {
	for (auto& bullet : bullets) {
	}
	bullets.clear();
}

void Player::Attack(const std::list<std::shared_ptr<Enemy>>& enemies) {
	auto input = Input::GetInstance();
	if (coolTime > 0) {
		coolTime--;
		return;
	}
	if (statas_[currentStyle].chargeTime < chargeTimer) {
		isCharging = true;
	} else {
		isCharging = false;
		chargeTimer++;
	}
	if (specialAttackCounter > 0) {
		if (input->TriggerKey(DIK_SPACE) || input->IsPadButtonPressed(0, 6)) {
			isSpecialAttack = true;
			coolTime = 60;
			specialAttackCounter--;
		}
	}

	if (input->IsMouseButtonPressed(0) || input->IsPadButtonPressed(0, 5)) {
		if (isCharging) {
			// チャージ攻撃
			std::unique_ptr<PlayerBullet> newBullet = std::make_unique<PlayerChargeBullet>();
			newBullet->Initialize(transform_.translate, camera_, reticlePosition_, statas_[currentStyle].renge * 1.5f, enemies, currentStyle);
			newBullet->SetStatus(statas_[currentStyle].hommingAccuracy + 0.2f, statas_[currentStyle].chargeAttack);
			newBullet->SetSize(1.5f); // チャージ弾のサイズを設定
			bullets.push_back(std::move(newBullet));
			chargeTimer = 0;                            // チャージタイマーリセット
			coolTime = statas_[currentStyle].haste * 2; // チャージ攻撃後のクールタイムも長くする
			maxHaste = statas_[currentStyle].haste * 2;
		} else {
			std::unique_ptr<PlayerBullet> newBullet = std::make_unique<PlayerNormalBullet>();
			newBullet->Initialize(transform_.translate, camera_, reticlePosition_, statas_[currentStyle].renge, enemies,currentStyle);
			newBullet->SetStatus(statas_[currentStyle].hommingAccuracy, statas_[currentStyle].attack);
			bullets.push_back(std::move(newBullet));
			coolTime = statas_[currentStyle].haste;
			maxHaste = statas_[currentStyle].haste;

			chargeTimer = 0; // チャージタイマーリセット
		}
	}
}

void Player::UpdateBullets(float cmrvel) {
	for (auto it = bullets.begin(); it != bullets.end();) {
		if (!(*it)->IsActive()) {
			it = bullets.erase(it);
		} else {
			(*it)->Update(cmrvel);
			++it;
		}
	}
}

void Player::InputMove() {
	auto input = Input::GetInstance();

	camera_->Update();
	if (cameraFollow) {
		camPos = camera_->GetTranslate();
	}
	Matrix4x4 camRotMat = MakeRotateMatrix(camera_->GetRotate());
	// 1. 入力からローカル移動方向(XY)を決定
	Vector3 moveInput = {0.0f, 0.0f, 0.0f};
	if (input->PushKey(DIK_W)) {
		moveInput.y += 1.0f;
	}
	if (input->PushKey(DIK_S)) {
		moveInput.y -= 1.0f;
	}
	if (input->PushKey(DIK_A)) {
		moveInput.x -= 1.0f;
	}
	if (input->PushKey(DIK_D)) {
		moveInput.x += 1.0f;
	}
	movePad.x = (input->GetPadLeftAxisX(0));
	if (movePad.x != 0) {
		moveInput.x += float(input->GetPadLeftAxisX(0) / 32768.0); // ゲームパッドの入力を-1.0f～1.0fに正規化
	}
	movePad.y = (input->GetPadLeftAxisY(0));

	if (input->GetPadLeftAxisY(0) != 0) {
		moveInput.y -= float(input->GetPadLeftAxisY(0) / 32768.0); // ゲームパッドの入力を-1.0f～1.0fに正規化
	}

	// 2. 正規化
	float length = std::sqrt(moveInput.x * moveInput.x + moveInput.y * moveInput.y);
	Vector3 targetVelocity = {0.0f, 0.0f, 0.0f};
	if (length > 0.0f) {
		targetVelocity.x = (moveInput.x / length) * statas_[currentStyle].speed;
		targetVelocity.y = (moveInput.y / length) * statas_[currentStyle].speed;
	}

	// 3. 慣性適用
	float lerpFactor = 0.12f;
	velocity_.x += (targetVelocity.x - velocity_.x) * lerpFactor;
	velocity_.y += (targetVelocity.y - velocity_.y) * lerpFactor;

	// 4. 相対座標の更新
	relativePos_.x += velocity_.x;
	relativePos_.y += velocity_.y;

	// 5. 【重要】カメラの視界（Frustum）に合わせた制限の計算
	// 一般的な画角(45度)とアスペクト比(16:9)の場合
	float fovY = 0.45f; // 垂直画角（ラジアン）
	float aspect = (float)WindowAPI::kClientWidth / (float)WindowAPI::kClientHeight;

	// 距離(relativePos_.z)に応じて、画面内に収まる限界値を計算
	// 高さ = 距離 * tan(画角/2)
	float verticalLimit = relativePos_.z * std::tan(fovY / 2.0f);
	float horizontalLimit = verticalLimit * aspect;

	// 自機が少し画面内に残るようにマージンを引く
	float margin = 0.8f;
	float finalLimitX = horizontalLimit - margin;
	float finalLimitY = verticalLimit - margin;

	relativePos_.x = std::clamp(relativePos_.x, -finalLimitX, finalLimitX);
	relativePos_.y = std::clamp(relativePos_.y, -finalLimitY, finalLimitY);

	// 6. ワールド座標に変換
	Vector3 rotatedOffset = TransformNormal(relativePos_, camRotMat);
	transform_.translate.x = camPos.x + rotatedOffset.x;
	transform_.translate.y = camPos.y + rotatedOffset.y;
	transform_.translate.z = camPos.z + rotatedOffset.z;

	// 7. 見た目の回転
	float speedSq = velocity_.x * velocity_.x + velocity_.y * velocity_.y;
	if (speedSq > 0.001f) {
		transform_.rotate.z = -velocity_.x * 2.0f;
		transform_.rotate.x = velocity_.y * 1.0f + attackRotate;
	} else {
		transform_.rotate.x = attackRotate;
	}
	transform_.rotate.y = camera_->GetRotate().y;

#pragma endregion

#pragma region 各種更新
	playerObject_->SetTranslate(transform_.translate);
	playerObject_->SetRotate(transform_.rotate);
	playerObject_->SetScale(transform_.scale);
	playerObject_->Update();

	// machineObject_->SetTranslate(transform_.translate);
	// machineObject_->SetRotate(transform_.rotate);
	// machineObject_->SetScale(transform_.scale);
	// machineObject_->Update();

	if (mouseMove.x == input->GetMouseScreen().x && mouseMove.y == input->GetMouseScreen().y) {
		reticlePad.x = (input->GetPadRightAxisX(0));
		reticlePad.y = (input->GetPadRightAxisY(0));
		reticlePosition_.x += reticlePad.x * reticleSpeed;
		reticlePosition_.y += reticlePad.y * reticleSpeed; // Y軸は上下逆なので減算
		if (reticlePosition_.x < 0) {
			reticlePosition_.x = 0;
		}
		if (reticlePosition_.x > WindowAPI::kClientWidth) {
			reticlePosition_.x = WindowAPI::kClientWidth;
		}
		if (reticlePosition_.y < 0) {
			reticlePosition_.y = 0;
		}
		if (reticlePosition_.y > WindowAPI::kClientHeight) {
			reticlePosition_.y = WindowAPI::kClientHeight;
		}

	} else {
		mouseMove = input->GetMouseScreen();
		reticlePosition_.x = std::clamp(mouseMove.x, 0.0f, float(WindowAPI::kClientWidth));
		reticlePosition_.y = std::clamp(mouseMove.y, 0.0f, float(WindowAPI::kClientHeight));
	}
}

void Player::UpdateImGui() {
#pragma region ImGui
#ifdef USE_IMGUI

	ImGui::Begin("Player Config");
	ImGui::Checkbox("Camera Follow", &cameraFollow);
	ImGui::Text("Style: %d", currentStyle);
	for (int i = 0; i < 4; i++) {
		if (ImGui::RadioButton(("Style " + std::to_string(i)).c_str(), currentStyle == i)) {
			currentStyle = Style(i);
		}
	}
	ImGui::Text("Z-Distance from Camera: 5.0 (Fixed)");
	ImGui::DragFloat3("World Pos", &transform_.translate.x, 0.1f);
	ImGui::DragFloat2("Relative Pos", &relativePos_.x, 0.1f);
	ImGui::DragInt("HP", &statas_[currentStyle].hp, 0.1f);
	ImGui::DragInt("Attack", &statas_[currentStyle].attack, 0.1f);
	ImGui::DragInt("Charge Attack", &statas_[currentStyle].chargeAttack, 0.1f);
	ImGui::DragInt("Charge Size", &statas_[currentStyle].chargeSize, 0.1f);
	ImGui::DragFloat("Speed", &statas_[currentStyle].speed, 0.01f);
	ImGui::DragFloat("Homing Accuracy", &statas_[currentStyle].hommingAccuracy, 0.0001f, 0.0f, 1.0f, "%.4f");
	ImGui::DragFloat("Reticle Speed", &reticleSpeed, 0.1f);
	ImGui::DragFloat("Renge", &statas_[currentStyle].renge, 0.1f);
	ImGui::DragInt("Charge Time", &statas_[currentStyle].chargeTime, 1);
	ImGui::DragInt("Haste", &statas_[currentStyle].haste, 1);
	ImGui::DragInt("Cost", &statas_[currentStyle].cost, 1);
	if (ImGui::Button("■ SaveStatas", ImVec2(240, 30))) {

		ImGui::DragFloat2("Move Pad", &movePad.x, 0.0f);
		ImGui::DragFloat2("Reticle Pad", &reticlePad.x, 0.0f);
		SaveStatas(GetFilePath());
	}

	ImGui::End();
	switch (currentStyle) {
	case normal:
		playerObject_->SetModel("normalMachine.obj");
		break;
	case speed:
		playerObject_->SetModel("speedMachine.obj");
		break;
	case power:
		playerObject_->SetModel("powerMachine.obj");
		break;
	case sniper:
		playerObject_->SetModel("sniperMachine.obj");
		break;
	default:
		playerObject_->SetModel("normalMachine.obj");
		break;
	}

#endif
#pragma endregion
}

void Player::StyleLevelUp(Style style, int statas) {}
