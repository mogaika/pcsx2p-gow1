#pragma once

#include "gow/debugFrame.h"

#include <map>

void InitGowHooker();

namespace gow {

class Hooker {
protected:
	DebugFrame *debugFrame;
	bool haveUpdates = false;

public:
    Hooker();

	void HaveUpdates() { haveUpdates = true; }
    DebugFrame &DebugFrame() { return *debugFrame; }
    void InitHooks();
	void BeforeFrame();

	std::map<std::pair<uint32_t, uint32_t>, std::string> hashesMap;
    std::map<uint32_t, wxString> audioMap;
};

extern Hooker *hooker;

} // namespace gow
