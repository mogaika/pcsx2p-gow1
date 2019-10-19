#include "PrecompiledHeader.h"

#include "gow/gow.h"
#include "gow/resources.h"

using namespace gow;

Core *gow::core = nullptr;
ResourceManagers gow::managers;

Core::Core() {
	core = core;
	window = new gow::Window();
	renderer = new gow::Renderer(window);

	managers.Init();
}

Core::~Core() {
	if (window) {
		window->Destroy();
		delete window;
    }
	if (renderer) { delete renderer; }
}

void gow::Core::BeginOfFrame() {
}

void gow::Core::EndOfFrame() {
    renderer->EndOfFrame();
}

void ResourceManagers::Init() {
	texture = new TextureManager();
}
