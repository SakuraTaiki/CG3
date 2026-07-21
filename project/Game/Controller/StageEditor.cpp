#include "StageEditor.h"

#include "ModelManager.h"
#include "Object3dCommon.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/json/json.hpp"
#endif

namespace {
constexpr float kPi = 3.14159265358979323846f;

const StageEditor::ItemDefinition kItems[] = {
    {1, "Ground", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.32f,0.72f,0.25f,1}, {2.0f,0.5f,1.0f}},
    {2, "Wall", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.72f,0.34f,0.22f,1}, {1.0f,2.0f,1.0f}},
    {3, "Brick", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.72f,0.28f,0.18f,1}, {1.0f,1.0f,1.0f}},
    {4, "PBlock", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.25f,0.55f,0.95f,1}, {1,1,1}},
    {5, "PBlock (On)", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.3f,0.9f,1,1}, {1,1,1}},
    {6, "Crumbling Floor", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.72f,0.55f,0.3f,1}, {1,0.4f,1}},
    {7, "Ice Block", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.65f,0.9f,1,0.85f}, {1,1,1}},
    {8, "Moving Floor", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.62f,0.32f,0.95f,1}, {2,0.3f,1}},
    {9, "Key Block", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.95f,0.72f,0.2f,1}, {1,1,1}},
    {10, "Timed Block", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.9f,0.35f,0.65f,1}, {1,1,1}},
    {11, "On Block", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.95f,0.2f,0.2f,1}, {1,1,1}},
    {12, "Off Block", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.2f,0.45f,0.95f,1}, {1,1,1}},
    {13, "Transparent Block", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.7f,0.8f,1,0.4f}, {1,1,1}},
    {14, "Ladder", StageEditor::Category::Gimmick, "Resources/Editor", "joint_box.obj", {0.65f,0.4f,0.18f,1}, {0.5f,1.5f,0.2f}},
    {15, "Star", StageEditor::Category::Gimmick, "Resources/Editor", "ico_sphere.obj", {1,0.9f,0.15f,1}, {0.45f,0.45f,0.45f}},
    {16, "Bubble Pickup", StageEditor::Category::Gimmick, "Resources/Editor", "ico_sphere.obj", {0.45f,0.85f,1,0.65f}, {0.55f,0.55f,0.55f}},
    {17, "Goal", StageEditor::Category::Gimmick, "Resources/Editor", "joint_box.obj", {1,0.85f,0.2f,1}, {0.3f,2,0.3f}},
    {18, "Door", StageEditor::Category::Gimmick, "Resources/Editor", "joint_box.obj", {0.45f,0.2f,0.08f,1}, {0.8f,1.5f,0.2f}},
    {19, "P Switch", StageEditor::Category::Gimmick, "Resources/Editor", "joint_box.obj", {0.15f,0.45f,0.95f,1}, {0.8f,0.25f,0.8f}},
    {20, "Key", StageEditor::Category::Gimmick, "Resources/Editor", "ico_sphere.obj", {1,0.72f,0.1f,1}, {0.35f,0.35f,0.35f}},
    {21, "On/Off Switch", StageEditor::Category::Gimmick, "Resources/Editor", "joint_box.obj", {0.9f,0.25f,0.25f,1}, {0.8f,0.35f,0.8f}},
    {22, "Enemy Walker", StageEditor::Category::Enemy, "Resources/Editor", "ico_sphere.obj", {0.95f,0.48f,0.2f,1}, {0.6f,0.6f,0.6f}},
    {23, "Enemy Flyer", StageEditor::Category::Enemy, "Resources/Editor", "ico_sphere.obj", {0.85f,0.25f,0.85f,1}, {0.55f,0.55f,0.55f}},
    {24, "Enemy Chaser", StageEditor::Category::Enemy, "Resources/Editor", "ico_sphere.obj", {0.95f,0.15f,0.15f,1}, {0.65f,0.65f,0.65f}},
    {25, "Player Start", StageEditor::Category::System, "Resources/Editor", "ico_sphere.obj", {0.2f,0.72f,1,1}, {0.5f,0.5f,0.5f}},
    {26, "Checkpoint", StageEditor::Category::System, "Resources/Editor", "joint_box.obj", {0.25f,1,0.45f,1}, {0.25f,1.4f,0.25f}},
    {27, "Spike", StageEditor::Category::Gimmick, "Resources/Editor", "ico_sphere.obj", {0.95f,0.18f,0.18f,1}, {0.45f,0.45f,0.45f}},
};

