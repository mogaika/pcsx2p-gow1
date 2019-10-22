#include "PrecompiledHeader.h"

#include "gow/resources/mesh.h"
#include "gow/gl.h"
#include "gow/gow.h"

using namespace gow;

Mesh::Mesh(u32 offset, u32 serverOffset):
	serverOffset(serverOffset) {
	mesh = pmem<stMesh>(offset);
	mesh->refMesh() = this;

	DevCon.WriteLn("mesh 0x%x", mesh);
	for (u32 i = 0; i < mesh->partsCount; i++) {
		auto part = mesh->getPart(i);
		DevCon.WriteLn("part 0x%x", part);
		for (u32 j = 0; j < part->groupsCount; j++) {
			auto group = part->getGroup(j);
            DevCon.WriteLn("group 0x%x", group);
			for (u32 k = 0; k < group->objectsCount; k++) {
				auto object = group->getObject(k);
                DevCon.WriteLn("object 0x%x", object);
				object->refMeshObject() = new MeshObject(object);
			}
		}
	}
}

Mesh::~Mesh() {
    for (u32 i = 0; i < mesh->partsCount; i++) {
        auto part = mesh->getPart(i);
        for (u32 j = 0; j < part->groupsCount; j++) {
            auto group = part->getGroup(j);
            for (u32 k = 0; k < group->objectsCount; k++) {
                auto object = group->getObject(k);
                delete object->refMeshObject();
            }
        }
    }
}

MeshObject::MeshObject(stMeshObject *object):
	object(object) {
    core->Window()->AttachContext();
	decompilePackets();
    core->Window()->DetachContext();
}

MeshObject::~MeshObject() {
    core->Window()->AttachContext();
    for (auto i = arrays.begin(); i != arrays.end(); ++i) {
		auto program = *i;
		for (auto j = program.begin(); j != program.end(); ++j) {
            glDeleteVertexArrays(1, &j->vao);
		}
    }

    for (auto i = buffers.begin(); i != buffers.end(); ++i) {
        glDeleteBuffers(1, &i->second);
    }
    core->Window()->DetachContext();
}

MeshObject::stProgram MeshObject::stDecomileState::getProgram() {
    // attibutes indexes:
    //  0 - pos
    //  1 - uv
    //  2 - color
    //  3 - normal
    //  4 - joint1
    //  5 - joint2

	GLuint vao;
	glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    core->Renderer()->CheckErrors("glBindVertexArray");
	if (!xyzw) {
		DevCon.Error("Empty xyzw buffer");	
	}

    glBindBuffer(GL_ARRAY_BUFFER, xyzw);
    core->Renderer()->CheckErrors("glBindBuffer");
    glEnableVertexAttribArray(0);
    core->Renderer()->CheckErrors("glEnableVertexAttribArray");
    glVertexAttribPointer(0, 3, GL_SHORT, GL_FALSE, 8, NULL);
    core->Renderer()->CheckErrors("glVertexAttribPointer");
	
	if (uv) {
        glBindBuffer(GL_ARRAY_BUFFER, uv);
		glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, uvWidth == 16 ? GL_SHORT : GL_INT, GL_FALSE, 0, NULL);
    }

	if (rgba) {
        glBindBuffer(GL_ARRAY_BUFFER, rgba);
        glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, 0, NULL);
        glEnableVertexAttribArray(2);
	}
	/*
    if (normals) {
        glBindBuffer(GL_ARRAY_BUFFER, normals);
        glVertexAttribPointer(4, 3, GL_BYTE, GL_TRUE, 0, NULL);
        glEnableVertexAttribArray(3);
	}
	*/

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element);
    core->Renderer()->CheckErrors("bind array element buffer");

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	core->Renderer()->CheckErrors("vif call to vbo converted");
    
	stProgram prog;
	prog.ebo = element;
	prog.vao = vao;
    prog.indexesCount = indexesCount;
	return prog;
}

