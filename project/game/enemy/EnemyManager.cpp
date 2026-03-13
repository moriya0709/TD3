#include "EnemyManager.h"
#include <algorithm>

#include <fstream>
#include <iostream> // エラー出力用

#include "type/HomingEnemy.h"
#include "type/NormalEnemy.h"
#include "type/TargetEnemy.h"

using json = nlohmann::json;
namespace fs = std::filesystem;

void EnemyManager::Initialize(const std::string& filePath, Player* player, Camera* camera)
{
    player_ = player;
    camera_ = camera;
    currentTimer_ = 0.0f;
    precurrenTimer = 0.0f;
    currentSpawnIndex_ = 0;

    LoadEnemyData(filePath);
}

void EnemyManager::Update()
{
    if (currentTimer_ == precurrenTimer) {
        return;
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
    // unique_ptr で生成する（これはそのままでOK）
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
    json jsonData;
    jsonData["enemies"] = json::array(); // 配列として初期化

    for (const auto& data : editingPopDatas_) { // 編集中のデータを保存
        json enemyData;
        enemyData["popTime"] = data.popTime;
        enemyData["type"] = data.type;
        enemyData["hp"] = data.hp;
        enemyData["position"] = Vector3ToJson(data.position);

        // 移動パターンをJSONに変換
        enemyData["movePattern"] = json::array();
        for (const auto& wp : data.movePattern) {
            json wpData;
            wpData["timeToReach"] = wp.timeToReach;
            wpData["target"] = Vector3ToJson(wp.target);
            enemyData["movePattern"].push_back(wpData);
        }

        // 逃走データをJSONに変換
        if (data.hasFleeData) {
            json fleeData;
            fleeData["timeToReach"] = data.fleeWaypoint.timeToReach;
            fleeData["target"] = Vector3ToJson(data.fleeWaypoint.target);
            enemyData["flee"] = fleeData;
        }

        jsonData["enemies"].push_back(enemyData);
    }

    std::ofstream file(filePath);
    if (file.is_open()) {
        file << jsonData.dump(4); // インデント4文字で保存
        file.close();

        // 【重要】保存直後に最終更新日時を更新し、自らホットリロードするのを防ぐ
        lastWriteTime_ = fs::last_write_time(filePath);
        std::cout << "JSON saved: " << filePath << std::endl;
    }
}
