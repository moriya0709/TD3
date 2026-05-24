#include "EnemyManager.h"
#include <algorithm>

#include <fstream>
#include <iostream> // エラー出力用

#include "../camera/CameraController.h"
#include "Boss/type/banana.h"
#include "Boss/type/grapesBoss.h"
#include "Normal/type/HomingEnemy.h"
#include "Normal/type/NormalEnemy.h"
#include "Normal/type/ShieldEnemy.h"
#include "Normal/type/TargetEnemy.h"
#include "Normal/type/rushEnemy.h"

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

    for (auto& boss : gboss_) {
        boss->Update();
    }
    for (auto& boss : bboss_) {
        boss->Update();
    }

    ///
    ///score加算
    ///

    //敵が消滅する直前に死んだ敵のスコアをGetScoreで回収
    for (auto& enemy : enemies_)
    {
        if (!enemy->GetIsAlive())
        {
            collectionScore_ += enemy->GetScore();
        }
    }

    for (auto& boss : gboss_)
    {
        if (!boss->GetIsAlive())
        {
            collectionScore_ += boss->GetScore();
        }
    }

    for (auto& boss : bboss_)
    {
        if (!boss->GetIsAlive())
        {
            collectionScore_ += boss->GetScore();
        }
    }

    ///
    ///
    /// 
  
    // --- 修正ポイント：引数を std::shared_ptr に変更 ---
    enemies_.remove_if([](const std::shared_ptr<Enemy>& enemy) { return !enemy->GetIsAlive(); });
    gboss_.remove_if([](const std::shared_ptr<grapesBoss>& enemy) { return !enemy->GetIsAlive(); });
    bboss_.remove_if([](const std::shared_ptr<banana>& enemy) { return !enemy->GetIsAlive(); });
}
void EnemyManager::Draw3D()
{
    for (auto& enemy : enemies_) {
        enemy->Draw3D();
    }

    for (auto& boss : gboss_) {

        boss->Draw3D();
    }
    for (auto& boss : bboss_) {

        boss->Draw3D();
    }
}

void EnemyManager::SetcurrentTimer_(float timer)
{
    precurrenTimer = currentTimer_;

    if (currentTimer_ > timer) {
        currentSpawnIndex_ = 0;
        enemies_.remove_if([](const std::shared_ptr<Enemy>& enemy) { return true; });
        gboss_.remove_if([](const std::shared_ptr<grapesBoss>& enemy) { return true; });
        bboss_.remove_if([](const std::shared_ptr<banana>& enemy) { return true; });
    }
    currentTimer_ = timer;
}

void EnemyManager::SetEnemyclear()
{
    enemies_.clear();
}

int EnemyManager::GiveScore()
{
    int score = collectionScore_;
    collectionScore_ = 0;//渡したため回収スコアリセット
    return score;
}

