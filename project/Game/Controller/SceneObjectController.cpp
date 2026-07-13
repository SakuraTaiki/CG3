#include "SceneObjectController.h"

#include "ModelManager.h"
#include "Object3dCommon.h"
#include "TextureManager.h"
#include "externals/json/json.hpp"

#include <fstream>
#include <functional>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <regex>
#include <sstream>

#include "SceneObjectControllerRuntime.h"
#include "SceneObjectControllerJsonHelpers.h"
#include "SceneObjectControllerSave.h"
#include "SceneObjectControllerLoad.h"
