#include "EnemyManager.h"
#include <algorithm>

#include <fstream>
#include <iostream> // エラー出力用

#include "../camera/CameraController.h"
#include "type/HomingEnemy.h"
#include "type/NormalEnemy.h"
#include "type/TargetEnemy.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

void EnemyManager::Initialize(Player* player, Camera* camera, CameraController* cameraController)
{
    player_ = player;
    camera_ = camera;
    cameraContrroller_ = cameraController;
    currentTimer_ = 0.0f;
    precurrenTimer = 0.0f;
    currentSpawnIndex_ = 0;

    currentLoadedStage_ = cameraContrroller_->GetCurrentStage();
    jsonFilePath_ = GetJsonPath(currentLoadedStage_);
    targetEditStage_ = currentLoadedStage_;

    LoadEnemyData(jsonFilePath_);
}

void EnemyManager::Update()
{

    // カメラ側のステージが変わったかどうかのチェック
    int cameraStage = cameraContrroller_->GetCurrentStage();

    // 編集モードでなく、かつカメラのステージが進行した場合、自動で次のJSONを読み込む
    if (!isEditing_ && currentLoadedStage_ != cameraStage) {
        currentLoadedStage_ = cameraStage;
        jsonFilePath_ = GetJsonPath(currentLoadedStage_);
        targetEditStage_ = currentLoadedStage_;

        std::cout << "Stage changed! Loading: " << jsonFilePath_ << std::endl;
        LoadEnemyData(jsonFilePath_);
    }

    // 更新処理

    if (currentTimer_ == precurrenTimer) {
        return; // タイマーが動いていないから早期リターン
    }

    if (!isEditing_ && std::filesystem::exists(jsonFilePath_)) { // 編集モードでなければチェック
        try {
            // ファイルの最終更新日時を取得
            auto currentWriteTime = fs::last_write_time(jsonFilePath_);

            // 最後に読み込んだ時より新しければリロード
            if (currentWriteTime > lastWriteTime_) {
                std::cout << "JSON modified. Hot-reloading: " << jsonFilePath_ << std::endl;
                LoadEnemyData(jsonFilePath_);
            }
        } catch (const fs::filesystem_error& e) {
            // ファイルが他のプロセスで使用中などのエラー対策
            std::cerr << "Filesystem error: " << e.what() << std::endl;
        }
    }

    while (currentSpawnIndex_ < popDatas_.size() && popDatas_[currentSpawnIndex_].popTime <= currentTimer_) {
        SpawnEnemy(popDatas_[currentSpawnIndex_]);
        currentSpawnIndex_++;
    }

    for (auto& enemy : enemies_) {
        enemy->Update();
    }

    // --- 修正ポイント：引数を std::shared_ptr に変更 ---
    enemies_.remove_if([](const std::shared_ptr<Enemy>& enemy) { return enemy->GetIsDead(); });
}
void EnemyManager::Draw3D()
{
    for (auto& enemy : enemies_) {
        enemy->Draw3D();
    }
}

void EnemyManager::SetcurrentTimer_(float timer)
{
    precurrenTimer = currentTimer_;

    if (currentTimer_ > timer) {
        currentSpawnIndex_ = 0;
        enemies_.remove_if([](const std::shared_ptr<Enemy>& enemy) { return true; });
    }
    currentTimer_ = timer;
}

void EnemyManager::LoadEnemyData(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        return;

    json data;
    try {
        file >> data;
        // 読み込みに成功したら、最終更新日時を更新
        lastWriteTime_ = fs::last_write_time(filePath);
    } catch (const json::parse_error& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
        return;
    }

    popDatas_.clear();

    if (data.contains("enemies") && data["enemies"].is_array()) {
        for (const auto& enemyData : data["enemies"]) {
            EnemyPopData data;

            // --- 安全なデータ取得 ---
            // .value("キー名", デフォルト値) を使うことで、
            // JSONに書き忘れていてもゲームが落ちずにデフォルト値が入ります。

            data.popTime = enemyData.value("popTime", 0.0f);
            data.type = enemyData.value("type", "NormalEnemy");
            data.hp = enemyData.value("hp", 10);

            // 座標(Vector3)の読み込み（ネストされている場合）
            if (enemyData.contains("position")) {
                const auto& posData = enemyData["position"];
                data.position.x = posData.value("x", 0.0f);
                data.position.y = posData.value("y", 0.0f);
                data.position.z = posData.value("z", 0.0f);
            } else {
                // positionタグ自体が無い場合は原点にする
                data.position = { 0.0f, 0.0f, 0.0f };
            }

            if (enemyData.contains("movePattern") && enemyData["movePattern"].is_array()) {

                for (const auto& wpData : enemyData["movePattern"]) {
                    WayPoint wp;

                    // 到達時間の取得（書き忘れ対策でデフォルト1.0秒）
                    wp.timeToReach = wpData.value("timeToReach", 1.0f);
                    wp.timeToStop = wpData.value("timeToStop", 1.0f);

                    // 目標座標の取得
                    if (wpData.contains("target")) {
                        const auto& tData = wpData["target"];
                        wp.target.x = tData.value("x", 0.0f);
                        wp.target.y = tData.value("y", 0.0f);
                        wp.target.z = tData.value("z", 0.0f);
                    } else {
                        // 座標がなければ原点をセット
                        wp.target = { 0.0f, 0.0f, 0.0f };
                    }

                    // 経路リストに追加
                    data.movePattern.push_back(wp);
                }
            }

            if (enemyData.contains("flee")) {
                const auto& fleeData = enemyData["flee"];

                data.fleeWaypoint.timeToReach = fleeData.value("timeToReach", 2.0f);
                data.fleeWaypoint.timeToStop = 0.0f; // 逃走時間に停止はない

                if (fleeData.contains("target")) {
                    const auto& tData = fleeData["target"];
                    data.fleeWaypoint.target.x = tData.value("x", 0.0f);
                    data.fleeWaypoint.target.y = tData.value("y", 0.0f);
                    data.fleeWaypoint.target.z = tData.value("z", 0.0f);
                } else {
                    data.fleeWaypoint.target = { 0.0f, 0.0f, 0.0f };
                }
                data.hasFleeData = true; // 逃走データあり
            } else {
                data.hasFleeData = false; // 逃走データなし（後でデフォルトの逃げ方をする）
            }

            // 取得したデータをリストに追加
            popDatas_.push_back(data);
        }
    }

    // sort(ぐちゃぐちゃになっているやつを整理)
    std::sort(popDatas_.begin(), popDatas_.end(), [](const EnemyPopData& a, const EnemyPopData& b) {
        return a.popTime < b.popTime;
    });

    // 編集用データを更新
    editingPopDatas_ = popDatas_;
}

