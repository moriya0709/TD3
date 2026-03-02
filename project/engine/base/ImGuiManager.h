#pragma once
#ifdef USE_IMGUI
#include <externals/imgui\imgui.h>
#include <externals/imgui/imgui_impl_dx12.h>
#include <externals/imgui/imgui_impl_win32.h>
#endif

class WindowAPI;
class DirectXCommon;
class SrvManager;

class ImGuiManager {
public:
	// 初期化
	void Initialize(WindowAPI* windowAPI,DirectXCommon* dxCommon, SrvManager* srvManager);
	// 描画
	void Draw();
	// 終了
	void Finalize();

	// 受付開始
	void Begin();
	// 受付終了
	void End();

private:
	// DirectXCommonポインタ
	DirectXCommon* dxCommon_;
	// SRVマネージャーポインタ
	SrvManager* srvManager_;

};

