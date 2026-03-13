#include "CameraController.h"
#include <algorithm>
#include <externals/nlohmann/json.hpp>
#include <filesystem>
#include <fstream>
#include <imgui.h>
#include <iostream>

using json = nlohmann::json;
namespace fs = std::filesystem;

// ※ プロジェクトに合わせて、ここに使用しているライン描画関数をインクルードまたは定義してください
// 以下の関数はダミーです
void DrawLine(const Vector3& s, const Vector3& e, uint32_t color) {
    /* 描画処理の実装 (例: DebugDraw::AddLine) */
}

// --- nlohmann/json 変換定義 ---
void to_json(json& j, const Vector3& v) { j = json{ {"x", v.x}, {"y", v.y}, {"z", v.z} }; }
void from_json(const json& j, Vector3& v) { j.at("x").get_to(v.x); j.at("y").get_to(v.y); j.at("z").get_to(v.z); }
void to_json(json& j, const CameraState& s) { j = json{ {"time", s.time}, {"vel", s.velocity}, {"angVel", s.angularVelocity} }; }
void from_json(const json& j, CameraState& s) { j.at("time").get_to(s.time); j.at("vel").get_to(s.velocity); j.at("angVel").get_to(s.angularVelocity); }

// --- 補間ヘルパー ---
Vector3 CameraController::CameraLerp(const Vector3& start, const Vector3& end, float t) {
    return { start.x + (end.x - start.x) * t, start.y + (end.y - start.y) * t, start.z + (end.z - start.z) * t };
}