u32 MeshObject::decompileVifUnpack(stVifTag *vif, stDecomileState &state) {
	u8 cmd = vif->cmd();

    u32 componentsCount = ((u32(cmd) >> 2) & 0x3) + 1; // elements per vertex
    u32 width = u32(1) << (5 - (u32(cmd) & 0x3)); // single element width in bits
	u32 dataSize = (componentsCount * width * u32(vif->num())) / 8;
	u32 target = vif->imm() & 0x3ff;

	u8 *data = (u8*)(u32(vif) + 4);
	u8 *dataEnd = data + dataSize;

	/*
	DevCon.WriteLn("unpack vif 0x%x", vif);
	for (auto p = data;p < dataEnd;) {
        wxString line;
		for (u32 i = 0; i < 16; i++) {
			line += wxString::Format("%2x ", *(p++));
			if (p == dataEnd) {
				break;
			}
		}
        DevCon.WriteLn(line);
	}
	*/

    if (width == 32 && componentsCount == 4) {
        if (target == 0x000 || target == 0x155 || target == 0x2ab) { // vmta
            // DevCon.WriteLn("              vmta");
			/*
			u32 metasCount = dataSize / sizeof(metasCount);
			auto metaEnd = metaStart + metasCount;

            for (auto meta = metaStart; meta != metaEnd; meta++) {}
			*/

        } else { // bndr
            // DevCon.WriteLn("              bndr");
        }
	} else {
        GLuint buffer;
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, dataSize, data, GL_STATIC_DRAW);

		buffers[vif] = buffer;

		if (componentsCount == 4) {
			if (width == 16) { // position int16[4]
                state.xyzw = buffer;
				 DevCon.WriteLn("              pos");
                // since vertex amount stored in byte we can't have more then 256 vertexes and 256*3-6 indexes respectively
				static u16 indexBuffer[4096]; 
                u16 *pIndex = &indexBuffer[0];

				u16 vertexCount = dataSize / (sizeof(u16) * 4);
                u16 indexesCount = 0;
                for (u16 iVertex = 0; iVertex < vertexCount; iVertex++) {
					if ((data[iVertex*8 + 7] & 0x80) == 0) {
                        indexesCount += 3;
						*(pIndex++) = iVertex;
                        *(pIndex++) = iVertex - 1;
                        *(pIndex++) = iVertex - 2;
					}
				}
                /*
                for (auto p = indexBuffer; p < pIndex;) {
                    wxString line;
                    for (u32 i = 0; i < 16; i++) {
                        line += wxString::Format("%4d ", *(p++));
                        if (p == pIndex) {
                            break;
                        }
                    }
                    DevCon.WriteLn(line);
                }
                DevCon.WriteLn("              idx");
				*/

				GLuint elementBuffer;
				glGenBuffers(1, &elementBuffer);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementBuffer);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexesCount * sizeof(u16), indexBuffer, GL_STATIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

				buffers[vif + 1] = elementBuffer;
				state.element = elementBuffer;
                state.indexesCount = indexesCount;
			} else if (width == 8) { // color uint8[4]
                state.rgba = buffer;
				// DevCon.WriteLn("              color");
			}
		} else if (componentsCount == 3) {
			if (width == 8) { // normals int8[3]
                state.normals = buffer;
				// DevCon.WriteLn("              norm");
			}
		} else {  // uv int16 | int32
            // DevCon.WriteLn("              uv %d", width);
            state.uv = buffer; 
			state.uvWidth = width;
		}
    }
	core->Renderer()->CheckErrors("vif unpack parsed");
	return dataSize;
}

