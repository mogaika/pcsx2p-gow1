#pragma once

#include "gow/debugFrame.h"
#include "gow/gow.h"

void InitGowHooker();

namespace gow {

class GowHooker {
protected:
	DebugFrame *debugFrame;
	bool haveUpdates = false;
    Core *core;
public:
    GowHooker();

	Core *Core() { return core; }
	void HaveUpdates() { haveUpdates = true; }
    DebugFrame *DebugFrame() { return debugFrame; }
    void InitHooks();
	void BeforeFrame();
};

} // namespace gow
