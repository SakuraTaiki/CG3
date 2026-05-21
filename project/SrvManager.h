#pragma once
#include"DirectXCommon.h"
class SrvManager
{
public:
	void Initialize(DirectXCommon*dxCommon);

private:
	DirectXCommon* directXCommon = nullptr;

};

