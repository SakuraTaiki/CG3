#include "SceneDebugPanel.h"

#include "EngineContext.h"
#include "HitEffectController.h"
#include "AnimationDebugController.h"
#include "SoundController.h"
#include "CameraDebugController.h"
#include "EnvironmentController.h"
#include "SceneObjectController.h"
#include "StageEditor.h"
#include "Object3dCommon.h"

#include "Ring.h"
#include "Cylinder.h"
#include "Primitive.h"
#include "ParticleManager.h"
#include "DirectXCommon.h"
#include "ImGuiManager.h"
#include "camera.h"
#include "GPUParticleManager.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <cstring>
#include <functional>
#include <filesystem>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/ImGuizmo.h"
#endif

#include "SceneDebugPanelDetail.h"

namespace Detail = SceneDebugPanelDetail;


#include "SceneDebugPanelWindows.h"
#include "SceneDebugPanelPostEffect.h"
#include "SceneDebugPanelEnvironment.h"
#include "SceneDebugPanelGameView.h"
#include "SceneDebugPanelEffectTab.h"
