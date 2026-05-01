#pragma once
#include <vector>
#include <memory>
#include "BookUi.h"

class Book {
public:
    void Initialize(const std::vector<std::string>& textures);
    void Update();
    void Draw();

    // ページ操作（外部からはこれだけ呼ぶ）
    void NextPage(); // 次へ（右ページを閉じる）
    void PrevPage(); // 前へ（右ページを開く）

    // 親の座標（これを変えると子も連動する）
    void SetPosition(Vector3 pos) { position_ = pos; }

private:
    std::vector<std::unique_ptr<BookUi>> pages_;
    int currentPageIndex_ = 0; // 現在めくる対象のページ
    Vector3 position_ = { 960.0f, 540.0f, 0.0f };
};