#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Cursor-driven 2.5D stage editor.
// X/Y are the gameplay plane. Z selects a small number of depth layers.
class StageEditor {
public:
    enum class Category { Basic, Gimmick, Enemy, System };

    struct BlockDefinition {
        int id;
        const char* name;
        Category category;
        uint32_t color;
        bool rotatable;
    };

    struct Cell {
        int blockId = 0;
        int rotationQuarterTurns = 0;
    };

    void Initialize();
    void Draw();
    void DrawGameView(float rectX, float rectY, float rectWidth, float rectHeight);
    bool IsEditingGameView() const { return true; }

private:
    struct Snapshot {
        std::vector<Cell> cells;
        int cursorX = 0;
        int cursorY = 0;
        int cursorZ = 0;
    };

    static constexpr int kDefaultWidth = 80;
    static constexpr int kDefaultHeight = 24;
    static constexpr int kDepthLayers = 3;
    static constexpr int kTilePixels = 32;

    void DrawStageEditorWindow();
    void DrawCategoryTabs();
    void DrawBlockPalette();
    void DrawFilePanel();
    void DrawHelpPanel();
    void DrawOverlay(float x, float y, float width, float height);
    void HandleKeyboard();
    void HandleMouse(float x, float y, float width, float height);

    bool IsInside(int x, int y, int z) const;
    size_t CellIndex(int x, int y, int z) const;
    Cell& CellAt(int x, int y, int z);
    const Cell& CellAt(int x, int y, int z) const;
    const BlockDefinition* FindBlock(int id) const;
    void PlaceSelectedBlock();
    void RemoveBlock();
    void RotateBlock();
    void MoveCursor(int dx, int dy, int dz);

    Snapshot MakeSnapshot() const;
    void ApplySnapshot(const Snapshot& snapshot);
    void PushUndo();
    void Undo();
    void Redo();
    void NewStage();

    bool SaveAsNewStage();
    bool LoadStage(const std::string& path);
    void RefreshStageFiles();
    std::string MakeUniqueStagePath(const std::string& requestedName) const;
    static std::string SanitizeFileName(const std::string& name);

    int stageWidth_ = kDefaultWidth;
    int stageHeight_ = kDefaultHeight;
    std::vector<Cell> cells_;

    int cursorX_ = 2;
    int cursorY_ = kDefaultHeight - 4;
    int cursorZ_ = 0;
    int selectedBlockId_ = 1;
    Category category_ = Category::Basic;

    float zoom_ = 1.0f;
    float viewOffsetX_ = 0.0f;
    float viewOffsetY_ = 0.0f;
    bool showGrid_ = true;
    bool gameViewHovered_ = false;

    std::vector<Snapshot> undoStack_;
    std::vector<Snapshot> redoStack_;
    std::vector<std::string> stageFiles_;
    int selectedStageFile_ = -1;

    std::string stageName_ = "stage_01";
    std::string currentFile_;
    std::string status_ = "Ready";
};
