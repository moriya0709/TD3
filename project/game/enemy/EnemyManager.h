#pragma once
#include <Calc.h>
#include <externals/nlohmann/json.hpp>
#include <filesystem>
#include <list>
#include <memory>
#include <string>

#include "Enemy.h"

class Player;
class CameraController;

// 敵の出現情報を保持する構造体
struct EnemyPopData {
    float popTime; // 出現時間（または距離）
    std::string type; // 敵の種類
    Vector3 position; // 出現座標
    int hp; // 体力
    // 複数の移動地点を保持するリスト
    std::vector<WayPoint> movePattern;
    WayPoint fleeWaypoint; // 逃走先
    bool hasFleeData; // 逃走先があるか
};

class EnemyManager {
public:
    /// <summary>
    /// 初期化
    /// </summary>
    /// <param name="filePath">読み込むぁいる</param>
    /// <param name="player">プレイヤーのポインタ</param>
    void Initialize(Player* player, Camera* camera, CameraController* cameraController);

    /// <summary>
    /// 更新
    /// </summary>
    void Update();

    /// <summary>
    /// 描画
    /// </summary>
    void Draw3D();

    // IMGUI描画
    void DrawImGui();
    void DrawEnemyImGui();

    /* Set関数 */
    void SetcurrentTimer_(float timer);

    /* Get関数 */
    // 【当たり判定用】生きている敵のリストを取得する
    const std::list<std::shared_ptr<Enemy>>& GetEnemies() const { return enemies_; }

private:
    /// <summary>
    /// Json読み込み―
    /// </summary>
    /// <param name="filePath">読み込み対象のファイル</param>
    void LoadEnemyData(const std::string& filePath);

    /// <summary>
    /// 敵のスポーン
    /// </summary>
    /// <param name="data">データ</param>
    void SpawnEnemy(const EnemyPopData& data);

    // 現在のデータをJSONファイルに保存する
    void SaveToJson(const std::string& filePath);

    // Vector3をJSONに変換するためのヘルパー関数
    nlohmann::ordered_json Vector3ToJson(const Vector3& v)
    {
        return { { "x", v.x }, { "y", v.y }, { "z", v.z } };
    }

    // ステージ番号からJSONのファイルパスを生成する関数
    std::string GetJsonPath(int stage) const
    {
        return "Resource/Data/enemySpawnStage" + std::to_string(stage) + ".json";
    }

private:
    // 読み込んだデータをストックしておくリスト
    std::vector<EnemyPopData> popDatas_;
    uint32_t currentSpawnIndex_ = 0; // 次に出現させる敵のインデックス

    float currentTimer_ = 0.0f; // ゲーム開始からの経過時間（または進行距離）
    float precurrenTimer = 0.0f;
    std::list<std::shared_ptr<Enemy>> enemies_; // 生きている敵のリスト

    Player* player_ = nullptr; // ターゲット用のプレイヤーポインタ
    Camera* camera_ = nullptr; // カメラポインタ
    CameraController* cameraContrroller_ = nullptr;

    // 書き込みのデータ
    std::string jsonFilePath_; // 読み込んでいるJSONのパス
    std::vector<EnemyPopData> editingPopDatas_; // IMGUIで編集中のデータ

    // ホットリロード用の変数
    std::filesystem::file_time_type lastWriteTime_; // JSONの最終更新日時

    // IMGUI用の状態変数
    bool isEditing_ = false; // 編集モードかどうか
    int selectedEnemyIndex_ = -1; // IMGUIで選択中の敵のインデックス

    int currentLoadedStage_ = -1; // 現在読み込まれているステージ番号
    int targetEditStage_ = 0; // ImGuiで編集・読み込みたいステージ番号
};
