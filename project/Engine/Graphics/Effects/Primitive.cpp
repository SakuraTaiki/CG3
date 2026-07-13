#include "Primitive.h"

#include <cassert>
#include <cstring>
#include <numbers>
#include <random>

#include <algorithm>
#include <cmath>

#include "D3DResourceHelper.h"
#include "EffectMath.h"

using namespace Microsoft::WRL;

namespace
{
    std::random_device primitiveSeedGenerator;

    std::mt19937_64 primitiveRandomEngine(
        primitiveSeedGenerator()
    );
}


#include "PrimitiveLifecycle.h"
#include "PrimitiveUpdate.h"
#include "PrimitiveDraw.h"
#include "PrimitiveEmit.h"
#include "PrimitivePipeline.h"