float DistanceSquared(const Vector3& a, const Vector3& b) {
    const float x=a.x-b.x, y=a.y-b.y, z=a.z-b.z;
    return x*x+y*y+z*z;
}
}

void StageEditor::Initialize(Object3dCommon* common, uint32_t environmentTexture, float environmentCoefficient) {
    object3dCommon_=common;
    environmentTexture_=environmentTexture;
    environmentCoefficient_=environmentCoefficient;
    ModelManager::Load("Resources/Editor", "joint_box.obj");
    ModelManager::Load("Resources/Editor", "ico_sphere.obj");
    Model* cursorFrameModel=ModelManager::Load("Resources/Editor","placement_cursor_frame.obj");
    cursorFrameObject_=std::make_unique<Object3d>();
    cursorFrameObject_->Initialize(object3dCommon_);
    cursorFrameObject_->SetModel(cursorFrameModel);
    cursorFrameObject_->SetColor({0.0f,0.0f,0.0f,1.0f});
    cursorFrameObject_->SetEnableLighting(false);
    cursorFrameObject_->SetEnvironmentTexture(environmentTexture_);
    cursorFrameObject_->SetEnvironmentCoefficient(0.0f);
    NewStage();
    RefreshStageFiles();
    LoadPlaylist();
}

void StageEditor::Finalize() {
    cursorFrameObject_.reset();
    cursorObject_.reset();
    objects_.clear();
    placements_.clear();
    object3dCommon_=nullptr;
}

void StageEditor::Update() {
    for (auto& object:objects_) if(object) object->Update();
    if(cursorObject_ && mode_==Mode::Editor) {
        cursorObject_->SetPosition(cursorPosition_);
        cursorObject_->SetRotation(cursorRotation_);
        cursorObject_->SetScale(cursorScale_);
        cursorObject_->Update();
    }
    if(cursorFrameObject_ && mode_==Mode::Editor){
        cursorFrameObject_->SetPosition({std::round(cursorPosition_.x),std::round(cursorPosition_.y),std::round(cursorPosition_.z)});
        cursorFrameObject_->SetRotation({0,0,0});
        cursorFrameObject_->SetScale({1.04f,1.04f,1.04f});
        cursorFrameObject_->Update();
    }
}

void StageEditor::Draw3D() {
    for (auto& object:objects_) if(object) object->Draw();
    if(cursorObject_ && mode_==Mode::Editor) cursorObject_->Draw();
    if(cursorFrameObject_ && mode_==Mode::Editor) cursorFrameObject_->Draw();
}

void StageEditor::Draw() {
#ifdef USE_IMGUI
    ImGui::SetNextWindowSize({330,720},ImGuiCond_FirstUseEver);
    if(!ImGui::Begin("3D Stage Editor")){ImGui::End();return;}
    DrawModeSwitcher();
    ImGui::Separator();
    if(mode_==Mode::Editor){
        DrawPalette();
        DrawSelectedItemSettings();
        DrawTransformPanel();
        ImGui::Separator();
        if(ImGui::Button("PLACE 3D OBJECT",{-1,36}))PlaceItem();
        if(ImGui::Button("REMOVE NEAREST",{-1,30}))RemoveNearest();
        if(ImGui::Button("ROTATE Y 90 deg",{-1,28}))RotateCursor();
        if(ImGui::Button("Undo"))Undo(); ImGui::SameLine(); if(ImGui::Button("Redo"))Redo();
        DrawFilePanel();
        DrawPlaylistManager();
    }else{
        ImGui::TextWrapped("GamePlayMode: placement cursor and editor input are disabled. Placed 3D objects remain in the stage.");
        ImGui::Text("Placed objects: %d",static_cast<int>(placements_.size()));
    }
    ImGui::Separator(); ImGui::TextWrapped("%s",status_.c_str());
    ImGui::End();
#endif
}

