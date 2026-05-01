#include "Book.h"

void Book::Initialize(const std::vector<std::string>& textures) {
    for (int i = 0; i < (int)textures.size(); i++) {
        auto page = std::make_unique<BookUi>();
        page->Initialize(textures[i]);

        // 重なり順の初期化（インデックスが低いほど手前）
        // ※Zバッファが 0(手前)～1(奥) の設定なら、iが大きいほど数値を大きくする
        page->SetBaseZ(0.5f + (i * 0.001f));
        page->SetPosition(position_);

        pages_.push_back(std::move(page));
    }
}

void Book::Update() {
    for (int i = 0; i < (int)pages_.size(); i++) {
        auto& page = pages_[i];
        float curlX = page->GetCurrentCurlX();

        float targetZ = 0.5f;
        if (curlX <= 0.0f) {
            // ① 右側に待機中（まだめくっていない）
            // 手前から奥へ順に重ねる
            targetZ = 0.5f + (i * 0.001f);
        } else if (curlX >= DirectX::XM_PI) {
            // ② 左側にめくり終わった！
            // ★次のページ（下のページ）を見せるため、一気に一番奥へ送る！
            targetZ = 0.8f - (i * 0.001f);
        } else {
            // ③ まさにめくっている最中！
            // ★アニメーションを見せるため、一時的に一番手前へ持ってくる！
            targetZ = 0.2f - (i * 0.001f);
        }

        page->SetBaseZ(targetZ);

        // 座標同期と更新
        page->Update();
    }
}

void Book::Draw() {
    // 背面のページから順に描画（ページ番号が大きい順に描画して重ねる）
    for (int i = (int)pages_.size() - 1; i >= 0; i--) {
        pages_[i]->Draw();
    }
}

void Book::NextPage() {
    // 現在のページを「右から左」へ閉じる
    if (currentPageIndex_ < pages_.size() - 1) {
        pages_[currentPageIndex_]->StartClosePageR();
        currentPageIndex_++;
    }
}

void Book::PrevPage() {
    // 一つ前のページを「左から右」へ開く
    if (currentPageIndex_ > 0) {
        currentPageIndex_--;
        pages_[currentPageIndex_]->StartOpenPageR();
    }
}