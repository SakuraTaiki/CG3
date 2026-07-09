#include "ParticleManager.h"

#include<cmath>
#include <cassert>
#include <random>
#include<numbers>
#include <algorithm>

#include "D3DResourceHelper.h"

using namespace Microsoft::WRL;


static std::random_device seed_gen;
static std::mt19937_64 engine(seed_gen());


#include "ParticleManagerLifecycle.h"
#include "ParticleManagerUpdate.h"
#include "ParticleManagerDraw.h"
#include "ParticleManagerEmit.h"
#include "ParticleManagerPipeline.h"
