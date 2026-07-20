#include "StageEditor.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/json/json.hpp"
#endif

namespace {
constexpr StageEditor::BlockDefinition kBlocks[] = {
    { 1, "Ground",         StageEditor::Category::Basic,   0xFF54B548u, false },
    { 2, "Wall",           StageEditor::Category::Basic,   0xFF536AB8u, true  },
    { 3, "Brick",          StageEditor::Category::Basic,   0xFF3769B5u, false },
    { 4, "Question",       StageEditor::Category::Basic,   0xFF31B5EDu, false },
    { 5, "Ice Block",      StageEditor::Category::Basic,   0xFFE7D7B9u, false },
    { 6, "Hazard",         StageEditor::Category::Gimmick, 0xFF4E4EE0u, true  },
    { 7, "Moving Lift",    StageEditor::Category::Gimmick, 0xFFFF70ABu, true  },
    { 8, "Crumbling Floor",StageEditor::Category::Gimmick, 0xFF5387D6u, false },
    { 9, "Enemy Spawn",    StageEditor::Category::Enemy,   0xFF6262F0u, true  },
    {10, "Player Start",   StageEditor::Category::System,  0xFFFFC64Bu, true  },
    {11, "Goal",           StageEditor::Category::System,  0xFF55DFFFu, true  },
};

#ifdef USE_IMGUI
ImU32 ToImColor(uint32_t abgr) { return static_cast<ImU32>(abgr); }

void CenteredText(ImDrawList* draw, const ImVec2& min, const ImVec2& max,
                  ImU32 color, const char* text) {
    const ImVec2 size = ImGui::CalcTextSize(text);
    draw->AddText({ (min.x + max.x - size.x) * 0.5f,
                    (min.y + max.y - size.y) * 0.5f }, color, text);
}
#endif
}

void StageEditor::Initialize() {
    NewStage();
    RefreshStageFiles();
}

void StageEditor::NewStage() {
    stageWidth_ = kDefaultWidth;
    stageHeight_ = kDefaultHeight;
    cells_.assign(static_cast<size_t>(stageWidth_ * stageHeight_ * kDepthLayers), {});
    cursorX_ = 2;
    cursorY_ = stageHeight_ - 4;
    cursorZ_ = 0;
    currentFile_.clear();
    undoStack_.clear();
    redoStack_.clear();
    viewOffsetX_ = 0.0f;
    viewOffsetY_ = 0.0f;
    status_ = "New unsaved stage";
}

void StageEditor::Draw() {
#ifdef USE_IMGUI
    if (cells_.empty()) Initialize();
    DrawStageEditorWindow();
#endif
}

void StageEditor::DrawStageEditorWindow() {
#ifdef USE_IMGUI
    ImGui::SetNextWindowSize({ 360.0f, 720.0f }, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Stage Editor")) { ImGui::End(); return; }

    ImGui::Text("Cursor  X:%d  Y:%d  Z:%d", cursorX_, cursorY_, cursorZ_);
    const BlockDefinition* selected = FindBlock(selectedBlockId_);
    ImGui::Text("Block: %s", selected ? selected->name : "None");
    ImGui::Separator();

    DrawCategoryTabs();
    DrawBlockPalette();
    ImGui::Separator();

    if (ImGui::Button("PLACE  (Enter)", { -1.0f, 38.0f })) PlaceSelectedBlock();
    if (ImGui::Button("REMOVE  (Space)", { -1.0f, 34.0f })) RemoveBlock();
    if (ImGui::Button("ROTATE  (R)", { -1.0f, 32.0f })) RotateBlock();

    ImGui::Separator();
    if (ImGui::Button("Undo", { 82.0f, 0.0f })) Undo();
    ImGui::SameLine();
    if (ImGui::Button("Redo", { 82.0f, 0.0f })) Redo();
    ImGui::SameLine();
    ImGui::Checkbox("Grid", &showGrid_);
    ImGui::SetNextItemWidth(150.0f);
    ImGui::SliderFloat("Zoom", &zoom_, 0.4f, 2.5f, "%.2fx");

    DrawFilePanel();
    DrawHelpPanel();
    ImGui::Separator();
    ImGui::TextWrapped("%s", status_.c_str());
    ImGui::End();
#endif
}

void StageEditor::DrawCategoryTabs() {
#ifdef USE_IMGUI
    const char* names[] = { "Basic", "Gimmick", "Enemy", "System" };
    for (int i = 0; i < 4; ++i) {
        if (i) ImGui::SameLine();
        if (ImGui::Selectable(names[i], static_cast<int>(category_) == i, 0, { 78.0f, 26.0f }))
            category_ = static_cast<Category>(i);
    }
#endif
}

void StageEditor::DrawBlockPalette() {
#ifdef USE_IMGUI
    int column = 0;
    for (const BlockDefinition& block : kBlocks) {
        if (block.category != category_) continue;
        ImGui::PushID(block.id);
        if (column++ % 2) ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::ColorConvertU32ToFloat4(ToImColor(block.color)));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::ColorConvertU32ToFloat4(ToImColor(block.color) | 0x00202020u));
        if (ImGui::Button(block.name, { 158.0f, 38.0f })) selectedBlockId_ = block.id;
        ImGui::PopStyleColor(2);
        if (selectedBlockId_ == block.id) {
            ImGui::GetWindowDrawList()->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(),
                                                IM_COL32(255,255,255,255), 3.0f, 0, 3.0f);
        }
        ImGui::PopID();
    }
