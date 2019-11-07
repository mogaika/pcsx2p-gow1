#include "PrecompiledHeader.h"

#include "gow/gow.h"

#include "gow/renderers/flp.h"

#include "gow/resources/renderer.h"

using namespace gow;

void renderers::Flp::Setup(Renderer &r) {
	uniforms.matrix = r.shader_flash.GetUniformLocation("uMatrix");
	uniforms.useTexture = r.shader_flash.GetUniformLocation("uUseTexture");
	uniforms.blendColor = r.shader_flash.GetUniformLocation("uBlendColor");
	uniforms.texYScale = r.shader_flash.GetUniformLocation("uTexYScale");
}

void renderers::Flp::Render(Renderer &r, raw::stRenderFlashUIBase *firstFlash) {
	auto glContext = r.Window().AttachContext();

	r.shader_flash.Use();

	glClear(GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glDepthMask(false);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glActiveTexture(GL_TEXTURE0);

	auto dump = r.LogDumpPush("FLP");

	float blendColors[4] = {1.0, 1.0, 1.0, 1.0};

	int renderedFlashes = 0;
	int renderedCommands = 0;
    for (auto flash = pmemz<raw::stRenderFlashUIBase>(cpuRegs.GPR.n.s5); flash != 0; flash = (raw::stRenderFlashUIBase*) flash->next()) {
        renderedCommands++;
		switch (flash->type) {
            case 0: {
                renderedFlashes++;
                auto draw = (raw::stRenderFlashUIDraw *)flash;
                auto matrix = draw->matrix();

				auto flpData2 = draw->flpRawData2();
				auto flp = draw->flpInstance();
				auto model = flp->getModel();
				auto mesh = model->getMesh(0);
				auto part = mesh->getPart(flpData2->meshPartIndex);
				auto group = part->getGroup(0);

                glUniformMatrix4fv(uniforms.matrix, 1, GL_FALSE, matrix);
				
				dump.Log(Color_Black, "draw mesh 0x%x (%d)", flpData2->meshPartIndex, flpData2->meshPartIndex);

				for (u32 iObject = 0; iObject < group->objectsCount; iObject++) {
                    auto flpDataSub2 = flpData2->getData2Sub1(iObject);
					auto object = group->getObject(iObject);
					
					bool hasTexture = flpDataSub2->txrInstanceOffset != 0;

					if (hasTexture) {
						auto texture = managers.texture->GetTexture(flpDataSub2->txrInstanceOffset);
						if (!texture) {
                            DevCon.Error("gow: render: flp: wasn't able to find texture 0x%x", flpDataSub2->txrInstanceOffset);
                        }
						glUniform1i(uniforms.useTexture, 1);
						glBindTexture(GL_TEXTURE_2D, texture->GetGl(0));
                        glUniform1f(uniforms.texYScale, texture->GetYScale());

						glUniform4f(uniforms.blendColor, blendColors[0], blendColors[1], blendColors[2], blendColors[3]);

						dump.Log(Color_Red, "draw textured %f %f %f %f", blendColors[0], blendColors[1], blendColors[2], blendColors[3]);
                    } else {
                        glUniform1i(uniforms.useTexture, 0);
						glBindTexture(GL_TEXTURE_2D, 0);

						const float u8tofloat = (1.0f / 255.0f);
                        float fBlendColors[4];

						// flpDataSub2->blendColor has BGR order 
						fBlendColors[0] = blendColors[0] * float(flpDataSub2->blendColor[2]) * u8tofloat;
                        fBlendColors[1] = blendColors[1] * float(flpDataSub2->blendColor[1]) * u8tofloat;
                        fBlendColors[2] = blendColors[2] * float(flpDataSub2->blendColor[0]) * u8tofloat;
                        fBlendColors[3] = blendColors[3] * float(flpDataSub2->blendColor[3]) * u8tofloat;

                        glUniform4f(uniforms.blendColor, fBlendColors[0], fBlendColors[1], fBlendColors[2], fBlendColors[3]);

						dump.Log(Color_Red, "draw untextured %f %f %f %f (0x%02x 0x%02x 0x%02x 0x%02x)",
							fBlendColors[0], fBlendColors[1], fBlendColors[2], fBlendColors[3],
							flpDataSub2->blendColor[0], flpDataSub2->blendColor[1], flpDataSub2->blendColor[2], flpDataSub2->blendColor[3]);
                    }

					auto meshObject = object->meshObject();
					auto programs = meshObject->GetPrograms()[0];

					for (auto vao = programs.begin(); vao != programs.end(); ++vao) {
                        r.CheckErrors("before bind");
                        glBindVertexArray(vao->vao);
						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao->ebo);
                        r.CheckErrors("before draw elements");
                        glDrawElements(GL_TRIANGLES, vao->indexesCount, GL_UNSIGNED_SHORT, 0);
						r.CheckErrors("draw elements");
                    }
				}
				} break;
            case 1: {
                auto color = (raw::stRenderFlashUISetColor*) flash;
                const float convu16tofloat = (1.0f / 256.0f);
				dump.Log(Color_Blue, "set color 0x%03x 0x%03x 0x%03x 0x%03x", color->color[0], color->color[1], color->color[2], color->color[3]);
                blendColors[0] = float(color->color[0]) * convu16tofloat;
                blendColors[1] = float(color->color[1]) * convu16tofloat;
                blendColors[2] = float(color->color[2]) * convu16tofloat;
                blendColors[3] = float(color->color[3]) * convu16tofloat;
				} break;
            default:
                DevCon.Error("Unknown flash type %d", flash->type);
				break;
		}
	}

	dump.Log(Color_Black, "commands count:%d flashes count: %d", offsets::uFrameCounter, renderedCommands, renderedFlashes);
}
