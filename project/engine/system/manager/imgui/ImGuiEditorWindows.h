#pragma once
#include <map>
#include <memory>
#include <string>

namespace Bonjin {
	class SceneManager;
	class IScene;

	class ImGuiEditorWindows {
	public:
		static void DrawSystemSettings(SceneManager* sceneManager);
		static void DrawGameView(SceneManager* sceneManager);
		static void DrawHierarchy(IScene* currentScene);
	};
}
