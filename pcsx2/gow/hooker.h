#pragma once

#include "gow/debugFrame.h"

void InitGowHooker();

namespace gow {

class Hooker {
protected:
	DebugFrame *debugFrame;
	bool haveUpdates = false;
public:
    Hooker();

	void HaveUpdates() { haveUpdates = true; }
    DebugFrame *DebugFrame() { return debugFrame; }
    void InitHooks();
	void BeforeFrame();
};

} // namespace gow
