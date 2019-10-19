#include "PrecompiledHeader.h"

#include "gow/gow.h"

using namespace gow;

Core::Core() {
	window = new Window();
	renderer = new Renderer(window);
}

Core::~Core() {
	if (window) {
		window->Destroy();
		delete window;
    }
}

void gow::Core::BeginOfFrame() {
}

void gow::Core::EndOfFrame() {
    renderer->EndOfFrame();
}
