#include "MyGame.h"
#include <Windows.h>

void MyGame::Initialize() {
    // --- 基盤初期化 ---
    winApp = new WinApp(); 
    winApp->Initialize();

    dxCommon = new DirectXCommon(); 
    dxCommon->Initialize(winApp);

    input = new Input(); 
    input->Initialize(winApp);

    srvManager = new SrvManager();
    srvManager->Initialize(dxCommon);

    textureManager = new TextureManager(); 
    textureManager->Initialize(dxCommon,srvManager);

    spriteCommon = new SpriteCommon(); 
    spriteCommon->SetTextureManager(textureManager);
    spriteCommon->Initialize(dxCommon);

    object3dCommon = new Object3dCommon();
    object3dCommon->SetTextureManager(textureManager);
    object3dCommon->Initialize(dxCommon);

    //カメラ
    camera = new Camera();
    camera->Update();

    object3dCommon->SetDefaultCamera(camera);

    particleManager = new ParticleManager();
    particleManager->Initialize(dxCommon, textureManager);


    // --- モデル読み込み (各1回ずつ) ---
    Model* modelPlane = Model::CreateFromOBJ(dxCommon, "Resources", "plane.obj", textureManager);
    Model* modelAxis = Model::CreateFromOBJ(dxCommon, "Resources", "axis.obj", textureManager);
    models.push_back(modelPlane);
    models.push_back(modelAxis);

    // --- オブジェクト生成 ---
    // 1つ目: 床
    Object3d* floor = CreateObject(modelPlane, { 0.0f, 0.0f, 0.0f });
    floor->SetScale({ 10.0f, 1.0f, 10.0f });

    // 2つ目: 右側の軸
    CreateObject(modelAxis, { 2.0f, 0.0f, 0.0f });

    // 3つ目: 左側の軸
    CreateObject(modelAxis, { -2.0f, 0.0f, 0.0f });

    // スプライト
    uint32_t texHandle = textureManager->LoadTexture("Resources/uvChecker.png");
    sprite = new Sprite();
    sprite->Initialize(spriteCommon, texHandle);

    //skyBox
    skybox = new Skybox();
    skybox->Initialize(dxCommon, textureManager, "Resources/skybox/rostock_laage_airport_4k.dds");
    skybox->SetScale({ 100.0f, 100.0f, 100.0f });

    //サウンド初期化
    sound.Initialize();
    //読み込み
    wavSoundData = sound.SoundLoadFile("Resources/Sound/Alarm01.wav");
    
    mp4SoundData = sound.SoundLoadFile("Resources/Sound/AlarmMovie.mp4");
   
    mp3SoundData = sound.SoundLoadFile("Resources/Sound/maou_bgm_neorock83.mp3");

    
}

Object3d* MyGame::CreateObject(Model* model, Vector3 pos) {
    Object3d* obj = new Object3d();
    obj->Initialize(object3dCommon);
    obj->SetModel(model);
    obj->SetPosition(pos);
    obj->SetRotation({ 1.57f, 0.0f, 0.0f }); // デフォルトで寝かせる
    objectList.push_back(obj); // ここでリストに追加されるので、Draw()で自動描画される
    return obj;
}

void MyGame::Update() {
    input->Update();
    if (input->TriggerKey(DIK_SPACE)) particleManager->Emit({ 0,0,0 }, 10);

    camera->Update();

    const Matrix4x4& view = camera->GetViewMatrix();
    const Matrix4x4& projection = camera->GetProjectionMatrix();
    
    for (Object3d* obj : objectList) {
        obj->Update();
    }

    if (skybox) {
        skybox->SetCamera(view, projection);
        skybox->Update();
    }

    sprite->Update();
    particleManager->Update(view, projection);


    // SPACEでwav再生
    if (input->TriggerKey(DIK_SPACE)) {
        sound.SoundPlay(wavSoundData,wavVolume);
    }

    // Mキーでmp4音声再生
    if (input->TriggerKey(DIK_M)) {
        sound.SoundPlay(mp4SoundData,mp4Volume);
       
    }

    if (input->TriggerKey(DIK_N)) {
        sound.SoundPlay(mp3SoundData,mp3Volume);
    }

    //mp3版音量変更キー
    if (input->TriggerKey(DIK_UP)) {
        mp3Volume += 0.1f;
        if (mp3Volume > 1.0f) {
            mp3Volume = 1.0f;
        }
        OutputDebugStringA("[MyGame] mp3 音量アップ\n");
    }

    if (input->TriggerKey(DIK_DOWN)) {
        mp3Volume -= 0.1f;
        if (mp3Volume < 0.0f) {
            mp3Volume = 0.0f;
        }
        OutputDebugStringA("[MyGame] mp3 音量ダウン\n");
    }

    //ImGui
    
}

void MyGame::Draw() {

    // RenderTextureに3Dシーンを描画
    dxCommon->PreDrawForRenderTexture();

    srvManager->PreDraw();

    // 3D描画 (リスト内の全オブジェクトをループで描画)
    object3dCommon->PreDraw();
    for (Object3d* obj : objectList) {
        obj->Draw();
    }

    if (skybox) {
        skybox->Draw();
    }

    particleManager->Draw();

    dxCommon->PreDraw();

    dxCommon->DrawRenderTextureToSwapChain();

    // 2D描画
    // 2Dスプライトは最終画面に描く
    srvManager->PreDraw();

    spriteCommon->PreDraw();
    sprite->Draw();

    dxCommon->PostDraw();
}

void MyGame::Finalize() {
    delete camera;

    delete skybox;
    delete sprite;

    for (Object3d* obj : objectList) delete obj;
    for (Model* m : models) delete m;

    delete spriteCommon;
    delete particleManager; 
    delete textureManager; 
    delete srvManager;
    delete input;
    delete dxCommon; 
    delete winApp;

    sound.Finalize();
}