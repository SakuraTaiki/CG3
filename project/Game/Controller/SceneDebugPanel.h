#pragma once

#include <cstddef>

#include "GameSceneDrawMode.h"

class EngineContext;
class HitEffectController;
class AnimationDebugController;
class SoundController;
class CameraDebugController;
class EnvironmentController;
class SceneObjectController;
class Ring;
class Cylinder;
class Primitive;
class Object3dCommon;

class SceneDebugPanel {
public:
    void Draw(
        EngineContext* context,
        GameSceneDrawMode& drawMode,
        HitEffectController& hitEffect,
        AnimationDebugController& animationDebug,
        SoundController& soundController,
        CameraDebugController& cameraDebug,
        EnvironmentController& environment,
        SceneObjectController& sceneObjects,
        Ring* ring,
        Cylinder* cylinder,
        Primitive* primitive
    );

private:

    enum class EditorSelection
    {

        None,
        Terrain,
        AxisPositive,
        AxisNegative,
        Camera,
        Environment,
        Effects,
        PostEffect,
        Sound,
        Asset,
        DynamicObject
    };

private:

    void DrawMainMenuBar(
        GameSceneDrawMode& drawMode,
        HitEffectController& hitEffect,
        AnimationDebugController& animationDebug
    );

    void DrawHierarchyWindow(SceneObjectController& sceneObjects);

    void DrawInspectorWindow(
        EngineContext* context,
        EnvironmentController& environment,
        SceneObjectController& sceneObjects,
        AnimationDebugController& animationDebug
    );

    void DrawProjectWindow();

    void DrawConsoleWindow();

    void DrawDebugToolsWindow(
        EngineContext* context,
        GameSceneDrawMode& drawMode,
        HitEffectController& hitEffect,
        AnimationDebugController& animationDebug,
        SoundController& soundController,
        EnvironmentController& environment,
        SceneObjectController& sceneObjects,
        Ring* ring,
        Cylinder* cylinder,
        Primitive* primitive
    );


    void DrawSceneControl(
        GameSceneDrawMode& drawMode,
        HitEffectController& hitEffect,
        AnimationDebugController& animationDebug
    );

    void DrawEffectTab(
        EngineContext* context,
        HitEffectController& hitEffect,
        Ring* ring,
        Cylinder* cylinder,
        Primitive* primitive
    );

    void DrawPostEffectTab(EngineContext* context);

    void DrawEnvironmentTab(
        Object3dCommon* object3dCommon,
        EnvironmentController& environment,
        SceneObjectController& sceneObjects,
        AnimationDebugController& animationDebug
    );


    void DrawGameViewWindow(EngineContext* context, SceneObjectController& sceneObjects);

    void DrawTransformGizmo(
        EngineContext* context,
        SceneObjectController& sceneObjects,
        float rectX,
        float rectY,
        float rectWidth,
        float rectHeight
    );

private:
    EditorSelection selected_ = EditorSelection::Terrain;
    size_t selectedObjectIndex_ = 0;

    bool showTransformGizmo_ = true;
    int gizmoOperation_ = 0; // 0: Move, 1: Rotate, 2: Scale
    int gizmoMode_ = 0;      // 0: Local, 1: World
};
