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

	u32 allocatorOffset = cpuRegs.GPR.n.a0.UL[0];
	managers.mesh->HookAllocatorDtor(allocatorOffset);
}

void hookIdleStepFrame() { hooker->BeforeFrame(); }

void hookTxrInstanceCtor() { managers.texture->HookInstanceCtor(); }
void hookTxrInstanceDtor() { managers.texture->HookInstanceDtor(); }

void hookMeshInstanceCtor() { managers.mesh->HookInstanceCtor(); }
void hookAllocatorDtor() {
	u32 allocatorOffset = cpuRegs.GPR.n.a0.UL[0];
    managers.mesh->HookAllocatorDtor(allocatorOffset);
}

void hookRenderFlash() {
	auto flash = pmemz<raw::stRenderFlashUIBase>(cpuRegs.GPR.n.s5);
	core->Renderer()->Master.HookRenderFlash(flash);
}

void hookRender3DFetchArgs() {
    core->Renderer()->Master.HookPass(cpuRegs.pc == 0x1679F8,
		cpuRegs.GPR.n.a1.UL[0], cpuRegs.GPR.n.a2.UL[0], cpuRegs.GPR.n.a3.UL[0], cpuRegs.GPR.n.t0.UL[0]);
}

void hookRenderStatic() {
	auto groupIndexStart = cpuRegs.GPR.n.v0.UL[0];
	auto groupIndexEnd = cpuRegs.GPR.n.v1.UL[0];
	auto binsStructOffset = rmem<u32>(cpuRegs.GPR.n.sp.UL[0] + 0x4C);
	core->Renderer()->Master.HookRenderStatic(groupIndexStart, groupIndexEnd, binsStructOffset);
}

void hookRenderDynamic() {
	auto groupIndexStart = cpuRegs.GPR.n.v0.UL[0];
	auto groupIndexEnd = cpuRegs.GPR.n.v1.UL[0];
	auto binsStructOffset = rmem<u32>(cpuRegs.GPR.n.sp.UL[0] + 0xC0);
	core->Renderer()->Master.HookRenderDynamic(groupIndexStart, groupIndexEnd, binsStructOffset);
}

void hookWadEventAdded() {
    hooker->DebugFrame().GetWadEvents().OnNewEvent(
		cpuRegs.GPR.n.a0.US[0], cpuRegs.GPR.n.a1.US[0], cpuRegs.GPR.n.a2.UL[0], pmemz<char>(cpuRegs.GPR.n.a3));
}

bool hookHashIsUpper;
uint32_t hookHashpStr;
uint32_t hookHashInit;
void hookHashBegin() {
	hookHashIsUpper = (cpuRegs.pc == 0x175780);
	hookHashpStr = cpuRegs.GPR.n.a0.UL[0];
	hookHashInit = cpuRegs.GPR.n.a1.UL[0];
}

void hookHashReturn() {
	uint32_t h = cpuRegs.GPR.n.a1.UL[0];
	auto key = std::pair<uint32_t, uint32_t>(h, hookHashInit);

	if (hooker->hashesMap.find(key) == hooker->hashesMap.end()) {
		std::string s(pmemz<char>(hookHashpStr));
		if (hookHashIsUpper) {
			for (auto & c : s) c = toupper(c);
		}

		hooker->hashesMap.insert({key, s});
        hooker->DebugFrame().GetRenderer().UpdateDumpHashesCount(hooker->hashesMap.size());
	}
}

void hookAddSoundItem() {
    wxString name = wxString(pmemz<char>(cpuRegs.GPR.n.a1.UL[0]));
    uint32_t soundRef = cpuRegs.GPR.n.s1.UL[0];
    //hooker->audioMap.insert({soundRef, name});
    hooker->audioMap[soundRef] = name;
    DevCon.WriteLn(L"Added sound %s 0x%x", WX_STR(name), soundRef);
}

void hookSoundPlay() {
    wxString name;
    uint32_t soundRef = cpuRegs.GPR.n.a1.UL[0];

	if (soundRef) {
        auto iter = hooker->audioMap.find(soundRef);
        if (iter != hooker->audioMap.end()) {
			name = iter->second;
		}
    }

    DevCon.WriteLn(L"Playing sound %s 0x%x", WX_STR(name), soundRef);
}

void Hooker::InitHooks() {
	DevCon.WriteLn("gow hooker initializing");

    addHook(0x13D710, hookCStackAllocatorCtor);
    addHook(0x13D8A0, hookCStackAllocatorDtor);

    addHook(0x17A9E8, hookIdleStepFrame);

	addHook(0x1665D8, hookTxrInstanceCtor);
    addHook(0x259688, hookTxrInstanceDtor);

	addHook(0x15770C, hookMeshInstanceCtor); // s0 - pMesh, s3 - pczMeshName

	addHook(0x146354, hookRenderFlash);

	addHook(0x1658F8, hookRender3DFetchArgs);
	addHook(0x165B04, hookRenderStatic);

	addHook(0x1679F8, hookRender3DFetchArgs);
    addHook(0x167B8C, hookRenderDynamic);

	addHook(0x1BB0F8, hookWadEventAdded);

	addHook(0x175740, hookHashBegin);
	addHook(0x175774, hookHashReturn);
	addHook(0x175780, hookHashBegin);
	addHook(0x1757CC, hookHashReturn);

	addHook(0x172F3C, hookAddSoundItem);
    addHook(0x170A50, hookSoundPlay);

    DevCon.WriteLn("gow hooker initialized");
}
