#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "MyMath.h"
#include "Object3d.h"

class Object3dCommon;
class Model;

// Free-position 3D stage editor for a 2.5D game.
class StageEditor {
public:
    enum class Mode { Editor, GamePlay };
    enum class Category { Basic, Gimmick, Enemy, System };

    struct ItemDefinition {
        int id = 0;
        const char* name = "";
        Category category = Category::Basic;
        const char* modelDirectory = "";
        const char* modelFile = "";
        Vector4 color{1,1,1,1};
        Vector3 defaultScale{1,1,1};
    };

    struct Placement {
        uint32_t id = 0;
        int itemId = 0;
        Vector3 position{};
        Vector3 rotation{};
        Vector3 scale{1,1,1};
        int variant = 0;
        Vector3 moveOffset{0,3,0};
    };

    void Initialize(Object3dCommon* common, uint32_t environmentTexture, float environmentCoefficient);
    void Finalize();
    void Update();
    void Draw3D();
    void Draw();
    void DrawGameView(float rectX, float rectY, float rectWidth, float rectHeight);

    bool IsEditingGameView() const { return mode_ == Mode::Editor; }
    bool IsGamePlayMode() const { return mode_ == Mode::GamePlay; }
    void ToggleMode();

private:
    struct Snapshot { std::vector<Placement> placements; uint32_t nextId = 1; };

    void DrawModeSwitcher();
    void DrawPalette();
    void DrawTransformPanel();
    void DrawFilePanel();
    void DrawPlaylistManager();
    void DrawSelectedItemSettings();
    void HandleEditorInput();

    void SetMode(Mode mode);
    void PlaceItem();
    void RemoveNearest();
    void RotateCursor();
    int FindNearestPlacement(float maxDistance) const;
    const ItemDefinition* FindItem(int id) const;

    std::unique_ptr<Object3d> CreateObject(const Placement& placement) const;
    void RebuildObjects();
    void UpdateCursorObject();

    Snapshot MakeSnapshot() const;
    void PushUndo();
    void Undo();
    void Redo();

    void NewStage();
    bool SaveAsNewStage();
    bool LoadStage(const std::string& path);
    void RefreshStageFiles();
    void LoadPlaylist();
    void SavePlaylist();
    std::string MakeUniqueStagePath() const;
    static std::string SanitizeFileName(const std::string& value);

    Object3dCommon* object3dCommon_ = nullptr;
    uint32_t environmentTexture_ = 0;
    float environmentCoefficient_ = 0.0f;

    Mode mode_ = Mode::Editor;
    Category category_ = Category::Basic;
    int selectedItemId_ = 1;
    uint32_t nextPlacementId_ = 1;

    Vector3 cursorPosition_{0.0f, 2.0f, 0.0f};
    Vector3 cursorRotation_{};
    Vector3 cursorScale_{1.0f, 1.0f, 1.0f};
    float cursorMoveSpeed_ = 4.0f;
    bool gameViewHovered_ = false;

    std::vector<Placement> placements_;
    std::vector<std::unique_ptr<Object3d>> objects_;
    std::unique_ptr<Object3d> cursorObject_;
    std::unique_ptr<Object3d> cursorFrameObject_;

    std::vector<Snapshot> undoStack_;
    std::vector<Snapshot> redoStack_;
    std::vector<std::string> stageFiles_;
    int selectedStageFile_ = -1;
    std::vector<std::string> campaignFiles_;
    std::vector<std::string> availableFiles_;
    int selectedCampaignIndex_ = -1;
    int selectedAvailableIndex_ = -1;

    int stageWidth_ = 100;
    int stageHeight_ = 100;
    int stageDepth_ = 20;
    int selectedDoorId_ = 1;
    int selectedSwitchId_ = 1;
    int selectedTimedGroupId_ = 1;
    int selectedTimedOrderId_ = 0;
    Vector3 movingFloorOffset_{0,3,0};

    std::string stageName_ = "stage_01";
    std::string currentFile_;
    std::string status_ = "Ready";
};