#include "IScene.h"
#include <algorithm>
#include "../graphics/3d/object/BaseObject.h"

using namespace Bonjin;

IScene::~IScene() {
	for (BaseObject* obj : sceneObjects_) {
		if (obj) {
			obj->SetParentScene(nullptr);
		}
	}
}

void IScene::RegisterObject(BaseObject* obj) {
	sceneObjects_.push_back(obj);
}

void IScene::UnregisterObject(BaseObject* obj) {
	auto it = std::find(sceneObjects_.begin(), sceneObjects_.end(), obj);
	if (it != sceneObjects_.end()) {
		sceneObjects_.erase(it);
	}
}

void IScene::Initialize(Camera* camera) {
	assert(camera);
	camera_ = camera;
}



