#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <vector>
#include <list>
#include "MyMath.h"
#include "DirectXCommon.h"
#include "TextureManager.h"

struct particle
{
	Transform transform;
	Vector3 velocity;
	Vector4 color;
	float lifeTime;
	float maxTime;
};

class ParticleManager
{
public:

	void Initialize();
	void Emit(const std::string name, const Vector3& position, uint32_t count);

};