void StageEditor::DrawModeSwitcher() {
#ifdef USE_IMGUI
    ImGui::TextUnformatted("MODE");
    int modeIndex=mode_==Mode::Editor?0:1;
    const char* modeNames[]={"StageEditor","GamePlay"};
    if(ImGui::Combo("App Mode",&modeIndex,modeNames,2))SetMode(modeIndex==0?Mode::Editor:Mode::GamePlay);
    ImGui::PushStyleColor(ImGuiCol_Button,mode_==Mode::Editor?ImVec4(.15f,.55f,.9f,1):ImVec4(.18f,.2f,.24f,1));
    if(ImGui::Button("EditorMode",{150,38}))SetMode(Mode::Editor);
    ImGui::PopStyleColor(); ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,mode_==Mode::GamePlay?ImVec4(.18f,.7f,.35f,1):ImVec4(.18f,.2f,.24f,1));
    if(ImGui::Button("GamePlayMode",{150,38}))SetMode(Mode::GamePlay);
    ImGui::PopStyleColor();
#endif
}

void StageEditor::DrawPalette() {
#ifdef USE_IMGUI
    const char* categories[]={"Basic Blocks","Gimmicks & Interactables","Enemies","System"};
    if(ImGui::BeginTabBar("BlockCategoryTabs")){
        for(int categoryIndex=0;categoryIndex<4;++categoryIndex){
            if(ImGui::BeginTabItem(categories[categoryIndex])){
                category_=static_cast<Category>(categoryIndex);
                int column=0;
                for(const auto& item:kItems){
                    if(item.category!=category_)continue;
                    if(column++%2)ImGui::SameLine();
                    ImGui::PushID(item.id);
                    const bool selected=selectedItemId_==item.id;
                    if(selected)ImGui::PushStyleColor(ImGuiCol_Button,{0.2f,0.6f,0.2f,1});
                    if(ImGui::Button(item.name,{145,30})){selectedItemId_=item.id;cursorScale_=item.defaultScale;UpdateCursorObject();}
                    if(selected)ImGui::PopStyleColor();
                    ImGui::PopID();
                }
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
#endif
}

void StageEditor::DrawSelectedItemSettings(){
#ifdef USE_IMGUI
    if(selectedItemId_==8&&ImGui::CollapsingHeader("Moving Floor Settings",ImGuiTreeNodeFlags_DefaultOpen))
        ImGui::DragFloat3("Move Offset",&movingFloorOffset_.x,1.0f,-20.0f,20.0f,"%.0f");
    if(selectedItemId_==18&&ImGui::CollapsingHeader("Door Settings",ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::SliderInt("Door ID Number",&selectedDoorId_,1,9,"ID: %d");ImGui::TextWrapped("Doors with the same ID connect to each other.");
    }
    if((selectedItemId_==4||selectedItemId_==5||selectedItemId_==19)&&ImGui::CollapsingHeader("P Switch Settings",ImGuiTreeNodeFlags_DefaultOpen))
        ImGui::SliderInt("P Switch ID Number",&selectedSwitchId_,1,9,"ID: %d");
    if(selectedItemId_==10&&ImGui::CollapsingHeader("Timed Block Settings",ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::SliderInt("Group ID",&selectedTimedGroupId_,1,9);ImGui::SliderInt("Order ID",&selectedTimedOrderId_,0,9);
    }
#endif
}

void StageEditor::DrawTransformPanel() {
#ifdef USE_IMGUI
    if(!ImGui::CollapsingHeader("Free 3D Placement",ImGuiTreeNodeFlags_DefaultOpen))return;
    ImGui::DragFloat3("Position",&cursorPosition_.x,0.05f);
    ImGui::DragFloat3("Rotation",&cursorRotation_.x,0.01f);
    ImGui::DragFloat3("Scale",&cursorScale_.x,0.02f,0.02f,100.0f);
    ImGui::DragFloat("Move speed",&cursorMoveSpeed_,0.1f,0.1f,30.0f,"%.1f units/s");
    ImGui::TextDisabled("Integer 3D cursor, matching the reference editor.");
    ImGui::TextDisabled("WASD: X/Y   Q/E: Z   Enter: place");
#endif
}

void StageEditor::DrawGameView(float x,float y,float width,float height) {
#ifdef USE_IMGUI
    gameViewHovered_=ImGui::IsItemHovered();
    HandleEditorInput();
    ImDrawList* draw=ImGui::GetWindowDrawList();
    const bool editing=mode_==Mode::Editor;
    const ImU32 color=editing?IM_COL32(38,145,235,235):IM_COL32(35,185,85,235);
    draw->AddRectFilled({x+10,y+10},{x+174,y+40},color,5);
    draw->AddText({x+20,y+18},IM_COL32_WHITE,editing?"EDITOR MODE":"GAMEPLAY MODE");
    if(editing){
        char buffer[160]{};
        snprintf(buffer,sizeof(buffer),"Cursor  %.2f, %.2f, %.2f",cursorPosition_.x,cursorPosition_.y,cursorPosition_.z);
        draw->AddRectFilled({x+10,y+46},{x+250,y+72},IM_COL32(10,12,18,205),4);
        draw->AddText({x+18,y+52},IM_COL32(255,230,70,255),buffer);
    }
#endif
}

void StageEditor::HandleEditorInput() {
#ifdef USE_IMGUI
    ImGuiIO& io=ImGui::GetIO();
    if(mode_!=Mode::Editor||!gameViewHovered_||io.WantTextInput)return;
    if(ImGui::IsKeyPressed(ImGuiKey_A,true))cursorPosition_.x-=1.0f;
    if(ImGui::IsKeyPressed(ImGuiKey_D,true))cursorPosition_.x+=1.0f;
    if(ImGui::IsKeyPressed(ImGuiKey_W,true))cursorPosition_.y+=1.0f;
    if(ImGui::IsKeyPressed(ImGuiKey_S,true))cursorPosition_.y-=1.0f;
    if(ImGui::IsKeyPressed(ImGuiKey_Q,true))cursorPosition_.z-=1.0f;
    if(ImGui::IsKeyPressed(ImGuiKey_E,true))cursorPosition_.z+=1.0f;
    cursorPosition_.x=std::clamp(cursorPosition_.x,0.0f,static_cast<float>(stageWidth_-1));
    cursorPosition_.y=std::clamp(cursorPosition_.y,0.0f,static_cast<float>(stageHeight_-1));
    cursorPosition_.z=std::clamp(cursorPosition_.z,0.0f,static_cast<float>(stageDepth_-1));
    if(ImGui::IsKeyPressed(ImGuiKey_Enter))PlaceItem();
    if(ImGui::IsKeyPressed(ImGuiKey_Space)||ImGui::IsKeyPressed(ImGuiKey_Delete)||ImGui::IsKeyPressed(ImGuiKey_Backspace))RemoveNearest();
    if(ImGui::IsKeyPressed(ImGuiKey_R))RotateCursor();
    if(io.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_Z))Undo();
    if(io.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_Y))Redo();
#endif
}

void StageEditor::SetMode(Mode mode){
    if(mode_==mode)return;
    mode_=mode;
    status_=mode==Mode::Editor?"EditorMode enabled":"GamePlayMode enabled";
}

void StageEditor::ToggleMode(){SetMode(mode_==Mode::Editor?Mode::GamePlay:Mode::Editor);}

void StageEditor::PlaceItem(){
    const ItemDefinition* item=FindItem(selectedItemId_); if(!item)return;
    PushUndo();
    const int existing=FindNearestPlacement(0.25f);
    if(existing>=0){placements_.erase(placements_.begin()+existing);objects_.erase(objects_.begin()+existing);}
    Placement p; p.id=nextPlacementId_++; p.itemId=item->id;
    p.position={std::round(cursorPosition_.x),std::round(cursorPosition_.y),std::round(cursorPosition_.z)};
    p.rotation=cursorRotation_; p.scale=cursorScale_;
    if(item->id==18)p.variant=selectedDoorId_;
    else if(item->id==4||item->id==5||item->id==19)p.variant=selectedSwitchId_;
    else if(item->id==10)p.variant=selectedTimedGroupId_*10+selectedTimedOrderId_;
    if(item->id==8)p.moveOffset=movingFloorOffset_;
    placements_.push_back(p); objects_.push_back(CreateObject(p)); status_="Placed 3D object: "+std::string(item->name);
}

void StageEditor::RemoveNearest(){
    const int index=FindNearestPlacement(0.25f); if(index<0){status_="No object at cursor";return;}
    PushUndo(); placements_.erase(placements_.begin()+index); objects_.erase(objects_.begin()+index); status_="Removed nearest object";
}

void StageEditor::RotateCursor(){
    const int index=FindNearestPlacement(0.25f);
    if(index>=0){PushUndo();placements_[index].rotation.y+=kPi*0.5f;if(placements_[index].rotation.y>=kPi*2)placements_[index].rotation.y-=kPi*2;objects_[index]=CreateObject(placements_[index]);status_="Rotated placed block";return;}
    cursorRotation_.y+=kPi*0.5f;if(cursorRotation_.y>=kPi*2)cursorRotation_.y-=kPi*2;UpdateCursorObject();
}

int StageEditor::FindNearestPlacement(float maxDistance) const {
    int result=-1; float best=maxDistance*maxDistance;
    for(int i=0;i<static_cast<int>(placements_.size());++i){const float d=DistanceSquared(cursorPosition_,placements_[i].position);if(d<best){best=d;result=i;}}
    return result;
}

const StageEditor::ItemDefinition* StageEditor::FindItem(int id) const {for(const auto& item:kItems)if(item.id==id)return &item;return nullptr;}

std::unique_ptr<Object3d> StageEditor::CreateObject(const Placement& p) const {
    const ItemDefinition* item=FindItem(p.itemId); if(!item||!object3dCommon_)return nullptr;
    Model* model=ModelManager::Load(item->modelDirectory,item->modelFile);
    auto object=std::make_unique<Object3d>(); object->Initialize(object3dCommon_); object->SetModel(model);
    object->SetPosition(p.position); object->SetRotation(p.rotation); object->SetScale(p.scale); object->SetColor(item->color);
    object->SetEnvironmentTexture(environmentTexture_); object->SetEnvironmentCoefficient(environmentCoefficient_); return object;
}

void StageEditor::RebuildObjects(){objects_.clear();objects_.reserve(placements_.size());for(const auto& p:placements_)objects_.push_back(CreateObject(p));}

void StageEditor::UpdateCursorObject(){
    const ItemDefinition* item=FindItem(selectedItemId_); if(!item||!object3dCommon_)return;
    Placement p; p.itemId=selectedItemId_;p.position=cursorPosition_;p.rotation=cursorRotation_;p.scale=cursorScale_;
    cursorObject_=CreateObject(p); if(cursorObject_){Vector4 c=item->color;c.w=.42f;cursorObject_->SetColor(c);cursorObject_->SetEnableLighting(false);}
}

StageEditor::Snapshot StageEditor::MakeSnapshot()const{return{placements_,nextPlacementId_};}
void StageEditor::PushUndo(){undoStack_.push_back(MakeSnapshot());if(undoStack_.size()>100)undoStack_.erase(undoStack_.begin());redoStack_.clear();}
void StageEditor::Undo(){if(undoStack_.empty())return;redoStack_.push_back(MakeSnapshot());auto s=undoStack_.back();undoStack_.pop_back();placements_=std::move(s.placements);nextPlacementId_=s.nextId;RebuildObjects();status_="Undo";}
void StageEditor::Redo(){if(redoStack_.empty())return;undoStack_.push_back(MakeSnapshot());auto s=redoStack_.back();redoStack_.pop_back();placements_=std::move(s.placements);nextPlacementId_=s.nextId;RebuildObjects();status_="Redo";}

void StageEditor::NewStage(){placements_.clear();objects_.clear();nextPlacementId_=1;undoStack_.clear();redoStack_.clear();currentFile_.clear();cursorPosition_={0,2,0};cursorRotation_={};const auto* item=FindItem(selectedItemId_);cursorScale_=item?item->defaultScale:Vector3{1,1,1};UpdateCursorObject();status_="New empty 3D stage";}

void StageEditor::DrawFilePanel(){
#ifdef USE_IMGUI
    if(!ImGui::CollapsingHeader("Stage Files",ImGuiTreeNodeFlags_DefaultOpen))return;
    int dimensions[3]={stageWidth_,stageHeight_,stageDepth_};
    if(ImGui::InputInt3("New Stage Size",dimensions)){stageWidth_=std::clamp(dimensions[0],1,500);stageHeight_=std::clamp(dimensions[1],1,500);stageDepth_=std::clamp(dimensions[2],1,100);}
    if(ImGui::Button("NEW EMPTY STAGE",{-1,28}))NewStage();
    char name[96]{};std::copy_n(stageName_.c_str(),(std::min)(stageName_.size(),sizeof(name)-1),name);
    if(ImGui::InputText("Stage name",name,sizeof(name)))stageName_=name;
    if(ImGui::Button("SAVE AS NEW FILE",{-1,34}))SaveAsNewStage();
    ImGui::TextDisabled("Existing files are never overwritten.");
    if(ImGui::Button("Refresh list"))RefreshStageFiles();
    std::string preview=selectedStageFile_>=0&&selectedStageFile_<static_cast<int>(stageFiles_.size())?std::filesystem::path(stageFiles_[selectedStageFile_]).filename().string():"Select stage...";
    if(ImGui::BeginCombo("Load",preview.c_str())){for(int i=0;i<static_cast<int>(stageFiles_.size());++i){std::string f=std::filesystem::path(stageFiles_[i]).filename().string();if(ImGui::Selectable(f.c_str(),selectedStageFile_==i))selectedStageFile_=i;}ImGui::EndCombo();}
    if(selectedStageFile_>=0&&selectedStageFile_<static_cast<int>(stageFiles_.size())&&ImGui::Button("LOAD SELECTED",{-1,28}))LoadStage(stageFiles_[selectedStageFile_]);
#endif
}

void StageEditor::DrawPlaylistManager(){
#ifdef USE_IMGUI
    if(!ImGui::CollapsingHeader("Playlist Manager",ImGuiTreeNodeFlags_DefaultOpen))return;
    ImGui::TextDisabled("Resources/Stages/sequence.txt");
    ImGui::TextUnformatted("Available Stages");
    ImGui::BeginChild("AvailableStages",{0,110},true);
    for(int i=0;i<static_cast<int>(availableFiles_.size());++i)if(ImGui::Selectable(availableFiles_[i].c_str(),selectedAvailableIndex_==i))selectedAvailableIndex_=i;
    ImGui::EndChild();
    if(ImGui::Button("Add to Playlist",{-1,28})&&selectedAvailableIndex_>=0&&selectedAvailableIndex_<static_cast<int>(availableFiles_.size())){campaignFiles_.push_back(availableFiles_[selectedAvailableIndex_]);availableFiles_.erase(availableFiles_.begin()+selectedAvailableIndex_);selectedAvailableIndex_=-1;}
    ImGui::Spacing();
    ImGui::TextUnformatted("Campaign Stages (Playlist)");
    ImGui::BeginChild("CampaignStages",{0,140},true);
    for(int i=0;i<static_cast<int>(campaignFiles_.size());++i){std::string label="Stage "+std::to_string(i+1)+": "+campaignFiles_[i];if(ImGui::Selectable(label.c_str(),selectedCampaignIndex_==i))selectedCampaignIndex_=i;}
    ImGui::EndChild();
    if(ImGui::Button("Remove")&&selectedCampaignIndex_>=0&&selectedCampaignIndex_<static_cast<int>(campaignFiles_.size())){availableFiles_.push_back(campaignFiles_[selectedCampaignIndex_]);campaignFiles_.erase(campaignFiles_.begin()+selectedCampaignIndex_);selectedCampaignIndex_=-1;}
    ImGui::SameLine();if(ImGui::Button("Up")&&selectedCampaignIndex_>0){std::swap(campaignFiles_[selectedCampaignIndex_],campaignFiles_[selectedCampaignIndex_-1]);--selectedCampaignIndex_;}
    ImGui::SameLine();if(ImGui::Button("Down")&&selectedCampaignIndex_>=0&&selectedCampaignIndex_+1<static_cast<int>(campaignFiles_.size())){std::swap(campaignFiles_[selectedCampaignIndex_],campaignFiles_[selectedCampaignIndex_+1]);++selectedCampaignIndex_;}
    ImGui::Separator();if(ImGui::Button("Save Playlist"))SavePlaylist();ImGui::SameLine();if(ImGui::Button("Reload"))LoadPlaylist();
#endif
}

std::string StageEditor::SanitizeFileName(const std::string& value){std::string r;for(unsigned char c:value)if(c>=0x80||std::isalnum(c)||c=='_'||c=='-'||c==' ')r.push_back(static_cast<char>(c));return r.empty()?"stage":r;}
std::string StageEditor::MakeUniqueStagePath()const{namespace fs=std::filesystem;const fs::path dir="Resources/Stages";const std::string base=SanitizeFileName(stageName_);fs::path p=dir/(base+".json");for(int n=2;fs::exists(p);++n)p=dir/(base+"_"+std::to_string(n)+".json");return p.generic_string();}

bool StageEditor::SaveAsNewStage(){
#ifdef USE_IMGUI
    namespace fs=std::filesystem;const std::string path=MakeUniqueStagePath();std::error_code ec;fs::create_directories(fs::path(path).parent_path(),ec);
    nlohmann::json root;root["version"]=4;root["name"]=fs::path(path).stem().string();root["coordinate_system"]="integer_3d_cursor";root["size"]={stageWidth_,stageHeight_,stageDepth_};root["placements"]=nlohmann::json::array();
    for(const auto& p:placements_)root["placements"].push_back({{"id",p.id},{"item_id",p.itemId},{"position",{p.position.x,p.position.y,p.position.z}},{"rotation",{p.rotation.x,p.rotation.y,p.rotation.z}},{"scale",{p.scale.x,p.scale.y,p.scale.z}},{"variant",p.variant},{"move_offset",{p.moveOffset.x,p.moveOffset.y,p.moveOffset.z}}});
    std::ofstream out(path);if(!out){status_="Save failed";return false;}out<<std::setw(2)<<root;currentFile_=path;status_="Saved new stage: "+path;RefreshStageFiles();LoadPlaylist();return true;
#else
    return false;
#endif
}

bool StageEditor::LoadStage(const std::string& path){
#ifdef USE_IMGUI
    try{std::ifstream in(path);if(!in)throw std::runtime_error("file not found");nlohmann::json root;in>>root;std::vector<Placement> loaded;uint32_t maxId=0;
        auto size=root.value("size",std::vector<int>{100,100,20});if(size.size()==3){stageWidth_=(std::max)(1,size[0]);stageHeight_=(std::max)(1,size[1]);stageDepth_=(std::max)(1,size[2]);}
        for(const auto& j:root.value("placements",nlohmann::json::array())){Placement p;p.id=j.value("id",0u);p.itemId=j.value("item_id",0);p.variant=j.value("variant",0);auto pos=j.value("position",std::vector<float>{0,0,0});auto rot=j.value("rotation",std::vector<float>{0,0,0});auto scale=j.value("scale",std::vector<float>{1,1,1});auto move=j.value("move_offset",std::vector<float>{0,3,0});if(pos.size()!=3||rot.size()!=3||scale.size()!=3||move.size()!=3)continue;p.position={pos[0],pos[1],pos[2]};p.rotation={rot[0],rot[1],rot[2]};p.scale={scale[0],scale[1],scale[2]};p.moveOffset={move[0],move[1],move[2]};if(FindItem(p.itemId)){loaded.push_back(p);maxId=(std::max)(maxId,p.id);}}
        placements_=std::move(loaded);nextPlacementId_=maxId+1;currentFile_=path;stageName_=root.value("name",std::filesystem::path(path).stem().string());undoStack_.clear();redoStack_.clear();RebuildObjects();status_="Loaded 3D stage: "+path;return true;
    }catch(const std::exception& e){status_=std::string("Load failed: ")+e.what();return false;}
#else
    return false;
#endif
}

void StageEditor::RefreshStageFiles(){stageFiles_.clear();selectedStageFile_=-1;std::error_code ec;const std::filesystem::path dir="Resources/Stages";if(!std::filesystem::exists(dir,ec))return;for(const auto& e:std::filesystem::directory_iterator(dir,ec))if(e.is_regular_file()&&e.path().extension()==".json")stageFiles_.push_back(e.path().generic_string());std::sort(stageFiles_.begin(),stageFiles_.end());}

void StageEditor::LoadPlaylist(){
    campaignFiles_.clear();availableFiles_.clear();std::ifstream in("Resources/Stages/sequence.txt");std::string line;while(std::getline(in,line))if(!line.empty())campaignFiles_.push_back(line);
    for(const auto& path:stageFiles_){const std::string name=std::filesystem::path(path).filename().string();if(std::find(campaignFiles_.begin(),campaignFiles_.end(),name)==campaignFiles_.end())availableFiles_.push_back(name);}
    selectedCampaignIndex_=selectedAvailableIndex_=-1;
}
void StageEditor::SavePlaylist(){std::filesystem::create_directories("Resources/Stages");std::ofstream out("Resources/Stages/sequence.txt");for(const auto& file:campaignFiles_)out<<file<<'\n';status_="Playlist saved";}