#endif
}

void StageEditor::DrawFilePanel() {
#ifdef USE_IMGUI
    if (!ImGui::CollapsingHeader("Stage Files", ImGuiTreeNodeFlags_DefaultOpen)) return;
    if (ImGui::Button("NEW EMPTY STAGE", { -1.0f, 30.0f })) NewStage();
    char name[96]{};
    std::copy_n(stageName_.c_str(), std::min(stageName_.size(), sizeof(name) - 1), name);
    if (ImGui::InputText("Stage name", name, sizeof(name))) stageName_ = name;
    if (ImGui::Button("SAVE AS NEW FILE", { -1.0f, 36.0f })) SaveAsNewStage();
    ImGui::TextDisabled("Existing files are never overwritten.");

    if (ImGui::Button("Refresh list")) RefreshStageFiles();
    std::string previewStorage = selectedStageFile_ >= 0 && selectedStageFile_ < static_cast<int>(stageFiles_.size())
        ? std::filesystem::path(stageFiles_[selectedStageFile_]).filename().string() : "Select stage...";
    if (ImGui::BeginCombo("Load", previewStorage.c_str())) {
        for (int i = 0; i < static_cast<int>(stageFiles_.size()); ++i) {
            const std::string fileName = std::filesystem::path(stageFiles_[i]).filename().string();
            if (ImGui::Selectable(fileName.c_str(), selectedStageFile_ == i)) selectedStageFile_ = i;
        }
        ImGui::EndCombo();
    }
    if (selectedStageFile_ >= 0 && selectedStageFile_ < static_cast<int>(stageFiles_.size())) {
        if (ImGui::Button("LOAD SELECTED", { -1.0f, 30.0f })) LoadStage(stageFiles_[selectedStageFile_]);
    }
#endif
}

void StageEditor::DrawHelpPanel() {
#ifdef USE_IMGUI
    if (!ImGui::CollapsingHeader("Controls")) return;
    ImGui::TextUnformatted("W/A/S/D : Move cursor X/Y");
    ImGui::TextUnformatted("Q / E     : Change depth Z");
    ImGui::TextUnformatted("Enter     : Place block");
    ImGui::TextUnformatted("Space     : Remove block");
    ImGui::TextUnformatted("R         : Rotate block");
    ImGui::TextUnformatted("Middle drag / wheel : Pan / zoom");
#endif
}

void StageEditor::DrawGameView(float x, float y, float width, float height) {
#ifdef USE_IMGUI
    if (cells_.empty() || width <= 1.0f || height <= 1.0f) return;
    gameViewHovered_ = ImGui::IsItemHovered();
    DrawOverlay(x, y, width, height);
    HandleMouse(x, y, width, height);
    HandleKeyboard();
#endif
}