void EnemyManager::SpawnEnemy(const EnemyPopData& data)
{
    // unique_ptr で生成する（
    std::unique_ptr<Enemy> newEnemy = nullptr;

    if (data.type == "NormalEnemy") {
        newEnemy = std::make_unique<NormalEnemy>();
    } else if (data.type == "HomingEnemy") {
        newEnemy = std::make_unique<HomingEnemy>();
    } else if (data.type == "TargetEnemy") {
        newEnemy = std::make_unique<TargetEnemy>();
    }

    if (newEnemy) {
        newEnemy->Initialize(camera_, data.position, data.hp);
        newEnemy->SetTargetPlayer(player_);
        newEnemy->SetWayPoints(data.movePattern);
        newEnemy->SetFleeWaypoint(data.fleeWaypoint, data.hasFleeData);

        // --- 修正ポイント：unique_ptr から shared_ptr への転送 ---
        // unique_ptr は shared_ptr にそのまま move して渡せます
        enemies_.push_back(std::move(newEnemy));
    }
}

void EnemyManager::SaveToJson(const std::string& filePath)
{
    nlohmann::ordered_json jsonData;
    jsonData["enemies"] = nlohmann::ordered_json::array();

    for (const auto& data : editingPopDatas_) {
        nlohmann::ordered_json enemyData;

        enemyData["popTime"] = data.popTime;
        enemyData["type"] = data.type;
        enemyData["hp"] = data.hp;
        enemyData["position"] = Vector3ToJson(data.position);

        // 移動パターン
        enemyData["movePattern"] = nlohmann::ordered_json::array();
        for (const auto& wp : data.movePattern) {
            nlohmann::ordered_json wpData;
            wpData["target"] = Vector3ToJson(wp.target);
            wpData["timeToReach"] = wp.timeToReach;
            wpData["timeToStop"] = wp.timeToStop;
            enemyData["movePattern"].push_back(wpData);
        }

        // 逃走データ
        if (data.hasFleeData) {
            nlohmann::ordered_json fleeData;
            fleeData["target"] = Vector3ToJson(data.fleeWaypoint.target);
            fleeData["timeToReach"] = data.fleeWaypoint.timeToReach;
            enemyData["flee"] = fleeData;
        }

        jsonData["enemies"].push_back(enemyData);
    }

    std::ofstream file(filePath);
    if (file.is_open()) {
        file << jsonData.dump(4); // インデント4文字で綺麗に保存
        file.close();

        lastWriteTime_ = fs::last_write_time(filePath);
        std::cout << "JSON saved (Ordered): " << filePath << std::endl;
    }
}