void EnemyManager::LoadEnemyData(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
        return;

    json data; // ← 注意: ここの変数名も data
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
            EnemyPopData popData; // ★ 変数名被りを避けるため popData に変更推奨

            // --- 安全なデータ取得 ---
            popData.popTime = enemyData.value("popTime", 0.0f);
            popData.type = enemyData.value("type", "NormalEnemy");
            popData.hp = enemyData.value("hp", 10);

            // 座標(Vector3)の読み込み
            if (enemyData.contains("position")) {
                const auto& posData = enemyData["position"];
                popData.position.x = posData.value("x", 0.0f);
                popData.position.y = posData.value("y", 0.0f);
                popData.position.z = posData.value("z", 0.0f);
            } else {
                popData.position = { 0.0f, 0.0f, 0.0f };
            }

            // ==========================================
            // ★ 修正ポイント：ボス以外の時だけ移動・逃走を読み込む
            // ==========================================
            if (popData.type != "grapesBoss" || popData.type != "bananaBoss") {

                // --- movePattern の読み込み ---
                if (enemyData.contains("movePattern") && enemyData["movePattern"].is_array()) {
                    for (const auto& wpData : enemyData["movePattern"]) {
                        WayPoint wp;
                        wp.timeToReach = wpData.value("timeToReach", 1.0f);
                        wp.timeToStop = wpData.value("timeToStop", 1.0f);

                        if (wpData.contains("target")) {
                            const auto& tData = wpData["target"];
                            wp.target.x = tData.value("x", 0.0f);
                            wp.target.y = tData.value("y", 0.0f);
                            wp.target.z = tData.value("z", 0.0f);
                        } else {
                            wp.target = { 0.0f, 0.0f, 0.0f };
                        }
                        popData.movePattern.push_back(wp);
                    }
                }

                // --- flee の読み込み ---
                if (enemyData.contains("flee")) {
                    const auto& fleeData = enemyData["flee"];
                    popData.fleeWaypoint.timeToReach = fleeData.value("timeToReach", 2.0f);
                    popData.fleeWaypoint.timeToStop = 0.0f;

                    if (fleeData.contains("target")) {
                        const auto& tData = fleeData["target"];
                        popData.fleeWaypoint.target.x = tData.value("x", 0.0f);
                        popData.fleeWaypoint.target.y = tData.value("y", 0.0f);
                        popData.fleeWaypoint.target.z = tData.value("z", 0.0f);
                    } else {
                        popData.fleeWaypoint.target = { 0.0f, 0.0f, 0.0f };
                    }
                    popData.hasFleeData = true;
                } else {
                    popData.hasFleeData = false;
                }

            } else {
                // grapesBoss の場合の初期化処理（念のため）
                popData.hasFleeData = false;
            }
            // ==========================================

            // 取得したデータをリストに追加
            popDatas_.push_back(popData);
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
    // --- 1. ボス（grapesBoss）の場合 ---
    if (data.type == "grapesBoss") {
        auto newBoss = std::make_unique<grapesBoss>();

        // ボス専用の初期化
        newBoss->Initialize(camera_, data.position, data.hp);
        newBoss->SetTargetPlayer(player_);

        // ボスリストに追加
        gboss_.push_back(std::move(newBoss));
        return; // ボスとして生成したのでここで終了
    } else if (data.type == "bananaBoss") {
        auto newBoss = std::make_unique<banana>();

        // ボス専用の初期化
        newBoss->Initialize(camera_, data.position, data.hp);
        newBoss->SetTargetPlayer(player_);

        // ボスリストに追加
        bboss_.push_back(std::move(newBoss));
        return; // ボスとして生成したのでここで終了
    }

    // --- 2. ザコ敵（Enemy）の場合 ---
    std::unique_ptr<Enemy> newEnemy = nullptr;

    if (data.type == "NormalEnemy") {
        newEnemy = std::make_unique<NormalEnemy>();
    } else if (data.type == "HomingEnemy") {
        newEnemy = std::make_unique<HomingEnemy>();
    } else if (data.type == "TargetEnemy") {
        newEnemy = std::make_unique<TargetEnemy>();
    } else if (data.type == "rushEnemy") {
        newEnemy = std::make_unique<rushEnemy>();
    } else if (data.type == "ShieldEnemy") {
        newEnemy = std::make_unique<ShieldEnemy>();
    }

    if (newEnemy) {
        newEnemy->Initialize(camera_, data.position, data.hp);
        newEnemy->SetTargetPlayer(player_);
        newEnemy->SetWayPoints(data.movePattern);
        newEnemy->SetFleeWaypoint(data.fleeWaypoint, data.hasFleeData);

        // ザコ敵リストに追加
        enemies_.push_back(std::move(newEnemy));
    }
}

void EnemyManager::SaveToJson(const std::string& filePath)
{
#ifdef USE_IMGUI
    nlohmann::ordered_json jsonData;
    jsonData["enemies"] = nlohmann::ordered_json::array();

    for (const auto& data : editingPopDatas_) {
        nlohmann::ordered_json enemyData;

        enemyData["popTime"] = data.popTime;
        enemyData["type"] = data.type;
        enemyData["hp"] = data.hp;
        enemyData["position"] = Vector3ToJson(data.position);

        // ★ 修正ポイント：ボス以外の時だけ移動・逃走データを保存する
        if (data.type != "grapesBoss" || data.type != "bananaBoss") {
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
        }

        jsonData["enemies"].push_back(enemyData);
    }

    std::ofstream file(filePath);
    if (file.is_open()) {
        file << jsonData.dump(4);
        file.close();

        lastWriteTime_ = fs::last_write_time(filePath);
        std::cout << "JSON saved (Ordered): " << filePath << std::endl;
    }
#endif
}

void EnemyManager::DrawImGui()
{
#ifdef USE_IMGUI
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

        // ★ 修正ポイント：ボス以外の時だけ移動・逃走の編集UIを表示する
        if (data.type != "grapesBoss" || data.type != "bananaBoss") {
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
        } // ★ ボス除外ここまで

        ImGui::Separator();
        if (ImGui::Button("Delete This Enemy")) {
            editingPopDatas_.erase(editingPopDatas_.begin() + selectedEnemyIndex_);
            selectedEnemyIndex_ = -1;
            isEditing_ = true;
        }

        ImGui::PopID(); // Enemy ID
    }

    ImGui::End();
#endif
}

void EnemyManager::DrawEnemyImGui()
{
}