void StageEditor::DrawOverlay(float x, float y, float width, float height) {
#ifdef USE_IMGUI
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->PushClipRect({x,y}, {x+width,y+height}, true);
    const float cellSize = kTilePixels * zoom_;
    const float baseX = x - viewOffsetX_;
    const float baseY = y - viewOffsetY_;
    const int firstX = std::max(0, static_cast<int>(viewOffsetX_ / cellSize));
    const int firstY = std::max(0, static_cast<int>(viewOffsetY_ / cellSize));
    const int lastX = std::min(stageWidth_, firstX + static_cast<int>(width / cellSize) + 2);
    const int lastY = std::min(stageHeight_, firstY + static_cast<int>(height / cellSize) + 2);

    for (int z = kDepthLayers - 1; z >= 0; --z) {
        for (int gy = firstY; gy < lastY; ++gy) {
            for (int gx = firstX; gx < lastX; ++gx) {
                const Cell& cell = CellAt(gx, gy, z);
                const BlockDefinition* block = FindBlock(cell.blockId);
                if (!block) continue;
                const float depthOffset = static_cast<float>(z - cursorZ_) * 4.0f;
                ImVec2 min{baseX + gx * cellSize + depthOffset, baseY + gy * cellSize - depthOffset};
                ImVec2 max{min.x + cellSize, min.y + cellSize};
                ImU32 color = ToImColor(block->color);
                if (z != cursorZ_) color = (color & 0x00FFFFFFu) | 0x78000000u;
                draw->AddRectFilled(min, max, color, 2.0f);
                draw->AddRect(min, max, IM_COL32(20,20,24,190), 2.0f);
                if (zoom_ >= 0.85f) CenteredText(draw, min, max, IM_COL32(255,255,255,230), block->name);
                if (cell.rotationQuarterTurns) {
                    const ImVec2 center{(min.x+max.x)*0.5f,(min.y+max.y)*0.5f};
                    const float a = cell.rotationQuarterTurns * 1.5707963f;
                    draw->AddLine(center, {center.x + std::cos(a)*cellSize*0.35f,
                                           center.y + std::sin(a)*cellSize*0.35f}, IM_COL32_WHITE, 3.0f);
                }
            }
        }
    }

    if (showGrid_) {
        for (int gx = firstX; gx <= lastX; ++gx) {
            const float px = baseX + gx * cellSize;
            draw->AddLine({px,y},{px,y+height},IM_COL32(255,255,255,28));
        }
        for (int gy = firstY; gy <= lastY; ++gy) {
            const float py = baseY + gy * cellSize;
            draw->AddLine({x,py},{x+width,py},IM_COL32(255,255,255,28));
        }
    }

    const ImVec2 cursorMin{baseX + cursorX_ * cellSize, baseY + cursorY_ * cellSize};
    const ImVec2 cursorMax{cursorMin.x + cellSize, cursorMin.y + cellSize};
    draw->AddRectFilled(cursorMin, cursorMax, IM_COL32(255,255,255,38));
    draw->AddRect(cursorMin, cursorMax, IM_COL32(255,235,64,255), 2.0f, 0, 4.0f);
    draw->AddLine({cursorMin.x-6,cursorMin.y},{cursorMin.x+8,cursorMin.y},IM_COL32(255,235,64,255),3.0f);
    draw->AddLine({cursorMin.x,cursorMin.y-6},{cursorMin.x,cursorMin.y+8},IM_COL32(255,235,64,255),3.0f);

    std::ostringstream info;
    info << "CURSOR  X:" << cursorX_ << "  Y:" << cursorY_ << "  Z:" << cursorZ_;
    draw->AddRectFilled({x+10,y+10},{x+245,y+38},IM_COL32(12,16,24,210),5.0f);
    draw->AddText({x+18,y+17},IM_COL32(255,235,64,255),info.str().c_str());
    draw->PopClipRect();
#endif
}

