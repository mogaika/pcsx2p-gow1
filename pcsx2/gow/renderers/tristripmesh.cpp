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
	uniforms.matrices = r.shader_mesh.GetUniformLocation("uMatrices");
	uniforms.useTexture = r.shader_mesh.GetUniformLocation("uUseTexture");
	uniforms.blendColor = r.shader_mesh.GetUniformLocation("uBlendColor");
	uniforms.projectionMatrix = r.shader_mesh.GetUniformLocation("uProjectionMatrix");
	uniforms.texYScale = r.shader_mesh.GetUniformLocation("uTexYScale");
	uniforms.texOffset = r.shader_mesh.GetUniformLocation("uTexOffset");
	uniforms.useJointIndexes = r.shader_mesh.GetUniformLocation("uUseJointIndexes");
}

void renderers::TriStripMesh::Render(Renderer &r, bool isDynamic, bool writeDepth, u32 groupIndexStart, u32 groupIndexEnd, u32 binsStructOffset) {
	auto glContext = r.Window().AttachContext();

	r.shader_mesh.Use();

	auto dump = r.LogDumpPush(isDynamic ? "RendererDynamic" : "RendererStatic");

	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (writeDepth) {
		glDepthMask(GL_TRUE);
	} else {
		glDepthMask(GL_FALSE);
	}
	glEnable(GL_DEPTH_TEST);
	glActiveTexture(GL_TEXTURE0);

	if (isDynamic) {
		glUniform1i(uniforms.useTexture, 1);
		glUniform1i(uniforms.useJointIndexes, 1);
	} else {
		glUniform1i(uniforms.useTexture, 1);
		glUniform1i(uniforms.useJointIndexes, 0);
		glUniform2f(uniforms.texOffset, 0.0f, 0.0f);
	}

	float aspectRatio = float(r.Window().GetWidth()) / float(r.Window().GetHeight());

	/*
	if (offsets::viewportsList) {
		auto camHead = offsets::viewportsList->head();

		for (auto iterCam = camHead; iterCam != (void*)((raw::stViewportsStackElement**) (&camHead)); iterCam = iterCam->next()) {
			dump.Log(Color_Black, "FOUND VIEWPORT: 0x%x", iterCam->viewport());
		}
	}
	*/
	auto perspectiveMatrix = glm::perspective(glm::pi<float>() * 0.235f, offsets::aspectRatio, 0.2f, 50000.f);
	glUniformMatrix4fv(uniforms.projectionMatrix, 1, GL_FALSE, glm::value_ptr(perspectiveMatrix));

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

			if (!isMaterialSetted) {
				auto matLayer = flash->matLayer();
                auto texture = managers.texture->GetTexture(matLayer->_textureOffset);

				if (!texture) {
					continue;
				}

				r.CheckErrors("before glBindTexture");
				dump.Log(Color_Black, "texture '%s' image index %d", texture->GetName(), texture->GetRaw()->gfxInstance()->currentImageIndex);

				glUniform4fv(uniforms.blendColor, 1, glm::value_ptr(matLayer->blendColor));

				if (matLayer->haveTexture()) {
					glBindTexture(GL_TEXTURE_2D, texture->GetAnimatedCurrentGL());
					glUniform1f(uniforms.texYScale, texture->GetYScale());
					glUniform2f(uniforms.texOffset, float(matLayer->offset.x), float(matLayer->offset.y));
				}

				if (matLayer->isBlendAdditive()) {
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
					if (writeDepth) {
						glDepthMask(GL_FALSE);
					}
				} else {
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					if (writeDepth) {
						glDepthMask(GL_TRUE);
					}
                }

				r.CheckErrors("setMaterial");
				isMaterialSetted = true;
			}

			glm::mat4x4 *matrices;
			if (isDynamic) {
				if (object->isUseInvertedMatrix()) {
					matrices = pmemz<glm::mat4x4>(rmem<u32>(flash->_someBufferInStackForFlashesWithMatrices + 0xC));
				} else {
					matrices = pmemz<glm::mat4x4>(rmem<u32>(flash->_someBufferInStackForFlashesWithMatrices + 8));
				}

				for (u32 i = 0; i < u32(object->jointsMapCountPerInstance); i++) {
					glUniformMatrix4fv(uniforms.matrices + i, 1, GL_FALSE, glm::value_ptr(matrices[jointMap[i]]));
					r.CheckErrors("glUniformMatrix4fv");
				}
			} else {
				matrices = pmemz<glm::mat4x4>(rmem<u32>(flash->_someBufferInStackForFlashesWithMatrices + 8));
				glUniformMatrix4fv(uniforms.matrices, 1, GL_FALSE, glm::value_ptr(matrices[jointMap[0]]));
			}

			dump.Log(Color_Black, "flash 0x%03x: inst %d layer %d matlayer 0x%08x instance id %d %s",
				renderedFlashes, flash->instanceId, flash->layerIndex,
				flash->_usedMatLayer, flash->instanceId,
				flash->meshObject()->meshObject()->getMesh().getName());

			auto programsArray = meshObject->GetPrograms();
            if (!programsArray.size()) {
				DevCon.Error("gow: render: dynamic: empty programs array");
				continue;
			}
			auto programs = programsArray[flash->instanceId * flash->layerIndex];

			dump.Log(Color_Black,"dma programs: 0x%x programs: 0x%x", meshObject->GetPrograms().size(), programs.size());
			
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
