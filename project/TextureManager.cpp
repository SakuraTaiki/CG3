#include "TextureManager.h"
TextureManager* TextureManager::instance = nullptr;

void TextureManager::Initialize() {
	textureDates.reserve(DirectXCommon::kMaxSRVCount);
}

TextureManager* TextureManager::GetInstance() {
	if (instance == nullptr) {
		instance = new TextureManager();
	}
	return instance;
}
void TextureManager::Finalize() {
	delete instance;
	instance = nullptr;
}
