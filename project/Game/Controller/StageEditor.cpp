#include "StageEditor.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/json/json.hpp"
#endif

namespace {
#ifdef USE_IMGUI
ImU32 TileColor(int id) {
    switch (id) {
    case 1: return IM_COL32(104, 173, 72, 255);
    case 2: return IM_COL32(181, 103, 55, 255);
    case 3: return IM_COL32(237, 177, 48, 255);
    case 4: return IM_COL32(206, 214, 224, 255);
    case 5: return IM_COL32(224, 78, 72, 255);
    default: return IM_COL32(0, 0, 0, 0);
    }
}

void DrawCenteredText(ImDrawList* draw, const ImVec2& min, const ImVec2& max,
                      ImU32 color, const char* text) {
    const ImVec2 textSize = ImGui::CalcTextSize(text);
    draw->AddText({ (min.x + max.x - textSize.x) * 0.5f,
                    (min.y + max.y - textSize.y) * 0.5f }, color, text);
}
#endif
}

void StageEditor::Initialize() {
    tiles_.assign(stageWidth_ * stageHeight_, 0);
    for (int y = stageHeight_ - 3; y < stageHeight_; ++y) {
        for (int x = 0; x < 24; ++x) {
            tiles_[TileIndex(x, y)] = 1;
        }
    }
    objects_.push_back({ nextObjectId_++, ObjectType::PlayerStart, 2, stageHeight_ - 5 });
    StageObject goal{ nextObjectId_++, ObjectType::Goal, 20, stageHeight_ - 6 };
    goal.width = 1;
    goal.height = 3;
    objects_.push_back(goal);
    undoStack_.clear();
    redoStack_.clear();
    status_ = "New stage created";
}

void StageEditor::Draw() {
#ifdef USE_IMGUI
    if (tiles_.empty()) Initialize();
    if (previewPlaying_) UpdatePreview(ImGui::GetIO().DeltaTime);

    ImGui::SetNextWindowSize({ 330.0f, 720.0f }, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Stage Tools")) {
        ImGui::End();
        return;
    }
    DrawToolbar();
    ImGui::Separator();
    ImGui::BeginChild("Palette", { 0.0f, 280.0f }, true);
    DrawPalette();
    ImGui::EndChild();
    ImGui::BeginChild("Inspector", { 0.0f, -26.0f }, true);
    DrawInspector();
    ImGui::EndChild();
    DrawStatusBar();
    ImGui::End();
#endif
}

void StageEditor::DrawGameView(float rectX, float rectY, float rectWidth, float rectHeight) {
#ifdef USE_IMGUI
    if (tiles_.empty() || rectWidth <= 1.0f || rectHeight <= 1.0f) return;
    DrawGridAndContents({ rectX, rectY }, { rectWidth, rectHeight });
    if (!previewPlaying_) HandleCanvasInput({ rectX, rectY }, { rectWidth, rectHeight });

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const char* mode = previewPlaying_ ? "TEST PREVIEW" : "EDIT MODE";
    const ImU32 color = previewPlaying_ ? IM_COL32(61, 205, 114, 235) : IM_COL32(66, 155, 255, 235);
    const ImVec2 labelMin{ rectX + 10.0f, rectY + 10.0f };
    const ImVec2 labelMax{ labelMin.x + 118.0f, labelMin.y + 27.0f };
    draw->AddRectFilled(labelMin, labelMax, color, 5.0f);
    DrawCenteredText(draw, labelMin, labelMax, IM_COL32(255, 255, 255, 255), mode);
#endif
}