void MeshObject::decompileVifProgram(stVifTag *vifStart, stVifTag *vifEnd, dma_program_t &program) {
    stDecomileState state;
	state.reset();

	for (stVifTag *vif = vifStart; vif != vifEnd; vif++) {
		auto cmd = vif->cmd();
        // DevCon.WriteLn("vif 0x%x cmd 0x%x", vif, cmd);
		if (cmd > 0x60) {
			// vif unpack
			// create vbo/ebo
            vif = pointer_add(vif, align_ceil(decompileVifUnpack(vif, state), 4));
		} else if (cmd == stVifTag::CMD_MSCAL) {
			// call rendering program
			// create vao
			DevCon.WriteLn("created program for vif 0x%x", vif);
			program.push_back(state.getProgram());
			state.reset();
		} else if (cmd == stVifTag::CMD_STROW) {
            vif = pointer_add(vif, 0x10);
        } else if (cmd == stVifTag::CMD_NOP || cmd == stVifTag::CMD_STCYCL || cmd == stVifTag::CMD_STMOD) {
            // ignore
		} else {
			DevCon.Error("gow: mesh: Unknown vif tag cmd 0x%x (0x%x) (0x%x)", vif->cmd(), vif->_tag, vif);
			break;
		}
	}
	if (state.xyzw) {
        program.push_back(state.getProgram());
    }
}

void  MeshObject::decompileDmaProgram(stDmaTag *dmaProgram, dma_program_t &program) {
	for (stDmaTag *tag = dmaProgram;; tag = pointer_add(tag, 0x10)) {
		switch (tag->id()) {
            case stDmaTag::ID_RET:
				return;
            case stDmaTag::ID_REF: {
					stVifTag *vifProgramBegin = pmem<stVifTag>(tag->addr());
					stVifTag *vifProgramEnd = pointer_add(vifProgramBegin, tag->qwc() * 0x10);

					DevCon.Error("gow: mesh: dma tag 0x%x (0x%x) (0x%x) vif prog 0x%x:0x%x",
								 tag->id(), tag->_tag, tag, vifProgramBegin, vifProgramEnd);

					decompileVifProgram(vifProgramBegin, vifProgramEnd, program);
					static int totalvao = 0;
					totalvao += program.size();
                    DevCon.Error("created vao count %d (total: %d)", program.size(), totalvao);
					for (auto i = program.begin(); i != program.end(); i++) {
						DevCon.WriteLn("vao 0x%x", i->vao);
					}
				}
				break;
            default:
				DevCon.Error("gow: mesh: Unknown dma tag 0x%x (0x%x) (0x%x)", tag->id(), tag->_tag, tag);
				return;
		}
    }
}

void MeshObject::decompilePackets() {
    DevCon.WriteLn("decompiling object 0x%x", object);
	for (u32 iLayer = 0; iLayer < object->textureLayersCount; iLayer++) {
		for (u32 iInstance = 0; iInstance < object->instancesCount; iInstance++) {
            DevCon.WriteLn("         layer 0x%x instance 0x%x", iLayer, iInstance);

			auto dmaTag = pointer_add(object->dmaTags, iLayer * iInstance * 0x10);

			DevCon.WriteLn("         dma prog offset 0x%x", dmaTag);
			dma_program_t program;
            decompileDmaProgram(dmaTag, program);
			arrays.push_back(program);
		}
	}
}

Mesh *MeshManager::GetMesh(u32 offset) {
    auto mesh = meshes.find(offset);
    return (mesh == meshes.end()) ? nullptr : mesh->second;
}

void MeshManager::HookInstanceCtor(u32 serverOffset) {
    auto offset = cpuRegs.GPR.n.s0.UL[0];
    auto name = pmemz<char>(cpuRegs.GPR.n.s3);
    if (strcmp(name, "Shell_0") && strcmp(name, "HUD_0")) {
        return;
    }
    auto mesh = new Mesh(offset, serverOffset);
    if (!meshes.insert(std::pair<u32, Mesh *>(offset, mesh)).second) {
		DevCon.Error("Wasn't able to insert mesh: key %x already exists", offset);
    } else {
		DevCon.WriteLn("gow: mesh: inserter new mesh %x %s", offset, name);
    };
}

void MeshManager::HookServerDtor() {
    auto serverOffset = cpuRegs.GPR.n.a0.UL[0];
	DevCon.WriteLn("gow: mesh: Server %x destroying", serverOffset);

	for (auto i = meshes.begin(); i != meshes.end();) {
		if (i->second->GetServerOffset() == serverOffset) {
            DevCon.Error("gow: mesh: removing: %x", i->first);
			delete i->second;
			i = meshes.erase(i);
		} else {
			++i;
		}
	}
}

