#include "PrecompiledHeader.h"

#include "gow/renderer.h"
#include "gow/gl.h"
#include "gow/gow.h"

using namespace gow;

void gow::Renderer::loadShaders() {
    shader_textured_quad.Load("../../gow/shaders/textured_quad.vert", "../../gow/shaders/textured_quad.frag");
	shader_flash.Load("../../gow/shaders/flash.vert", "../../gow/shaders/flash.frag");
    shader_mesh.Load("../../gow/shaders/mesh.vert", "../../gow/shaders/mesh.frag");
}

Renderer::Renderer(Window *window):
	window(window), 
	reloadShadersRequest(0),
	currentPreviewTexture(0),
	dumpFrame(false) {
	size1 = 1.0;
    // size2 = 0.001953125;
    size2 = 1.0;
	Setup();
}

void Renderer::CheckErrors(char *phase) {
    auto error = glGetError();
    if (error != GL_NO_ERROR) {
        DevCon.Error("gow: OPENGL phase '%s' ERROR 0x%x %d", phase, error, error);
    }
}

void initializeQuad();
void Renderer::Setup() {
	if (window->wglSwapIntervalEXT) {
		// window->wglSwapIntervalEXT(-1);
	}
	window->AttachContext();

	glClearColor(0.0, 0.0, 1.0, 1.0);
    glClearDepth(10000.0);

	CheckErrors("color");

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	CheckErrors("setup");

	loadShaders();
	
	initializeQuad();
    CheckErrors("initializeQuad");

	window->DetachContext();
}

static GLuint quad_vbo, quad_vao, quad_ebo;
void initializeQuad() {
    static float vertices[] = {
        // positions        texture coords
        0.5f, 0.5f, 0.0f, 1.0f, 1.0f,   // top right
        0.5f, -0.5f, 0.0f, 1.0f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, // bottom left
        -0.5f, 0.5f, 0.0f, 0.0f, 1.0f   // top left
    };
    static unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle
    };

    glGenVertexArrays(1, &quad_vao);
    glGenBuffers(1, &quad_vbo);
    glGenBuffers(1, &quad_ebo);


    glBindVertexArray(quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void Renderer::renderTexturedQuad(GLuint texture) {    
	shader_textured_quad.Use();
    CheckErrors("use shader quad");

	glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

	glBindVertexArray(quad_vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_ebo);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    CheckErrors("draw elements");
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
	CheckErrors("unuse program");
}

void Renderer::RenderFlashes() {
	window->AttachContext();
	
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(false);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);
	
	shader_flash.Use();
	auto matrixUniform = shader_flash.GetUniformLocation("uMatrix");
    auto size1uniform = shader_flash.GetUniformLocation("size1");
    auto size2uniform = shader_flash.GetUniformLocation("size2");
    auto useTextureuniform = shader_flash.GetUniformLocation("uUseTexture");
    auto blendColoruniform = shader_flash.GetUniformLocation("uBlendColor");

	float blendColors[4] = {1.0, 1.0, 1.0, 1.0};

	if (dumpFrame) {
		DevCon.Error("[frame 0x%.7x Flashes start", offsets::uFrameCounter);
	}

	glUniform4f(blendColoruniform, 1.0, 1.0, 1.0, 1.0);

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

				CheckErrors("gow: render: preuniform");
				glUniform1f(size1uniform, size1);
                glUniform1f(size2uniform, size2);
                CheckErrors("gow: render: postuniform");
                glUniformMatrix4fv(matrixUniform, 1, GL_FALSE, matrix);
				
				if (dumpFrame) {
                    DevCon.WriteLn("draw mesh 0x%x (%d)", flpData2->meshPartIndex, flpData2->meshPartIndex);
                }

				for (u32 iObject = 0; iObject < group->objectsCount; iObject++) {
                    auto flpDataSub2 = flpData2->getData2Sub1(iObject);
					auto object = group->getObject(iObject);
					
					bool hasTexture = flpDataSub2->txrInstanceOffset != 0;

					if (hasTexture) {
						auto texture = managers.texture->GetTexture(flpDataSub2->txrInstanceOffset);
						if (!texture) {
                            DevCon.Error("gow: render: flp: wasn't able to find texture 0x%x", flpDataSub2->txrInstanceOffset);
                        }
						glUniform1i(useTextureuniform, 1);
                        glBindTexture(GL_TEXTURE_2D, texture->GetGl(0));

						glUniform4f(blendColoruniform, blendColors[0], blendColors[1], blendColors[2], blendColors[3]);
                        if (dumpFrame) {
                            DevCon.WriteLn("draw textured %f %f %f %f",
								blendColors[0], blendColors[1], blendColors[2], blendColors[3]);
                        }
                    } else {
                        glBindTexture(GL_TEXTURE_2D, 0);
                        glUniform1i(useTextureuniform, 0);

						const float u8tofloat = (1.0f / 255.0f);
                        float fBlendColors[4];

						// flpDataSub2->blendColor has inverted order 
						fBlendColors[0] = blendColors[0] * float(flpDataSub2->blendColor[2]) * u8tofloat;
                        fBlendColors[1] = blendColors[1] * float(flpDataSub2->blendColor[1]) * u8tofloat;
                        fBlendColors[2] = blendColors[2] * float(flpDataSub2->blendColor[0]) * u8tofloat;
                        fBlendColors[3] = blendColors[3] * float(flpDataSub2->blendColor[3]) * u8tofloat;

                        glUniform4f(blendColoruniform, fBlendColors[0], fBlendColors[1], fBlendColors[2], fBlendColors[3]);

						if (dumpFrame) {
                            DevCon.WriteLn("draw untextured %f %f %f %f (0x%02x 0x%02x 0x%02x 0x%02x)",
								fBlendColors[0], fBlendColors[1], fBlendColors[2], fBlendColors[3],
								flpDataSub2->blendColor[0], flpDataSub2->blendColor[1], flpDataSub2->blendColor[2], flpDataSub2->blendColor[3]);
                        }
                    }

					auto meshObject = object->refMeshObject();

					//DevCon.WriteLn("gow: render: flp: dma programs: %x", meshObject->GetPrograms().size());
					auto programs = meshObject->GetPrograms()[0];
                    //DevCon.WriteLn("gow: render: flp: programs: %x", programs.size());

					for (auto vao = programs.begin(); vao != programs.end(); ++vao) {
                        CheckErrors("gow: render: flp: before bind");
                        // DevCon.WriteLn("gow: render: flp: bind vao 0x%x count 0x%x", vao->vao, vao->indexesCount);
                        glBindVertexArray(vao->vao);
						glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao->ebo);
                        CheckErrors("gow: render: flp: before draw elements");
                        glDrawElements(GL_TRIANGLES, vao->indexesCount, GL_UNSIGNED_SHORT, 0);
						// DevCon.WriteLn("gow: render: flp: rendered vao 0x%x", vao->first);
						CheckErrors("gow: render: flp: draw elements");
                    }
				}
				} break;
            case 1: {
                auto color = (raw::stRenderFlashUISetColor*) flash;
                const float convu16tofloat = (1.0f / 256.0f);
                if (dumpFrame) {
                    DevCon.WriteLn("set color 0x%03x 0x%03x 0x%03x 0x%03x", color->color[0], color->color[1], color->color[2], color->color[3]);
                }
                blendColors[0] = float(color->color[0]) * convu16tofloat;
                blendColors[1] = float(color->color[1]) * convu16tofloat;
                blendColors[2] = float(color->color[2]) * convu16tofloat;
                blendColors[3] = float(color->color[3]) * convu16tofloat;
				// ignore for now
				} break;
            default:
                DevCon.Error("Unknown flash type %d", flash->type);
				break;
		}
	}

    if (dumpFrame) {
        DevCon.Error("[frame 0x%.7x commands count:%d flashes count: %d", offsets::uFrameCounter, renderedCommands, renderedFlashes);
    }

	window->DetachContext();
}

