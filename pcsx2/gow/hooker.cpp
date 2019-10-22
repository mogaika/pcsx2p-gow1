#include "PrecompiledHeader.h"

#include "x86/iR5900.h"

#include "gow/hooker.h"
#include "gow/gow.h"
#include "gow/utils.h"

using namespace gow;

Hooker *gow::hooker = NULL;

void InitGowHooker() {
	hooker = new Hooker();
}

Hooker::Hooker() {
	InitHooks();

    haveUpdates = false;

    debugFrame = new gow::DebugFrame(wxT("GoW Debugger"));
    debugFrame->Show();

	core = new gow::Core();
};

void Hooker::BeforeFrame() {
	if (haveUpdates) {
        debugFrame->Refresh();
        debugFrame->Update();
        haveUpdates = false;
    }
	core->BeginOfFrame();
	core->EndOfFrame(); // TODO: find own place for end of frame
}

void hookCStackAllocatorCtor() {
    u32 size = cpuRegs.GPR.n.a1.UL[0];
    char *name = pmem<char>(cpuRegs.GPR.n.a2);
	hooker->DebugFrame()->GetMemoryMap()->AddAllocator(cpuRegs.GPR.n.a0.UL[0], size, name);
    hooker->HaveUpdates();
}

void hookCStackAllocatorDtor() {
    hooker->DebugFrame()->GetMemoryMap()->RemoveAllocator(cpuRegs.GPR.n.a0.UL[0]);
    hooker->HaveUpdates();
}

void hookIdleStepFrame() { hooker->BeforeFrame(); }

void hookTxrInstanceCtor() { managers.texture->HookInstanceCtor(); }
void hookTxrInstanceDtor() { managers.texture->HookInstanceDtor(); }

u32 meshLoadingServer;
void hookMeshInstanceCtorCollectServer() { meshLoadingServer = cpuRegs.GPR.n.a0.UL[0]; }

void hookMeshInstanceCtor() { managers.mesh->HookInstanceCtor(meshLoadingServer); }
void hookMeshServerDtor() { managers.mesh->HookServerDtor(); }

void hookRenderFlash() { core->Renderer()->RenderFlashes(); }

void Hooker::InitHooks() {
	DevCon.WriteLn("gow hooker initializing");

    addHook(0x13D710, hookCStackAllocatorCtor);
    addHook(0x13D8A0, hookCStackAllocatorDtor);

    addHook(0x17A9E8, hookIdleStepFrame);

	addHook(0x1665D8, hookTxrInstanceCtor);
    addHook(0x259688, hookTxrInstanceDtor);

	addHook(0x1576BC, hookMeshInstanceCtorCollectServer); // a0 - pServer
	addHook(0x15770C, hookMeshInstanceCtor); // s0 - pMesh, s3 - pczMeshName
	addHook(0x1573E8, hookMeshServerDtor); // a0 - pServer

	addHook(0x146354, hookRenderFlash); // s5 - pFlash (first element of forward linked list)

    DevCon.WriteLn("gow hooker initialized");
}