void StageEditor::DrawToolbar() {
#ifdef USE_IMGUI
    if (ImGui::Button("New")) ResetStage();
    ImGui::SameLine();
    if (ImGui::Button("Save")) Save(filePath_);
    ImGui::SameLine();
    if (ImGui::Button("Load")) Load(filePath_);
    ImGui::SameLine();
    ImGui::BeginDisabled(undoStack_.empty() || previewPlaying_);
    if (ImGui::Button("Undo")) Undo();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(redoStack_.empty() || previewPlaying_);
    if (ImGui::Button("Redo")) Redo();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::TextUnformatted("|");
    ImGui::SameLine();
    if (!previewPlaying_) {
        if (ImGui::Button("Play Preview")) StartPreview();
    } else {
        if (ImGui::Button("Stop Preview")) StopPreview();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Grid", &showGrid_);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120.0f);
    ImGui::SliderFloat("Zoom", &zoom_, 0.35f, 2.5f, "%.2fx");
    ImGui::TextDisabled("Edit directly in Game View");
    ImGui::TextDisabled("LMB edit | RMB erase | MMB pan | Wheel zoom");
#endif
}

void StageEditor::DrawPalette() {
#ifdef USE_IMGUI
    ImGui::TextUnformatted("TOOLS");
    const char* tools[] = { "Select", "Paint", "Erase", "Object" };
    for (int i = 0; i < 4; ++i) {
        if (ImGui::Selectable(tools[i], static_cast<int>(tool_) == i)) tool_ = static_cast<Tool>(i);
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextUnformatted("TILES");
    for (int id = 1; id < kTileTypeCount; ++id) {
        ImGui::PushID(id);
        const ImVec4 color = ImGui::ColorConvertU32ToFloat4(TileColor(id));
        ImGui::ColorButton("tile", color, ImGuiColorEditFlags_NoTooltip, { 24, 24 });
        ImGui::SameLine();
        if (ImGui::Selectable(TileName(id), selectedTile_ == id)) {
            selectedTile_ = id;
            tool_ = Tool::Paint;
        }
        ImGui::PopID();
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextUnformatted("OBJECTS");
    const ObjectType types[] = { ObjectType::PlayerStart, ObjectType::Goal, ObjectType::MovingPlatform };
    for (ObjectType type : types) {
        if (ImGui::Selectable(ObjectName(type), objectType_ == type && tool_ == Tool::Object)) {
            objectType_ = type;
            tool_ = Tool::Object;
        }
    }
#endif
}

void StageEditor::DrawCanvas() {
#ifdef USE_IMGUI
    ImGui::Text("Stage %d x %d", stageWidth_, stageHeight_);
    const ImVec2 available = ImGui::GetContentRegionAvail();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("stage_canvas", available,
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
    DrawGridAndContents({ origin.x, origin.y }, { available.x, available.y });
    if (!previewPlaying_) HandleCanvasInput({ origin.x, origin.y }, { available.x, available.y });
#endif
}

void StageEditor::DrawGridAndContents(const std::array<float, 2>& origin,
                                      const std::array<float, 2>& size) {
#ifdef USE_IMGUI
    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 canvasMin{ origin[0], origin[1] };
    const ImVec2 canvasMax{ origin[0] + size[0], origin[1] + size[1] };
    draw->PushClipRect(canvasMin, canvasMax, true);
    const float cell = kTileSize * zoom_;
    const float baseX = origin[0] - scrollX_;
    const float baseY = origin[1] - scrollY_;

    const int firstX = std::max(0, static_cast<int>(std::floor(scrollX_ / cell)));
    const int firstY = std::max(0, static_cast<int>(std::floor(scrollY_ / cell)));
    const int lastX = std::min(stageWidth_, firstX + static_cast<int>(size[0] / cell) + 2);
    const int lastY = std::min(stageHeight_, firstY + static_cast<int>(size[1] / cell) + 2);
    for (int y = firstY; y < lastY; ++y) {
        for (int x = firstX; x < lastX; ++x) {
            const int tile = tiles_[TileIndex(x, y)];
            if (tile == 0) continue;
            ImVec2 min{ baseX + x * cell, baseY + y * cell };
            ImVec2 max{ min.x + cell, min.y + cell };
            draw->AddRectFilled(min, max, TileColor(tile));
            draw->AddRect(min, max, IM_COL32(255, 255, 255, 45));
            if (zoom_ >= 0.7f) DrawCenteredText(draw, min, max, IM_COL32(30, 30, 30, 190), TileName(tile));
        }
    }

    for (const StageObject& object : objects_) {
        float drawX = static_cast<float>(object.gridX);
        float drawY = static_cast<float>(object.gridY);
        if (previewPlaying_ && object.type == ObjectType::MovingPlatform) {
            const float dx = static_cast<float>(object.endGridX - object.gridX);
            const float dy = static_cast<float>(object.endGridY - object.gridY);
            const float distance = std::max(0.001f, std::sqrt(dx * dx + dy * dy));
            const float travel = distance / std::max(0.1f, object.speed);
            float phase = std::fmod(previewTime_, travel * 2.0f) / travel;
            if (phase > 1.0f) phase = 2.0f - phase;
            drawX += dx * phase;
            drawY += dy * phase;
        }
        ImVec2 min{ baseX + drawX * cell, baseY + drawY * cell };
        ImVec2 max{ min.x + object.width * cell, min.y + object.height * cell };
        ImU32 color = object.type == ObjectType::PlayerStart ? IM_COL32(64, 190, 255, 255)
                      : object.type == ObjectType::Goal ? IM_COL32(255, 218, 70, 255)
                      : IM_COL32(171, 110, 255, 255);
        draw->AddRectFilled(min, max, color, 4.0f);
        draw->AddRect(min, max, object.id == selectedObjectId_ ? IM_COL32_WHITE : IM_COL32(20, 20, 20, 220),
                      4.0f, 0, object.id == selectedObjectId_ ? 3.0f : 1.0f);
        DrawCenteredText(draw, min, max, IM_COL32(20, 24, 32, 255), ObjectName(object.type));
        if (object.type == ObjectType::MovingPlatform && !previewPlaying_) {
            ImVec2 start{ min.x + object.width * cell * 0.5f, min.y + object.height * cell * 0.5f };
            ImVec2 end{ baseX + (object.endGridX + object.width * 0.5f) * cell,
                        baseY + (object.endGridY + object.height * 0.5f) * cell };
            draw->AddLine(start, end, IM_COL32(213, 184, 255, 220), 2.0f);
            draw->AddCircleFilled(end, 5.0f, IM_COL32(213, 184, 255, 255));
        }
    }

    if (showGrid_) {
        for (int x = firstX; x <= lastX; ++x) {
            const float px = baseX + x * cell;
            draw->AddLine({ px, std::max(canvasMin.y, baseY + firstY * cell) },
                          { px, std::min(canvasMax.y, baseY + lastY * cell) }, IM_COL32(255, 255, 255, 25));
        }
        for (int y = firstY; y <= lastY; ++y) {
            const float py = baseY + y * cell;
            draw->AddLine({ std::max(canvasMin.x, baseX + firstX * cell), py },
                          { std::min(canvasMax.x, baseX + lastX * cell), py }, IM_COL32(255, 255, 255, 25));
        }
    }
    draw->PopClipRect();
#endif
}

void StageEditor::HandleCanvasInput(const std::array<float, 2>& origin,
                                    const std::array<float, 2>&) {
#ifdef USE_IMGUI
    const bool hovered = ImGui::IsItemHovered();
    ImGuiIO& io = ImGui::GetIO();
    if (hovered && io.MouseWheel != 0.0f) {
        zoom_ = std::clamp(zoom_ + io.MouseWheel * 0.1f, 0.35f, 2.5f);
    }
    if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        scrollX_ = std::max(0.0f, scrollX_ - io.MouseDelta.x);
        scrollY_ = std::max(0.0f, scrollY_ - io.MouseDelta.y);
    }
    if (!hovered && !strokeActive_) return;

    const float cell = kTileSize * zoom_;
    const int x = static_cast<int>(std::floor((io.MousePos.x - origin[0] + scrollX_) / cell));
    const int y = static_cast<int>(std::floor((io.MousePos.y - origin[1] + scrollY_) / cell));

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        BeginEdit();
        strokeActive_ = true;
        if (tool_ == Tool::Select) {
            const int index = FindObjectAt(x, y);
            selectedObjectId_ = index >= 0 ? objects_[index].id : 0;
        } else if (tool_ == Tool::Object && IsInside(x, y)) {
            PlaceObject(x, y);
            strokeChanged_ = true;
        }
    }
    if (strokeActive_ && ImGui::IsMouseDown(ImGuiMouseButton_Left) && IsInside(x, y)) {
        if (tool_ == Tool::Paint && tiles_[TileIndex(x, y)] != selectedTile_) {
            PaintTile(x, y, selectedTile_); strokeChanged_ = true;
        } else if (tool_ == Tool::Erase && tiles_[TileIndex(x, y)] != 0) {
            PaintTile(x, y, 0); strokeChanged_ = true;
        }
    }
    if (strokeActive_ && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) CommitEdit();

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        const int index = FindObjectAt(x, y);
        if (index >= 0) {
            BeginEdit();
            objects_.erase(objects_.begin() + index);
            selectedObjectId_ = 0;
            strokeChanged_ = true;
            CommitEdit();
        } else if (IsInside(x, y) && tiles_[TileIndex(x, y)] != 0) {
            BeginEdit(); PaintTile(x, y, 0); strokeChanged_ = true; CommitEdit();
        }
    }
#endif
}

void StageEditor::DrawInspector() {
#ifdef USE_IMGUI
    ImGui::TextUnformatted("INSPECTOR");
    ImGui::Separator();
    StageObject* object = SelectedObject();
    if (!object) {
        ImGui::TextWrapped("Select an object on the canvas to edit its settings.");
        ImGui::Spacing();
        ImGui::Text("Selected tile: %s", TileName(selectedTile_));
    } else {
        ImGui::Text("%s  #%u", ObjectName(object->type), object->id);
        Snapshot before = MakeSnapshot();
        bool changed = false;
        changed |= ImGui::DragInt("Grid X", &object->gridX, 1.0f, 0, stageWidth_ - 1);
        changed |= ImGui::DragInt("Grid Y", &object->gridY, 1.0f, 0, stageHeight_ - 1);
        changed |= ImGui::DragInt("Width", &object->width, 1.0f, 1, 32);
        changed |= ImGui::DragInt("Height", &object->height, 1.0f, 1, 16);
        if (object->type == ObjectType::MovingPlatform) {
            ImGui::Separator();
            ImGui::TextUnformatted("MOVEMENT");
            changed |= ImGui::DragInt("End X", &object->endGridX, 1.0f, 0, stageWidth_ - 1);
            changed |= ImGui::DragInt("End Y", &object->endGridY, 1.0f, 0, stageHeight_ - 1);
            changed |= ImGui::DragFloat("Speed", &object->speed, 0.1f, 0.1f, 30.0f, "%.1f tiles/s");
            changed |= ImGui::DragFloat("Wait", &object->waitSeconds, 0.05f, 0.0f, 10.0f, "%.2f s");
            changed |= ImGui::Checkbox("Ping pong", &object->pingPong);
        }
        if (changed) {
            undoStack_.push_back(std::move(before));
            redoStack_.clear();
        }
        ImGui::Spacing();
        if (ImGui::Button("Delete object")) {
            BeginEdit();
            objects_.erase(std::remove_if(objects_.begin(), objects_.end(),
                [&](const StageObject& item) { return item.id == selectedObjectId_; }), objects_.end());
            selectedObjectId_ = 0; strokeChanged_ = true; CommitEdit();
        }
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::TextUnformatted("FILE");
    char path[256]{};
    std::copy_n(filePath_.c_str(), std::min(filePath_.size(), sizeof(path) - 1), path);
    if (ImGui::InputText("##path", path, sizeof(path))) filePath_ = path;
    ImGui::TextWrapped("Save data contains tile IDs and object parameters only; shared textures are not duplicated per stage.");
#endif
}

void StageEditor::DrawStatusBar() {
#ifdef USE_IMGUI
    ImGui::TextDisabled("%s | Tiles: %zu | Objects: %zu | Undo: %zu",
                        status_.c_str(), std::count_if(tiles_.begin(), tiles_.end(), [](int v) { return v != 0; }),
                        objects_.size(), undoStack_.size());
#endif
}

int StageEditor::TileIndex(int x, int y) const { return y * stageWidth_ + x; }
bool StageEditor::IsInside(int x, int y) const { return x >= 0 && y >= 0 && x < stageWidth_ && y < stageHeight_; }
void StageEditor::PaintTile(int x, int y, int tileId) { if (IsInside(x, y)) tiles_[TileIndex(x, y)] = tileId; }

void StageEditor::PlaceObject(int x, int y) {
    StageObject object;
    object.id = nextObjectId_++;
    object.type = objectType_;
    object.gridX = x;
    object.gridY = y;
    if (object.type == ObjectType::PlayerStart) { object.width = 1; object.height = 2; }
    if (object.type == ObjectType::Goal) { object.width = 1; object.height = 3; }
    object.endGridX = std::min(stageWidth_ - object.width, x + 6);
    object.endGridY = y;
    objects_.push_back(object);
    selectedObjectId_ = object.id;
}

int StageEditor::FindObjectAt(int x, int y) const {
    for (int i = static_cast<int>(objects_.size()) - 1; i >= 0; --i) {
        const auto& o = objects_[i];
        if (x >= o.gridX && y >= o.gridY && x < o.gridX + o.width && y < o.gridY + o.height) return i;
    }
    return -1;
}

StageEditor::StageObject* StageEditor::SelectedObject() {
    auto it = std::find_if(objects_.begin(), objects_.end(), [&](const StageObject& o) { return o.id == selectedObjectId_; });
    return it == objects_.end() ? nullptr : &*it;
}
const StageEditor::StageObject* StageEditor::SelectedObject() const {
    auto it = std::find_if(objects_.begin(), objects_.end(), [&](const StageObject& o) { return o.id == selectedObjectId_; });
    return it == objects_.end() ? nullptr : &*it;
}

void StageEditor::BeginEdit() { strokeBefore_ = MakeSnapshot(); strokeChanged_ = false; }
void StageEditor::CommitEdit() {
    if (strokeChanged_) { undoStack_.push_back(std::move(strokeBefore_)); redoStack_.clear(); status_ = "Edited"; }
    strokeActive_ = false; strokeChanged_ = false;
}
StageEditor::Snapshot StageEditor::MakeSnapshot() const { return { tiles_, objects_, nextObjectId_ }; }
void StageEditor::ApplySnapshot(const Snapshot& s) { tiles_ = s.tiles; objects_ = s.objects; nextObjectId_ = s.nextObjectId; selectedObjectId_ = 0; }
void StageEditor::Undo() { if (undoStack_.empty()) return; redoStack_.push_back(MakeSnapshot()); ApplySnapshot(undoStack_.back()); undoStack_.pop_back(); status_ = "Undo"; }
void StageEditor::Redo() { if (redoStack_.empty()) return; undoStack_.push_back(MakeSnapshot()); ApplySnapshot(redoStack_.back()); redoStack_.pop_back(); status_ = "Redo"; }
void StageEditor::ResetStage() { Initialize(); scrollX_ = scrollY_ = 0.0f; previewPlaying_ = false; }

bool StageEditor::Save(const std::string& path) {
#ifdef USE_IMGUI
    nlohmann::json root;
    root["version"] = 1;
    root["name"] = std::filesystem::path(path).stem().string();
    root["tile_size"] = kTileSize;
    root["width"] = stageWidth_;
    root["height"] = stageHeight_;
    root["tiles"] = tiles_;
    root["objects"] = nlohmann::json::array();
    for (const auto& o : objects_) {
        root["objects"].push_back({
            {"id", o.id}, {"type", static_cast<int>(o.type)}, {"x", o.gridX}, {"y", o.gridY},
            {"width", o.width}, {"height", o.height}, {"end_x", o.endGridX}, {"end_y", o.endGridY},
            {"speed", o.speed}, {"wait", o.waitSeconds}, {"ping_pong", o.pingPong}
        });
    }
    std::error_code error;
    const auto parent = std::filesystem::path(path).parent_path();
    if (!parent.empty()) std::filesystem::create_directories(parent, error);
    std::ofstream output(path);
    if (!output) { status_ = "Save failed: " + path; return false; }
    output << root.dump(2);
    status_ = "Saved: " + path;
    return true;
#else
    return false;
#endif
}

bool StageEditor::Load(const std::string& path) {
#ifdef USE_IMGUI
    std::ifstream input(path);
    if (!input) { status_ = "Load failed: " + path; return false; }
    try {
        nlohmann::json root; input >> root;
        const int width = root.value("width", kDefaultWidth);
        const int height = root.value("height", kDefaultHeight);
        std::vector<int> loadedTiles = root.at("tiles").get<std::vector<int>>();
        if (width <= 0 || height <= 0 || loadedTiles.size() != static_cast<size_t>(width * height)) throw std::runtime_error("invalid stage size");
        std::vector<StageObject> loadedObjects;
        uint32_t maxId = 0;
        for (const auto& item : root.value("objects", nlohmann::json::array())) {
            StageObject o;
            o.id = item.value("id", 0u); o.type = static_cast<ObjectType>(item.value("type", 2));
            o.gridX = item.value("x", 0); o.gridY = item.value("y", 0);
            o.width = item.value("width", 1); o.height = item.value("height", 1);
            o.endGridX = item.value("end_x", o.gridX); o.endGridY = item.value("end_y", o.gridY);
            o.speed = item.value("speed", 3.0f); o.waitSeconds = item.value("wait", 0.25f);
            o.pingPong = item.value("ping_pong", true);
            maxId = std::max(maxId, o.id); loadedObjects.push_back(o);
        }
        stageWidth_ = width; stageHeight_ = height; tiles_ = std::move(loadedTiles); objects_ = std::move(loadedObjects);
        nextObjectId_ = maxId + 1; undoStack_.clear(); redoStack_.clear(); selectedObjectId_ = 0;
        status_ = "Loaded: " + path; return true;
    } catch (const std::exception& e) { status_ = std::string("Load failed: ") + e.what(); return false; }
#else
    return false;
#endif
}

void StageEditor::StartPreview() { previewPlaying_ = true; previewTime_ = 0.0f; selectedObjectId_ = 0; status_ = "Preview playing"; }
void StageEditor::StopPreview() { previewPlaying_ = false; previewTime_ = 0.0f; status_ = "Preview stopped"; }
void StageEditor::UpdatePreview(float dt) { previewTime_ += std::min(dt, 0.1f); }

const char* StageEditor::TileName(int id) const {
    static const char* names[] = { "Empty", "Ground", "Brick", "Question", "Ice", "Hazard" };
    return id >= 0 && id < kTileTypeCount ? names[id] : "Unknown";
}
const char* StageEditor::ObjectName(ObjectType type) const {
    switch (type) { case ObjectType::PlayerStart: return "Player"; case ObjectType::Goal: return "Goal"; default: return "Moving Lift"; }
}