void StageEditor::HandleKeyboard() {
#ifdef USE_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    if (!gameViewHovered_ || io.WantTextInput) return;
    if (ImGui::IsKeyPressed(ImGuiKey_A, true)) MoveCursor(-1,0,0);
    if (ImGui::IsKeyPressed(ImGuiKey_D, true)) MoveCursor( 1,0,0);
    if (ImGui::IsKeyPressed(ImGuiKey_W, true)) MoveCursor(0,-1,0);
    if (ImGui::IsKeyPressed(ImGuiKey_S, true)) MoveCursor(0, 1,0);
    if (ImGui::IsKeyPressed(ImGuiKey_Q, true)) MoveCursor(0,0,-1);
    if (ImGui::IsKeyPressed(ImGuiKey_E, true)) MoveCursor(0,0, 1);
    if (ImGui::IsKeyPressed(ImGuiKey_Enter)) PlaceSelectedBlock();
    if (ImGui::IsKeyPressed(ImGuiKey_Space)) RemoveBlock();
    if (ImGui::IsKeyPressed(ImGuiKey_R)) RotateBlock();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z)) Undo();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y)) Redo();
#endif
}

void StageEditor::HandleMouse(float x, float y, float, float) {
#ifdef USE_IMGUI
    ImGuiIO& io = ImGui::GetIO();
    if (!gameViewHovered_) return;
    if (io.MouseWheel != 0.0f) zoom_ = std::clamp(zoom_ + io.MouseWheel * 0.1f, 0.4f, 2.5f);
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        viewOffsetX_ = std::max(0.0f, viewOffsetX_ - io.MouseDelta.x);
        viewOffsetY_ = std::max(0.0f, viewOffsetY_ - io.MouseDelta.y);
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const float size = kTilePixels * zoom_;
        MoveCursor(static_cast<int>((io.MousePos.x - x + viewOffsetX_) / size) - cursorX_,
                   static_cast<int>((io.MousePos.y - y + viewOffsetY_) / size) - cursorY_, 0);
    }
#endif
}

bool StageEditor::IsInside(int x, int y, int z) const {
    return x >= 0 && x < stageWidth_ && y >= 0 && y < stageHeight_ && z >= 0 && z < kDepthLayers;
}
size_t StageEditor::CellIndex(int x, int y, int z) const {
    return static_cast<size_t>((z * stageHeight_ + y) * stageWidth_ + x);
}
StageEditor::Cell& StageEditor::CellAt(int x, int y, int z) { return cells_[CellIndex(x,y,z)]; }
const StageEditor::Cell& StageEditor::CellAt(int x, int y, int z) const { return cells_[CellIndex(x,y,z)]; }
const StageEditor::BlockDefinition* StageEditor::FindBlock(int id) const {
    for (const auto& block : kBlocks) if (block.id == id) return &block;
    return nullptr;
}

void StageEditor::MoveCursor(int dx, int dy, int dz) {
    cursorX_ = std::clamp(cursorX_ + dx, 0, stageWidth_ - 1);
    cursorY_ = std::clamp(cursorY_ + dy, 0, stageHeight_ - 1);
    cursorZ_ = std::clamp(cursorZ_ + dz, 0, kDepthLayers - 1);
}
void StageEditor::PlaceSelectedBlock() {
    if (!IsInside(cursorX_,cursorY_,cursorZ_)) return;
    Cell& cell = CellAt(cursorX_,cursorY_,cursorZ_);
    if (cell.blockId == selectedBlockId_) return;
    PushUndo(); cell.blockId = selectedBlockId_; cell.rotationQuarterTurns = 0; status_ = "Block placed";
}
void StageEditor::RemoveBlock() {
    Cell& cell = CellAt(cursorX_,cursorY_,cursorZ_);
    if (!cell.blockId) return;
    PushUndo(); cell = {}; status_ = "Block removed";
}
void StageEditor::RotateBlock() {
    Cell& cell = CellAt(cursorX_,cursorY_,cursorZ_);
    const BlockDefinition* block = FindBlock(cell.blockId);
    if (!block || !block->rotatable) return;
    PushUndo(); cell.rotationQuarterTurns = (cell.rotationQuarterTurns + 1) % 4; status_ = "Block rotated";
}

