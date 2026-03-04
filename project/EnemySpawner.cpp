#include "EnemySpawner.h"
#include <fstream>
#include <iostream>
// #include "json.hpp"// ←後でインストール

void EnemySpawner::loadJson(const std::string& filename)
{
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "エラー: " << filename << " を開けませんでした。" << std::endl;
        return;
    }

    // json jsonData;
    // file >> jsonData;

    // JSONデータをもとに敵を生成する
    // for (const auto& enemyData : jsonData["enemies"]) {
    // float x = enemyData["position"]["x"];
    // float y = enemyData["position"]["y"];
    // float z = enemyData["position"]["z"];

    // 読み込んだ座標を使って、スライムを生成（メモリの自動管理を活用）
    // std::unique_ptr<Enemy> newEnemy = std::make_unique<Slime>(x, y, z);

    // 敵を出現させる（スライムのappearが呼ばれる）
    // newEnemy->appear();
    //}
}
