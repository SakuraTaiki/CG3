#pragma once

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
        EnvironmentController& environment,
        SceneObjectController& sceneObjects,
        AnimationDebugController& animationDebug
    );
};