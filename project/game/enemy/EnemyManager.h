#pragma once
#include <Calc.h>
#include <list>
#include <memory>
#include <string>

#include "Enemy.h"

// 敵の出現情報を保持する構造体
struct EnemyPopData {
    float popTime; // 出現時間（または距離）
    std::string type; // 敵の種類
    Vector3 position; // 出現座標
    int hp; // 体力
};

class EnemyManager {
public:
    /// <summary>
    /// Json読み込み―
    /// </summary>
    /// <param name="filePath">読み込み対象のファイル</param>
    void LoadEnemyData(const std::string& filePath);

    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="filePath">読み込むぁいる</param>
    /// <param name="player">プレイヤーのポインタ</param>
    void Initialize(const std::string& filePath, Player* player, Camera* camera);

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw3D();

    /* Set関数 */

    /* Get関数 */
    // 【当たり判定用】生きている敵のリストを取得する
    const std::list<std::unique_ptr<Enemy>>& GetEnemies() const { return enemies_; }

private:
    /// <summary>
    /// 敵のスポーン
    /// </summary>
    /// <param name="data">データ</param>
    void SpawnEnemy(const EnemyPopData& data);

    // 読み込んだデータをストックしておくリスト
    std::vector<EnemyPopData> popDatas_;
    uint32_t currentSpawnIndex_ = 0; // 次に出現させる敵のインデックス

    float currentTimer_ = 0.0f; // ゲーム開始からの経過時間（または進行距離）
    std::list<std::unique_ptr<Enemy>> enemies_; // 生きている敵のリスト
    Player* player_ = nullptr; // ターゲット用のプレイヤーポインタ
    Camera* camera_ = nullptr; // カメラポインタ
};
