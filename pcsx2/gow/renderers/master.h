#pragma once

#include "gow/renderers/flp.h"
#include "gow/renderers/tristripmesh.h"

#include "gow/resources/renderer.h"

namespace gow {
	class Renderer;
namespace renderers {

class Master {
protected:
	struct stPass {
		stPass(u32 pass1, u32 pass2, u32 pass3, u32 pass4);
		stPass(): stPass(-1, -1, -1, -1) {};
		union {
			u8 pass[4];
			u32 _uid;
		};
		explicit operator u32() { return _uid; }
		u8 operator [] (u32 i) { return pass[i]; }
	};

	stPass currentPass;
	int step;
	bool maskMirrors;
	Renderer &r;
public:
	Flp Flp;
	TriStripMesh TriStripMesh;

	Master(Renderer &r);

	void Setup();
	void HookPass(bool isDynamic, u32 renderPass1, u32 renderPass2, u32 renderPass3, u32 renderPass4);
	void HookRenderStatic(u32 groupIndexStart, u32 groupIndexEnd, u32 binsStructOffset) {
		TriStripMesh.Render(r, false, maskMirrors, groupIndexStart, groupIndexEnd, binsStructOffset);
	}
	void HookRenderDynamic(u32 groupIndexStart, u32 groupIndexEnd, u32 binsStructOffset) {
		TriStripMesh.Render(r, true, maskMirrors, groupIndexStart, groupIndexEnd, binsStructOffset);
	}
	void HookRenderFlash(raw::stRenderFlashUIBase *flash) { Flp.Render(r, flash); }
	void FrameBegin();
	void FrameEnd();

	bool blueClearColor;
};

} // namespace renderers
} // namespace gow

/*
	render pass 0:1:0:0
	render pass 0:1:2:0
	render pass 0:1:4:0

	render pass 0:2:0:0
		- fake reflection
	render pass 0:2:2:0
		- fake reflection substract
	render pass 0:2:4:0
		- fake reflection additive
	render pass 0:3:0:0
	render pass 0:4:0:0

	render pass 1:0:0:0
		- mirrors (who reflects)
	render pass 1:0:1:0
	render pass 1:0:2:0
	render pass 1:0:4:0
	
	render pass 1:1:0:0
		- sky (diff?)
	render pass 1:1:1:0
		- sky (diff?), transparent
	
	render pass 1:0:0:0
		- mirrors (who reflects)
	render pass 1:0:1:0
	render pass 1:0:2:0
	render pass 1:0:4:0
	
	render pass 1:2:0:1
		- normal models, non transparent
	render pass 1:2:1:1
		- normal models, transparent
	render pass 1:2:0:2
		- normal models, non transparent?? hero? entity? hero, waterfall0**, MAIBlade
	render pass 1:2:1:2
	render pass 1:2:2:0
	render pass 1:2:4:0
		- water
		- normal models, additive
		- lightRays

	render pass 1:3:0:0
		- normal models, doublelayered? (second layer additive?)
	render pass 1:3:1:0
	render pass 1:3:2:0
	render pass 1:3:4:0
		- SavePoint_0, additive

	
	render pass 1:4:0:0
	render pass 1:4:1:0
	render pass 1:4:2:0
	render pass 1:4:4:0
*/