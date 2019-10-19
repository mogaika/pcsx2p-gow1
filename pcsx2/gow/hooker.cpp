#include "PrecompiledHeader.h"

#include "x86/iR5900.h"

#include "gow/hooker.h"
#include "gow/debugFrame.h"

using namespace gow;

GowHooker *hooker = NULL;

__inline void * parseAddr(u32 addr) {
    return (void *)(0x20000000 + addr);
}

__inline void *parseAddr(GPR_reg &reg) {
    return parseAddr(reg.UL[0]);
}

void InitGowHooker() {
	hooker = new GowHooker();
}

GowHooker::GowHooker() {
	InitHooks();

    haveUpdates = false;

    debugFrame = new gow::DebugFrame(wxT("GoW Debugger"));
    debugFrame->Show();

	core = new gow::Core();
};

void GowHooker::BeforeFrame() {
	if (haveUpdates) {
        debugFrame->Refresh();
        debugFrame->Update();
        haveUpdates = false;
    }
	core->BeginOfFrame();
	core->EndOfFrame(); // TODO: find own place for end of frame
}

void hookCStackAllocatorCtor() {
    void *allocator = parseAddr(cpuRegs.GPR.n.a0);
    u32 size = cpuRegs.GPR.n.a1.UL[0];
    char *name = (char*)parseAddr(cpuRegs.GPR.n.a2);

	DevCon.WriteLn("gow: creating allocator %x %x %s", allocator, size, name);
	hooker->DebugFrame()->GetMemoryMap()->AddAllocator(cpuRegs.GPR.n.a0.UL[0], size, name);
    hooker->HaveUpdates();
}

void hookCStackAllocatorDtor() {
    void *allocator = parseAddr(cpuRegs.GPR.n.a0);

    DevCon.WriteLn("gow: removing allocator %x", allocator);
    hooker->DebugFrame()->GetMemoryMap()->RemoveAllocator(cpuRegs.GPR.n.a0.UL[0]);
    hooker->HaveUpdates();
}

void hookIdleStepFrame() {
	hooker->BeforeFrame();
}

void GowHooker::InitHooks() {
	DevCon.WriteLn("gow hooker initializing");
    addHook(0x13D710, hookCStackAllocatorCtor);
    addHook(0x13D8A0, hookCStackAllocatorDtor);
    addHook(0x17A9E8, hookIdleStepFrame);
    DevCon.WriteLn("gow hooker initialized");
}
