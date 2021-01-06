#include "PrecompiledHeader.h"

#include "gow/gow.h"
#include "gow/renderers/tristripmesh.h"
#include "gow/resources/renderer.h"

#include "../3rdparty/glm/mat4x4.hpp"
#include "../3rdparty/glm/ext/matrix_clip_space.hpp"
#include "../3rdparty/glm/ext/scalar_constants.hpp"
#include "../3rdparty/glm/gtc/type_ptr.hpp"

using namespace gow;

void renderers::TriStripMesh::Setup(Renderer &r) {
	Shader &s = r.shader_mesh;

	uniforms.matrices = s.GetUniformLocation("uMatrices");
	uniforms.useTexture = s.GetUniformLocation("uUseTexture");
	uniforms.blendColor = s.GetUniformLocation("uBlendColor");
	uniforms.projectionMatrix = s.GetUniformLocation("uProjectionMatrix");
	uniforms.texYScale = s.GetUniformLocation("uTexYScale");
	uniforms.texOffset = s.GetUniformLocation("uTexOffset");
	uniforms.useJointIndexes = s.GetUniformLocation("uUseJointIndexes");
	uniforms.transparency = s.GetUniformLocation("uTransparency");
}

void renderers::TriStripMesh::Render(Renderer &r, bool isDynamic, bool maskMirrors, u32 groupIndexStart, u32 groupIndexEnd, u32 binsStructOffset) {
	auto glContext = r.Window().AttachContext();

	r.shader_mesh.Use();

	auto dump = r.LogDumpPush(isDynamic ? "RendererDynamic" : "RendererStatic");

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glActiveTexture(GL_TEXTURE0);

	if (maskMirrors) {
		glUniform1i(uniforms.useTexture, 0);
	} else {
		glUniform1i(uniforms.useTexture, 1);
	}

	if (isDynamic) {
		glUniform1i(uniforms.useJointIndexes, 1);
	} else {
		glUniform1i(uniforms.useJointIndexes, 0);

		glUniform2f(uniforms.texOffset, 0.0f, 0.0f);
		glUniform1f(uniforms.transparency, 1.0f);
	}

	float aspectRatio = float(r.Window().GetWidth()) / float(r.Window().GetHeight());

	/*
	if (offsets::viewportsList) {
		auto camHead = offsets::viewportsList->head();
		auto end = offsets::viewportsList->end();

		for (auto iterCam = camHead; iterCam != end; iterCam = iterCam->next()) {
			auto viewport = iterCam->viewport();
			dump.Log(Color_Black, "FOUND VIEWPORT: 0x%x", viewport);
			for (int i = 0; i < 13; i++) {
				//dump.Log(Color_Blue, " matrix %d", i);
				//dump.LogMatrix(Color_Black, viewport->matrices[i]);
			}
		}
	}
	*/

	// auto viewport1 = offsets::viewportsList->head()->viewport();
	auto viewport2 = offsets::viewportsList->head()->next()->viewport();
	
	//auto perspectiveMatrix = glm::perspective(glm::pi<float>() * 0.235f, offsets::aspectRatio, 0.2f, 50000.f);
	//glUniformMatrix4fv(uniforms.projectionMatrix, 1, GL_FALSE, glm::value_ptr(perspectiveMatrix));
	glUniformMatrix4fv(uniforms.projectionMatrix, 1, GL_FALSE, glm::value_ptr(viewport2->matrices[4]));

	//dump.Log(Color_Red, " Perspective matrix");
	//dump.LogMatrix(Color_Black, perspectiveMatrix);

	int renderedFlashes = 0;
	int renderedGroups = 0;
	auto texGroupArray = pmemz<u32>(rmem<u32>(binsStructOffset + 0x4));

	for (u32 iTexGroup = groupIndexStart; iTexGroup < groupIndexEnd; iTexGroup++) {
        auto firstFlashOffset = texGroupArray[iTexGroup];
        if (!firstFlashOffset) {
			continue;
        }
		renderedGroups++;

		dump.Log(Color_Green, "texture group %d", renderedGroups);

		bool isMaterialSetted = false;

        auto firstFlash = pmemz<raw::stRenderFlashTriStrip>(firstFlashOffset);
        for (auto flash = firstFlash; flash != 0; flash = (raw::stRenderFlashTriStrip *)flash->next()) {
            renderedFlashes++;

			auto object = flash->meshObject();
			auto meshObject = object->meshObject();
			u32 *jointMap = object->jointMap(flash->instanceId);

			if (!meshObject) {
				// TODO: examine issue. Dynamic water in static server?
				continue;
			}

			if (!isMaterialSetted && !maskMirrors) {
				auto matLayer = flash->matLayer();
                auto texture = managers.texture->GetTexture(matLayer->_textureOffset);

				if (!texture) {
					continue;
				}

				r.CheckErrors("before glBindTexture");
				if (r.IsDumpingFrame()) {
					dump.Log(Color_Black, "texture '%s' image index %d", wxString(texture->GetName()).c_str().AsInternal(), texture->GetRaw()->gfxInstance()->currentImageIndex);
				}

				glUniform4fv(uniforms.blendColor, 1, glm::value_ptr(matLayer->blendColor));

				if (matLayer->haveTexture()) {
					glBindTexture(GL_TEXTURE_2D, texture->GetAnimatedCurrentGL());
					glUniform1f(uniforms.texYScale, texture->GetYScale());
					glUniform2f(uniforms.texOffset, float(matLayer->offset.x), float(matLayer->offset.y));
				}

				if (matLayer->isBlendAdditive()) {
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
					glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
					glDepthMask(GL_FALSE);
				} else if (matLayer->isBlendSubstract()) {
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
					glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT);
					glDepthMask(GL_FALSE);
				} else {
					if (matLayer->isDisableDepthWrite()) {
						glDepthMask(GL_FALSE);
					} else {
						glDepthMask(GL_TRUE);
					}

                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                }

				dump.Log(Color_StrongCyan, "matlayer disable depth write %d ba %d bn %d br %d bs %d",
					matLayer->isDisableDepthWrite(),
					matLayer->isBlendAdditive(), matLayer->isBlendNone(),
					matLayer->isBlendReflection(), matLayer->isBlendSubstract());

				r.CheckErrors("setMaterial");
				isMaterialSetted = true;
			}

			if (isDynamic) {
				glm::mat4x4 *matrices;
				if (object->isUseInvertedMatrix()) {
					matrices = pmemz<glm::mat4x4>(rmem<u32>(flash->_someBufferInStackForFlashesWithMatrices + 0xC));
				} else {
					matrices = pmemz<glm::mat4x4>(rmem<u32>(flash->_someBufferInStackForFlashesWithMatrices + 8));
					if (object->type == 0x1d) {
						matrices = &matrices[1];
					}
				}

				for (u32 i = 0; i < u32(object->jointsMapCountPerInstance); i++) {
					glUniformMatrix4fv(uniforms.matrices + i, 1, GL_FALSE, glm::value_ptr(matrices[jointMap[i]]));
					r.CheckErrors("glUniformMatrix4fv");
				}
			} else {
				auto matrices = pmemz<glm::mat4x4>(rmem<u32>(flash->_someBufferInStackForFlashesWithMatrices + 8));
				glUniformMatrix4fv(uniforms.matrices, 1, GL_FALSE, glm::value_ptr(matrices[jointMap[0]]));
			}

			glUniform1f(uniforms.transparency, flash->transparency);

			auto programsArray = meshObject->GetPrograms();
            if (!programsArray.size()) {
				DevCon.Error("gow: render: dynamic: empty programs array");
				continue;
			}
			auto programId = flash->instanceId + flash->layerIndex;
			auto programs = programsArray[programId];

			if (r.IsDumpingFrame()) {
				dump.Log(Color_StrongBlack, "flash 0x%03x: inst %d layer %d transp %f dma: 0x%x progI: %d progS: 0x%x %s",
					renderedFlashes, flash->instanceId, flash->layerIndex, flash->transparency,
					meshObject->GetPrograms().size(), programId, programs.size(),
					wxString(flash->meshObject()->meshObject()->getMesh()->getName()).c_str().AsInternal());
			}
			
            for (auto vao = programs.begin(); vao != programs.end(); ++vao) {
                r.CheckErrors("before bind");
                glBindVertexArray(vao->vao);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao->ebo);
                r.CheckErrors("before draw elements");
                glDrawElements(GL_TRIANGLES, vao->indexesCount, GL_UNSIGNED_SHORT, 0);
                r.CheckErrors("draw elements");
            }
		}
    }

	dump.Log(Color_Black, "[frame 0x%07x] Static groups: %d flashes: %d", offsets::uFrameCounter, renderedGroups, renderedFlashes);
}