void EnemyManager::DrawImGui()
{
    ImGui::Begin("EnemyPopManager");

    if (ImGui::CollapsingHeader("Stage & File Management", ImGuiTreeNodeFlags_DefaultOpen)) {

        // 現在の状況を表示
        ImGui::Text("Camera Current Stage: %d", cameraContrroller_->GetCurrentStage());
        ImGui::Text("Loaded JSON Stage : %d", currentLoadedStage_);
        ImGui::TextDisabled("File: %s", jsonFilePath_.c_str());

        ImGui::Spacing();

        // 読み込むJSONを強制的に切り替える（編集用）
        ImGui::InputInt("Target Stage", &targetEditStage_);

        // マイナスにならないように制限
        if (targetEditStage_ < 0)
            targetEditStage_ = 0;

        ImGui::SameLine();
        if (ImGui::Button("Load Stage JSON")) {
            currentLoadedStage_ = targetEditStage_;
            jsonFilePath_ = GetJsonPath(currentLoadedStage_);
            LoadEnemyData(jsonFilePath_);

            // 別のステージを読み込んだら一旦編集モードをリセットする
            isEditing_ = false;
        }
    }

    ImGui::Separator();

    if (isEditing_) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Editing Stage %d", currentLoadedStage_);
        if (ImGui::Button("Save to JSON")) {
            SaveToJson(jsonFilePath_);
            LoadEnemyData(jsonFilePath_); // セーブ後に再読み込み
            isEditing_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            editingPopDatas_ = popDatas_;
            isEditing_ = false;
        }
    } else {
        if (ImGui::Button("Reload Current JSON")) {
            LoadEnemyData(jsonFilePath_);
        }
        ImGui::SameLine();
        if (ImGui::Button("Start Edit")) {
            isEditing_ = true;
            editingPopDatas_ = popDatas_;
        }
    }

    ImGui::Separator();

    if (ImGui::CollapsingHeader("Enemy Pop List")) {
        for (int i = 0; i < (int)editingPopDatas_.size(); ++i) {
            std::string label = "Enemy [" + std::to_string(i) + "] - " + editingPopDatas_[i].type;
            if (ImGui::Selectable(label.c_str(), selectedEnemyIndex_ == i)) {
                selectedEnemyIndex_ = i;
            }
        }
        if (ImGui::Button("Add New Enemy")) {
            EnemyPopData newData;
            newData.type = "NormalEnemy";
            newData.hp = 100;
            newData.position = { 0, 0, 60 }; // 遠くに
            editingPopDatas_.push_back(newData);
            selectedEnemyIndex_ = (int)editingPopDatas_.size() - 1;
            isEditing_ = true;
        }
    }

    ImGui::Separator();

    if (selectedEnemyIndex_ >= 0 && selectedEnemyIndex_ < (int)editingPopDatas_.size()) {
        ImGui::PushID(selectedEnemyIndex_); // IDをプッシュしてバグ防止

        auto& data = editingPopDatas_[selectedEnemyIndex_];

        ImGui::Text("Selected Enemy [%d]", selectedEnemyIndex_);

        // 基本データ編集
        if (ImGui::InputFloat("Pop Time", &data.popTime))
            isEditing_ = true;

        // typeの編集
        char typeBuffer[64];
        strncpy_s(typeBuffer, data.type.c_str(), sizeof(typeBuffer));
        if (ImGui::InputText("Type", typeBuffer, sizeof(typeBuffer))) {
            data.type = typeBuffer;
            isEditing_ = true;
        }

        if (ImGui::InputInt("HP", &data.hp))
            isEditing_ = true;
        if (ImGui::InputFloat3("Spawn Pos", &data.position.x))
            isEditing_ = true;

        ImGui::Separator();

        // 移動経路（movePattern）編集
        if (ImGui::CollapsingHeader("Move Pattern Waypoints")) {
            for (int j = 0; j < (int)data.movePattern.size(); ++j) {
                ImGui::PushID(j); // WP用のID
                auto& wp = data.movePattern[j];
                std::string wpLabel = "WP [" + std::to_string(j) + "]";
                if (ImGui::InputFloat3(wpLabel.c_str(), &wp.target.x))
                    isEditing_ = true;
                ImGui::SameLine();
                ImGui::SetNextItemWidth(100);
                if (ImGui::InputFloat("timeToReach", &wp.timeToReach))
                    isEditing_ = true;
                if (ImGui::InputFloat("timeToStop", &wp.timeToStop))
                    isEditing_ = true;

                ImGui::SameLine();
                if (ImGui::Button("Delete")) {
                    data.movePattern.erase(data.movePattern.begin() + j);
                    isEditing_ = true;
                }
                ImGui::PopID(); // WP ID
            }
            if (ImGui::Button("Add Waypoint")) {
                WayPoint newWP;
                newWP.target = data.position; // スポーン地点から開始
                newWP.timeToReach = 2.0f;
                newWP.timeToStop = 1.0f;
                data.movePattern.push_back(newWP);
                isEditing_ = true;
            }
        }

        ImGui::Separator();

        // 逃走（flee）データ編集
        if (ImGui::Checkbox("Has Flee Data", &data.hasFleeData))
            isEditing_ = true;
        if (data.hasFleeData) {
            if (ImGui::InputFloat3("Flee Target", &data.fleeWaypoint.target.x))
                isEditing_ = true;
            if (ImGui::InputFloat("Flee Time", &data.fleeWaypoint.timeToReach))
                isEditing_ = true;
        }

        ImGui::Separator();
        if (ImGui::Button("Delete This Enemy")) {
            editingPopDatas_.erase(editingPopDatas_.begin() + selectedEnemyIndex_);
            selectedEnemyIndex_ = -1;
            isEditing_ = true;
        }

        ImGui::PopID(); // Enemy ID
    }

    ImGui::End();
}

void EnemyManager::DrawEnemyImGui()
{
}
