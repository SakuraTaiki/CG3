#include "GPUParticleManager.h"

#include <algorithm>
#include <cassert>
#include <cstring>

#include "D3DResourceHelper.h"

using Microsoft::WRL::ComPtr;


#include "GPUParticleManagerLifecycle.h"
#include "GPUParticleManagerUpdate.h"
#include "GPUParticleManagerDraw.h"
#include "GPUParticleManagerEmit.h"
#include "GPUParticleManagerResources.h"
#include "GPUParticleManagerGraphicsPipeline.h"
#include "GPUParticleManagerComputePipeline.h"
#include "GPUParticleManagerUtility.h"