StageEditor::Snapshot StageEditor::MakeSnapshot() const { return {cells_,cursorX_,cursorY_,cursorZ_}; }
void StageEditor::ApplySnapshot(const Snapshot& s) { cells_=s.cells; cursorX_=s.cursorX; cursorY_=s.cursorY; cursorZ_=s.cursorZ; }
void StageEditor::PushUndo() { undoStack_.push_back(MakeSnapshot()); if (undoStack_.size()>100) undoStack_.erase(undoStack_.begin()); redoStack_.clear(); }
void StageEditor::Undo() { if(undoStack_.empty())return; redoStack_.push_back(MakeSnapshot()); ApplySnapshot(undoStack_.back()); undoStack_.pop_back(); status_="Undo"; }
void StageEditor::Redo() { if(redoStack_.empty())return; undoStack_.push_back(MakeSnapshot()); ApplySnapshot(redoStack_.back()); redoStack_.pop_back(); status_="Redo"; }

std::string StageEditor::SanitizeFileName(const std::string& name) {
    std::string result;
    for (unsigned char c : name) {
        if (c >= 0x80 || std::isalnum(c) || c == '_' || c == '-' || c == ' ')
            result.push_back(static_cast<char>(c));
    }
    return result.empty() ? "stage" : result;
}
std::string StageEditor::MakeUniqueStagePath(const std::string& requestedName) const {
    namespace fs = std::filesystem;
    const fs::path directory = "Resources/Stages";
    const std::string base = SanitizeFileName(requestedName);
    fs::path candidate = directory / (base + ".json");
    for (int suffix=2; fs::exists(candidate); ++suffix) candidate = directory / (base + "_" + std::to_string(suffix) + ".json");
    return candidate.generic_string();
}

bool StageEditor::SaveAsNewStage() {
#ifdef USE_IMGUI
    namespace fs = std::filesystem;
    const std::string path = MakeUniqueStagePath(stageName_);
    std::error_code error; fs::create_directories(fs::path(path).parent_path(), error);
    nlohmann::json root;
    root["version"]=2; root["name"]=fs::path(path).stem().string();
    root["width"]=stageWidth_; root["height"]=stageHeight_; root["depth_layers"]=kDepthLayers;
    root["placements"]=nlohmann::json::array();
    for(int z=0;z<kDepthLayers;++z) for(int y=0;y<stageHeight_;++y) for(int x=0;x<stageWidth_;++x) {
        const Cell& cell=CellAt(x,y,z); if(!cell.blockId)continue;
        root["placements"].push_back({{"x",x},{"y",y},{"z",z},{"block_id",cell.blockId},{"rotation",cell.rotationQuarterTurns}});
    }
    std::ofstream output(path); if(!output){status_="Save failed";return false;}
    output<<std::setw(2)<<root; currentFile_=path; status_="Saved new stage: "+path; RefreshStageFiles(); return true;
#else
    return false;
#endif
}

bool StageEditor::LoadStage(const std::string& path) {
#ifdef USE_IMGUI
    try {
        std::ifstream input(path); if(!input)throw std::runtime_error("file not found");
        nlohmann::json root; input>>root;
        const int width=root.value("width",kDefaultWidth), height=root.value("height",kDefaultHeight);
        if(width<=0||height<=0)throw std::runtime_error("invalid stage size");
        stageWidth_=width; stageHeight_=height;
        cells_.assign(static_cast<size_t>(stageWidth_*stageHeight_*kDepthLayers),{});
        for(const auto& p:root.value("placements",nlohmann::json::array())) {
            const int x=p.value("x",-1),y=p.value("y",-1),z=p.value("z",0);
            if(IsInside(x,y,z)) CellAt(x,y,z)={p.value("block_id",0),p.value("rotation",0)};
        }
        currentFile_=path; stageName_=root.value("name",std::filesystem::path(path).stem().string());
        cursorX_=cursorY_=cursorZ_=0; undoStack_.clear(); redoStack_.clear(); status_="Loaded: "+path; return true;
    } catch(const std::exception& e){status_=std::string("Load failed: ")+e.what();return false;}
#else
    return false;
#endif
}

void StageEditor::RefreshStageFiles() {
    stageFiles_.clear(); selectedStageFile_=-1;
    std::error_code error; const std::filesystem::path directory="Resources/Stages";
    if(!std::filesystem::exists(directory,error))return;
    for(const auto& entry:std::filesystem::directory_iterator(directory,error))
        if(entry.is_regular_file()&&entry.path().extension()==".json")stageFiles_.push_back(entry.path().generic_string());
    std::sort(stageFiles_.begin(),stageFiles_.end());
}
