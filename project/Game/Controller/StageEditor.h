#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

class StageEditor {
public:
    enum class Tool { Select, Paint, Erase, Object };
    enum class ObjectType { PlayerStart, Goal, MovingPlatform };

    struct StageObject {
        uint32_t id = 0;
        ObjectType type = ObjectType::MovingPlatform;
        int gridX = 0;
        int gridY = 0;
        int width = 3;
        int height = 1;
        int endGridX = 6;
        int endGridY = 0;
        float speed = 3.0f;
        float waitSeconds = 0.25f;
        bool pingPong = true;
    };

    void Initialize();
    void Draw();
    void DrawGameView(float rectX, float rectY, float rectWidth, float rectHeight);
    bool IsEditingGameView() const { return !previewPlaying_; }

private:
    struct Snapshot {
        std::vector<int> tiles;
        std::vector<StageObject> objects;
        uint32_t nextObjectId = 1;
    };

    static constexpr int kDefaultWidth = 120;
    static constexpr int kDefaultHeight = 30;
    static constexpr int kTileSize = 32;
    static constexpr int kTileTypeCount = 6;

    void DrawToolbar();
    void DrawPalette();
    void DrawCanvas();
    void DrawInspector();
    void DrawStatusBar();
    void DrawGridAndContents(const std::array<float, 2>& origin,
                             const std::array<float, 2>& size);
    void HandleCanvasInput(const std::array<float, 2>& origin,
                           const std::array<float, 2>& size);

    int TileIndex(int x, int y) const;
    bool IsInside(int x, int y) const;
    void PaintTile(int x, int y, int tileId);
    void PlaceObject(int x, int y);
    int FindObjectAt(int x, int y) const;
    StageObject* SelectedObject();
    const StageObject* SelectedObject() const;

    void BeginEdit();
    void CommitEdit();
    Snapshot MakeSnapshot() const;
    void ApplySnapshot(const Snapshot& snapshot);
    void Undo();
    void Redo();
    void ResetStage();
    bool Save(const std::string& path);
    bool Load(const std::string& path);
    void StartPreview();
    void StopPreview();
    void UpdatePreview(float deltaSeconds);

    const char* TileName(int tileId) const;
    const char* ObjectName(ObjectType type) const;

    int stageWidth_ = kDefaultWidth;
    int stageHeight_ = kDefaultHeight;
    std::vector<int> tiles_;
    std::vector<StageObject> objects_;
    uint32_t nextObjectId_ = 1;

    Tool tool_ = Tool::Paint;
    ObjectType objectType_ = ObjectType::MovingPlatform;
    int selectedTile_ = 1;
    uint32_t selectedObjectId_ = 0;

    float zoom_ = 1.0f;
    float scrollX_ = 0.0f;
    float scrollY_ = 0.0f;
    bool showGrid_ = true;
    bool previewPlaying_ = false;
    float previewTime_ = 0.0f;
    bool strokeActive_ = false;
    bool strokeChanged_ = false;
    Snapshot strokeBefore_;

    std::vector<Snapshot> undoStack_;
    std::vector<Snapshot> redoStack_;
    std::string filePath_ = "Resources/Stages/stage_01.json";
    std::string status_ = "Ready";
};
