#pragma once
#include "DirectXCommon.h"

class ModelCommon
{
public:

	void Intialize(DirectXCommon* dxCommon);
	DirectXCommon* GetDxCommon()const { return dxCommon_; }
private:
	DirectXCommon* dxCommon_ = nullptr;
};

