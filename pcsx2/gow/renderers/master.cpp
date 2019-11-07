#include "PrecompiledHeader.h"

#include "gow/gow.h"
#include "gow/renderers/master.h"
#include "gow/renderer.h"

using namespace gow;

renderers::Master::Master(Renderer &r):
	r(r), blueClearColor(false) {
}

renderers::Master::stPass::stPass(u32 pass1, u32 pass2, u32 pass3, u32 pass4) {
	pass[0] = pass1;
	pass[1] = pass2;
	pass[2] = pass3;
	pass[3] = pass4;
}

void renderers::Master::HookPass(bool isDynamic, u32 renderPass1, u32 renderPass2, u32 renderPass3, u32 renderPass4) {
	auto dump = r.LogDumpPush("Master");
	auto contextGl = r.Window().AttachContext();

	stPass prevPass = currentPass;
	currentPass = stPass(renderPass1, renderPass2, renderPass3, renderPass4);

	/*
	steps:
		1 - prepare reflections render
		2 - prepare stensil buffers to mask mirrors (do not draw depth + color)
		3 - prepare sky render (use setnsil buffer to mask mirrors)
		4 - prepare world render: depth clear after sky, disable stensil
	*/

	int newStep = step;

	if (u32(currentPass) != u32(prevPass)) {
		if (currentPass[0] == 0) {
			if (step <= 1) {
				newStep = 1;
			}
		} else if (currentPass[0] == 1) {
			if (currentPass[1] == 0) {
				if (step <= 2) {
					newStep = 2;
				} else if (step <= 4) {
					newStep = 4;
				}
			} else if (currentPass[1] == 1) {
				if (step <= 3) {
					newStep = 3;
				}
			} else {
				if (step <= 4) {
					newStep = 4;
				}
			}
		}
	}

	if (newStep != step) {
		dump.Log(Color_Blue, "Step changed to 0x%x (prev: 0x%x)", newStep, step);
		writeDepth = true;
		switch (newStep) {
		case 1:
		case 4:
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glDepthMask(GL_TRUE);
			glDisable(GL_STENCIL_TEST);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

			glClear(GL_DEPTH_BUFFER_BIT);
			break;
		case 2:
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
			glDepthMask(GL_FALSE);
			glEnable(GL_STENCIL_TEST);
			glStencilFunc(GL_ALWAYS, 1, 0xFF);
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
			writeDepth = false;

			glClear(GL_STENCIL_BUFFER_BIT);
			break;
		case 3:
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glDepthMask(GL_TRUE);
			glEnable(GL_STENCIL_TEST);
			glStencilFunc(GL_EQUAL, 0, 0xFF);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

			glClear(GL_DEPTH_BUFFER_BIT);
			break;
		default:
			DevCon.Error("RenderMaster: newStep == 0x%x", newStep);
		}
		step = newStep;
	}

	dump.Log(Color_Black, "----------- [frame 0x%07x %s pass (render pass %d:%d:%d:%d)",
		offsets::uFrameCounter, isDynamic ? "Dynamic" : "Static",
		renderPass1, renderPass2, renderPass3, renderPass4);
}

void renderers::Master::Setup() {
	Flp.Setup(r);
	TriStripMesh.Setup(r);
}

void renderers::Master::FrameBegin() {
	step = 0;
	currentPass = stPass();

	glClearColor(0.0, 0.0, blueClearColor ? 1.0 : 0.0, 1.0);
	glClearDepth(50000.0);
	glClear(GL_COLOR_BUFFER_BIT);

	r.CheckErrors("FrameBegin");
}

void renderers::Master::FrameEnd() {

	r.CheckErrors("FrameEnd");
}