Vector3 CameraController::CatmullRom(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    return {
        0.5f * (2.0f * p1.x + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3),
        0.5f * (2.0f * p1.y + (-p0.y + p2.y) * t + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3),
        0.5f * (2.0f * p1.z + (-p0.z + p2.z) * t + (2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 + (-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3)
    };
}

void CameraController::Initialize(Camera* targetCamera) {
    this->camera = targetCamera;
    timer = 0.0f;
    isReplaying = isPaused = isRecording = false;
    currentSlot = 1;
    playbackSpeed = 1.0f;

    if (camera) {
        initialTransform.rotate = camera->GetRotate();
        initialTransform.translate = camera->GetTranslate();
    }
    cameraTransform = initialTransform;
    LoadFromJSON(GetFilePath(currentSlot));
}

std::string CameraController::GetFilePath(int slot) const { return "Resource/Data/replay_" + std::to_string(slot) + ".json"; }

void CameraController::Update() {
    auto input = Input::GetInstance();
    int newSlot = -1;
    if (input->TriggerKey(DIK_1)) newSlot = 1;
    else if (input->TriggerKey(DIK_2)) newSlot = 2;
    else if (input->TriggerKey(DIK_3)) newSlot = 3;
    else if (input->TriggerKey(DIK_4)) newSlot = 4;
    else if (input->TriggerKey(DIK_5)) newSlot = 5;

    if (newSlot != -1 && newSlot != currentSlot) {
        currentSlot = newSlot;
        isReplaying = isRecording = false;
        LoadFromJSON(GetFilePath(currentSlot));
        if (!stateHistory.empty()) StartReplay();
    }

    float deltaTime = 1.0f / 60.0f;
    Vector3 currentVel = { 0, 0, 0 };
    Vector3 currentAngVel = { 0, 0, 0 };

    if (isReplaying) {
        if (!isPaused) {
            timer += deltaTime * playbackSpeed;
            ApplyReplayState(currentVel, currentAngVel);
            ApplyPhysics(
                { currentVel.x * playbackSpeed, currentVel.y * playbackSpeed, currentVel.z * playbackSpeed },
                { currentAngVel.x * playbackSpeed, currentAngVel.y * playbackSpeed, currentAngVel.z * playbackSpeed });
        }
    } else if (isRecording) {
        timer += deltaTime;
        currentVel = uiVelocity;
        currentAngVel = uiAngularVelocity;
        RecordStateIfChanged(currentVel, currentAngVel);
        ApplyPhysics(currentVel, currentAngVel);
    } else {
        // ★ 待機中：常に初期座標を適用。これで録画開始前でも座標が固定される
        cameraTransform = initialTransform;
    }

    if (camera) {
        camera->SetRotate(cameraTransform.rotate);
        camera->SetTranslate(cameraTransform.translate);
    }
}

void CameraController::DrawImGui() {
    ImGui::Begin("Camera Recording Studio");

    ImGui::Checkbox("Show Trace Line", &showDebugTrace);
    ImGui::SameLine();
    ImGui::Checkbox("Catmull-Rom Mode", &isSmoothMode);

    ImGui::Separator();

    ImGui::Text("Slot: %d", currentSlot);
    for (int i = 1; i <= 5; ++i) {
        if (ImGui::RadioButton(std::to_string(i).c_str(), currentSlot == i)) {
            currentSlot = i;
            isReplaying = isRecording = false;
            LoadFromJSON(GetFilePath(currentSlot));
            if (!stateHistory.empty()) StartReplay();
        }
        if (i < 5) ImGui::SameLine();
    }

    if (ImGui::CollapsingHeader("Initial Transform Setup", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat3("Start Pos", &initialTransform.translate.x, 0.1f);
        ImGui::DragFloat3("Start Rot", &initialTransform.rotate.x, 0.01f);
        if (ImGui::Button("Set Current Pos as Initial")) initialTransform = cameraTransform;
    }

    ImGui::Separator();

    if (isReplaying) {
        ImGui::TextColored(ImVec4(0, 1, 1, 1), "STATUS: REPLAYING");
        ImGui::SliderFloat("Speed", &playbackSpeed, 0.0f, 3.0f, "%.1fx");

        float maxTime = stateHistory.empty() ? 0.0f : stateHistory.back().time;
        if (ImGui::SliderFloat("Seek", &timer, 0.0f, maxTime)) SeekTo(timer);

        if (ImGui::Button(isPaused ? "Play" : "Pause")) isPaused = !isPaused;
        ImGui::SameLine();
        if (ImGui::Button("Stop")) isReplaying = false;

    } else {
        ImGui::Text("STATUS: %s", isRecording ? "RECORDING..." : "WAITING (Initial Applied)");
        ImGui::DragFloat3("Input Vel", &uiVelocity.x, 0.01f, -1.0f, 1.0f);
        ImGui::DragFloat3("Input Rot", &uiAngularVelocity.x, 0.005f, -0.05f, 0.05f);

        if (!isRecording) {
            if (ImGui::Button("● Start Recording from Initial", ImVec2(240, 30))) {
                stateHistory.clear();
                timer = 0.0f;
                isRecording = true;
                cameraTransform = initialTransform; // 開始位置へ強制移動
                RecordStateIfChanged(uiVelocity, uiAngularVelocity);
            }
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.0f, 0.0f, 1.0f));
            if (ImGui::Button("■ Stop & Save", ImVec2(240, 30))) {
                isRecording = false;
                SaveToJSON(GetFilePath(currentSlot));
            }
            ImGui::PopStyleColor();
        }
    }
    ImGui::End();
}

void CameraController::DrawDebugTrace() {
    if (!showDebugTrace || stateHistory.empty()) return;

    Vector3 prevPos = initialTransform.translate;
    Vector3 currentPos = initialTransform.translate;
    
    // シミュレーション用の一時変数
    float simTime = 0.0f;
    float maxTime = stateHistory.back().time;
    float step = 1.0f / 60.0f; // 1フレーム刻みでシミュレート

    // 元の状態を保存
    float backupTimer = timer;

    while (simTime <= maxTime) {
        timer = simTime;
        Vector3 v, av;
        ApplyReplayState(v, av);
        
        currentPos.x += v.x; currentPos.y += v.y; currentPos.z += v.z;
        
        DrawLine(prevPos, currentPos, 0x00FF00FF); // 緑色で描画
        
        prevPos = currentPos;
        simTime += step;
    }

    timer = backupTimer; // タイマーを復元
}

void CameraController::ApplyReplayState(Vector3& vel, Vector3& angVel) {
    if (stateHistory.empty()) return;

    size_t n = stateHistory.size();
    size_t i1 = 0;
    while (i1 < n && timer > stateHistory[i1].time) {
        i1++;
    }

    if (i1 == 0) {
        vel = stateHistory[0].velocity;
        angVel = stateHistory[0].angularVelocity;
    } else if (i1 >= n) {
        vel = stateHistory.back().velocity;
        angVel = stateHistory.back().angularVelocity;
        isPaused = true;
    } else {
        size_t i0 = i1 - 1;
        float t = (timer - stateHistory[i0].time) / (stateHistory[i1].time - stateHistory[i0].time);

        if (isSmoothMode && n >= 4) {
            // Catmull-Rom 用の 4 点を準備 (i-1, i, i+1, i+2)
            size_t im1 = (i0 == 0) ? i0 : i0 - 1;
            size_t i2 = i1 + 1;
            if (i2 >= n) i2 = n - 1;

            vel = CatmullRom(stateHistory[im1].velocity, stateHistory[i0].velocity, stateHistory[i1].velocity, stateHistory[i2].velocity, t);
            angVel = CatmullRom(stateHistory[im1].angularVelocity, stateHistory[i0].angularVelocity, stateHistory[i1].angularVelocity, stateHistory[i2].angularVelocity, t);
        } else {
            // 線形補間
            vel = CameraLerp(stateHistory[i0].velocity, stateHistory[i1].velocity, t);
            angVel = CameraLerp(stateHistory[i0].angularVelocity, stateHistory[i1].angularVelocity, t);
        }
    }
}

void CameraController::ApplyPhysics(const Vector3& vel, const Vector3& angVel) {
    cameraTransform.translate.x += vel.x; cameraTransform.translate.y += vel.y; cameraTransform.translate.z += vel.z;
    cameraTransform.rotate.x += angVel.x; cameraTransform.rotate.y += angVel.y; cameraTransform.rotate.z += angVel.z;
}

void CameraController::RecordStateIfChanged(const Vector3& vel, const Vector3& angVel) {
    if (vel.x != lastRecordedVel.x || vel.y != lastRecordedVel.y || vel.z != lastRecordedVel.z ||
        angVel.x != lastRecordedAngVel.x || angVel.y != lastRecordedAngVel.y || angVel.z != lastRecordedAngVel.z) {
        stateHistory.push_back({ timer, vel, angVel });
        lastRecordedVel = vel;
        lastRecordedAngVel = angVel;
    }
}

void CameraController::StartReplay() {
    if (stateHistory.empty()) return;
    isReplaying = true; isPaused = isRecording = false;
    timer = 0.0f;
    cameraTransform = initialTransform;
}

void CameraController::SeekTo(float targetTime) {
    timer = targetTime;
    cameraTransform = initialTransform;
    const float simDelta = 1.0f / 60.0f;
    float backupTimer = timer;
    for (float t = 0.0f; t < targetTime; t += simDelta) {
        Vector3 v, av;
        timer = t;
        ApplyReplayState(v, av);
        ApplyPhysics(v, av);
    }
    timer = backupTimer;
}

void CameraController::UpdateOrInsertKeyframe(float time, const Vector3& vel, const Vector3& angVel) {
    auto it = std::find_if(stateHistory.begin(), stateHistory.end(), [time](const CameraState& s) { return std::abs(s.time - time) < 0.001f; });
    if (it != stateHistory.end()) {
        it->velocity = vel; it->angularVelocity = angVel;
    } else {
        stateHistory.push_back({ time, vel, angVel });
        std::sort(stateHistory.begin(), stateHistory.end(), [](const CameraState& a, const CameraState& b) { return a.time < b.time; });
    }
}

void CameraController::SaveToJSON(const std::string& filename) {
    fs::create_directories(fs::path(filename).parent_path());
    json j;
    j["initialTransform"] = { {"translate", initialTransform.translate}, {"rotate", initialTransform.rotate} };
    j["states"] = stateHistory;
    std::ofstream file(filename);
    if (file.is_open()) file << j.dump(4);
}

void CameraController::LoadFromJSON(const std::string& filename) {
    stateHistory.clear();
    if (!fs::exists(filename)) return;
    std::ifstream file(filename);
    if (file.is_open()) {
        try {
            json j; file >> j;
            initialTransform.translate = j["initialTransform"]["translate"].get<Vector3>();
            initialTransform.rotate = j["initialTransform"]["rotate"].get<Vector3>();
            stateHistory = j["states"].get<std::vector<CameraState>>();
        } catch (...) {}
    }
}