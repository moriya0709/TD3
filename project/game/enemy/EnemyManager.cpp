#include "EnemyManager.h"
#include <algorithm>
#include <externals/nlohmann/json.hpp>
#include <fstream>
#include <iostream> // エラー出力用

#include "type/HomingEnemy.h"
#include "type/NormalEnemy.h"
#include "type/TargetEnemy.h"

using json = nlohmann::json;

void EnemyManager::LoadEnemyData(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "エラー: JSONファイルが開けません [" << filePath << "]\n";
        return;
    }

    json data;
    file >> data;

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

            // 取得したデータをリストに追加
            popDatas_.push_back(data);
        }
    }
}

void EnemyManager::Initialize(const std::string& filePath, Player* player, Camera* camera)
{
    player_ = player;
    camera_ = camera;
    currentTimer_ = 0.0f;
    currentSpawnIndex_ = 0;

    LoadEnemyData(filePath);

    // sort(ぐちゃぐちゃになっているやつを整理)
    std::sort(popDatas_.begin(), popDatas_.end(),
        [](const EnemyPopData& a, const EnemyPopData& b) {
            return a.popTime < b.popTime;
        });
}

void EnemyManager::Update()
{
    currentTimer_ += 1.0f / 60.0f;
    // スポーン処理
    while (currentSpawnIndex_ < popDatas_.size() && popDatas_[currentSpawnIndex_].popTime <= currentTimer_) {

        SpawnEnemy(popDatas_[currentSpawnIndex_]);
        currentSpawnIndex_++; // 次の敵へ
    }

    for (auto& enemy : enemies_) {
        enemy->Update();
    }

    // 死んだ際のlist消し
    // enemies_.remove_if([](const std::unique_ptr<Enemy>& enemy) {
    // return enemy->GetIsDead();
    //});
}

void EnemyManager::Draw3D()
{
    for (auto& enemy : enemies_) {
        enemy->Draw3D();
    }
}

void EnemyManager::SpawnEnemy(const EnemyPopData& data)
{
    std::unique_ptr<Enemy> newEnemy = nullptr;

    // typeから対象の敵を生成。
    if (data.type == "NormalEnemy") {
        newEnemy = std::make_unique<NormalEnemy>();
    } else if (data.type == "HomingEnemy") {
        newEnemy = std::make_unique<HomingEnemy>();
    } else if (data.type == "TargetEnemy") {
        newEnemy = std::make_unique<TargetEnemy>();
    }

    // 生成に成功したら初期化
    if (newEnemy) {
        newEnemy->Initialize(camera_, data.position, data.hp);
        newEnemy->SetTargetPlayer(player_); // プレイヤーの情報を渡す

        enemies_.push_back(std::move(newEnemy));
    }
}