void Renderer::RenderStatic() {
	window->AttachContext();

	glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDepthMask(true);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);

	if (dumpFrame) {
        DevCon.Error("[frame 0x%.7x Static start", offsets::uFrameCounter);
    }

	int renderedFlashes = 0;
	auto firstFlash = pmemz<raw::stRenderFlashStatic>(cpuRegs.GPR.n.sp.UL[0] + 0x50);

    for (auto flash = firstFlash; flash != 0; flash = (raw::stRenderFlashStatic *)flash->next()) {
        renderedFlashes++;
    }
	if (dumpFrame) {
		DevCon.Error("[frame 0x%.7x Static elements: %d", offsets::uFrameCounter, renderedFlashes);
    }

	window->DetachContext();
}

void Renderer::EndOfFrame() {
	window->AttachContext();

	if (reloadShadersRequest) {
		loadShaders();
		reloadShadersRequest = false;
	}

	// managers.texture->GetTexture
    CheckErrors("clear");
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    CheckErrors("swap buffers");

	if (currentPreviewTexture) {
        auto texture = managers.texture->GetTexture(currentPreviewTexture);
		if (texture) {
			renderTexturedQuad(texture->GetGl(0));
	    } else {
			DevCon.Error("gow:render:debug Wasn't able to find texture %x", currentPreviewTexture);
		}
    }

	dumpFrame = false;
    window->SwapBuffers();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	window->DetachContext();
}

GLuint Shader::compileShader(char *filename, GLuint shader) {
    wxFile file(filename);
    if (!file.IsOpened()) {
        DevCon.Error("gow: shader: can't open %d file %s", shader, filename);
        DevCon.Error(wxGetCwd());
		return shader;
    }
    
	wxString shaderString;
    file.ReadAll(&shaderString);
    file.Close();

	wxCStrData shaderCStr = shaderString.c_str();
	const char *shaderpChar = shaderCStr.AsChar();

    glShaderSource(shader, 1, &shaderpChar, NULL);
    glCompileShader(shader);

	GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
		char log[256];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        DevCon.Error("gow: shader: Error compiling %s", filename);
        DevCon.Error("%s", log);
    };

	return shader;
}

void Shader::Load(char *vertexFilename, char *fragmentFilename) {
    auto vert = glCreateShader(GL_VERTEX_SHADER);
    auto frag = glCreateShader(GL_FRAGMENT_SHADER);

	compileShader(vertexFilename, vert);
    compileShader(fragmentFilename, frag);

	GLuint oldProgram = program;
	program = glCreateProgram();
	glAttachShader(program, vert);
    glAttachShader(program, frag);
	glLinkProgram(program);
	
	GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[256];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        DevCon.Error("gow: shader: Error linking %s:%s", vertexFilename, fragmentFilename);
        DevCon.Error("%s", log);

		glDeleteProgram(program);
		if (loaded) {
            program = oldProgram;
        }
    } else {
		if (loaded) {
            glDeleteProgram(oldProgram);
        }
        loaded = true;
	}

	glDeleteShader(vert);
    glDeleteShader(frag);
}

void gow::Shader::Use() {
	glUseProgram(program);
}

GLuint Shader::GetUniformLocation(char *name) {
    return glGetUniformLocation(program, name);
}
