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
	hooker->DebugFrame().GetMemoryMap().AddAllocator(cpuRegs.GPR.n.a0.UL[0], size, name);
    hooker->HaveUpdates();
}

void hookCStackAllocatorDtor() {
    hooker->DebugFrame().GetMemoryMap().RemoveAllocator(cpuRegs.GPR.n.a0.UL[0]);
    hooker->HaveUpdates();
}

void hookIdleStepFrame() { hooker->BeforeFrame(); }

void hookTxrInstanceCtor() { managers.texture->HookInstanceCtor(); }
void hookTxrInstanceDtor() { managers.texture->HookInstanceDtor(); }

void hookMeshInstanceCtor() { managers.mesh->HookInstanceCtor(); }
void hookAllocatorDtor() {
	u32 allocatorOffset = cpuRegs.GPR.n.a0.UL[0];
    managers.mesh->HookAllocatorDtor(allocatorOffset);
}

void hookRenderFlash() { core->Renderer()->RenderFlashes(); }

u32 _renderStaticRenderPass[4];
void hookRenderStaticFetchArgs() {
    _renderStaticRenderPass[0] = cpuRegs.GPR.n.a1.UL[0];
    _renderStaticRenderPass[1] = cpuRegs.GPR.n.a2.UL[0];
    _renderStaticRenderPass[2] = cpuRegs.GPR.n.a3.UL[0];
    _renderStaticRenderPass[3] = cpuRegs.GPR.n.t0.UL[0];
    core->Renderer()->RenderStaticPasses(_renderStaticRenderPass[0], _renderStaticRenderPass[1], _renderStaticRenderPass[2], _renderStaticRenderPass[3]);
}
void hookRenderStatic() {
    u32 renderPass2 = rmem<u32>(cpuRegs.GPR.n.sp.UL[0] + 0x40);
    u32 renderPass1 = cpuRegs.GPR.n.s3.UL[0];
    core->Renderer()->RenderStatic(_renderStaticRenderPass[0], _renderStaticRenderPass[1], _renderStaticRenderPass[2], _renderStaticRenderPass[3]);
}

void hookWadEventAdded() {
    hooker->DebugFrame().GetWadEvents().OnNewEvent(
		cpuRegs.GPR.n.a0.US[0], cpuRegs.GPR.n.a1.US[0],
        cpuRegs.GPR.n.a2.UL[0], pmemz<char>(cpuRegs.GPR.n.a3));
}

void Hooker::InitHooks() {
	DevCon.WriteLn("gow hooker initializing");

    addHook(0x13D710, hookCStackAllocatorCtor);
    addHook(0x13D8A0, hookCStackAllocatorDtor);

    addHook(0x17A9E8, hookIdleStepFrame);

	addHook(0x1665D8, hookTxrInstanceCtor);
    addHook(0x259688, hookTxrInstanceDtor);

	addHook(0x15770C, hookMeshInstanceCtor); // s0 - pMesh, s3 - pczMeshName

	addHook(0x146354, hookRenderFlash); // s5 - pFlash (first element of forward linked list)

	addHook(0x165B04, hookRenderStatic);
    addHook(0x1658F8, hookRenderStaticFetchArgs);

	addHook(0x1BB0F8, hookWadEventAdded);

	addHook(0x13D8A0, hookAllocatorDtor);

    DevCon.WriteLn("gow hooker initialized");
}
