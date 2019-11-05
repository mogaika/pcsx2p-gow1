#include "PrecompiledHeader.h"

#include "gow/resources/mesh.h"
#include "gow/gl.h"
#include "gow/gow.h"
#include "gow/dynmem.h"

using namespace gow;

const float raw::stMeshGroup::LOD_MAX = -34359738368.0f;

Mesh::Mesh(u32 offset, u32 allocatorOffset, char *name):
	allocatorOffset(allocatorOffset) {
    mesh = pmem<raw::stMesh>(offset);

	strncpy(this->name, name, sizeof(this->name));

#ifdef GOW_MESH_DEBUG
	DevCon.WriteLn("mesh 0x%x", mesh);
#endif
	for (u32 i = 0; i < mesh->partsCount; i++) {
		auto part = mesh->getPart(i);
#ifdef GOW_MESH_DEBUG
		DevCon.WriteLn("part 0x%x", part);
#endif
		for (u32 j = 0; j < part->groupsCount; j++) {
			auto group = part->getGroup(j);
#ifdef GOW_MESH_DEBUG
            DevCon.WriteLn("group 0x%x", group);
#endif
			for (u32 k = 0; k < group->objectsCount; k++) {
				auto object = group->getObject(k);
#ifdef GOW_MESH_DEBUG
                DevCon.WriteLn("object 0x%x", object);
#endif
				object->setMeshObject(new MeshObject(this, object));
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
                delete object->meshObject();
            }
        }
    }
}

MeshObject::MeshObject(Mesh *mesh, raw::stMeshObject *object):
	object(object),
	mesh(mesh) {
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
    //  4 - joints

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
	
    if (normals) {
        glBindBuffer(GL_ARRAY_BUFFER, normals);
        glVertexAttribPointer(3, 3, GL_BYTE, GL_TRUE, 0, NULL);
        glEnableVertexAttribArray(3);
	}

	if (jointIndexes) {
        glBindBuffer(GL_ARRAY_BUFFER, jointIndexes);
        glVertexAttribPointer(4, 2, GL_BYTE, GL_FALSE, 0, NULL);
        glEnableVertexAttribArray(4);
    }

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

u32 MeshObject::decompileVifUnpack(raw::stVifTag *vif, stDecomileState &state) {
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
	bool handled = false;

	// TODO: cache buffer using buffers[vif] map

    if (width == 32 && componentsCount == 4) {
        if (target == 0x000 || target == 0x155 || target == 0x2ab) { // vmta
            handled = true;
            // DevCon.WriteLn("              vmta");
			
			// since vertex amount stored in byte we can't have more then 256 vertexes and 256*2 jointsIndexes
            static u8 jointIndexesBuffer[512];
			int jointBufferIndex = 0;

			u32 metasCount = dataSize / sizeof(raw::stMeshPacketMeta);
            for (auto meta = (raw::stMeshPacketMeta *)data; ; meta++) {
                for (u8 i = 0; i < meta->indexesCount; i++) {
                    jointIndexesBuffer[jointBufferIndex++] = meta->jointIndexes[0] >> 2;
                    jointIndexesBuffer[jointBufferIndex++] = meta->jointIndexes[1] >> 4;

#ifndef NDEBUG
                    if (jointBufferIndex >= sizeof(jointIndexesBuffer)/sizeof(jointIndexesBuffer[0])) {
                        DevCon.Error("JOINT INDEX BUFFER OVERFLOW: 0x%x", jointBufferIndex);
                    }
#endif
				}
				if (meta->isLast()) {
					break;
				}
			}
			
			GLuint buffer;
            glGenBuffers(1, &buffer);
            glBindBuffer(GL_ARRAY_BUFFER, buffer);
            glBufferData(GL_ARRAY_BUFFER, jointBufferIndex * sizeof(u8), jointIndexesBuffer, GL_STATIC_DRAW);
			state.jointIndexes = buffer;

			buffers[vif] = buffer;
        } else { // bndr
            handled = true;
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
                handled = true;
                state.xyzw = buffer;
				 //DevCon.WriteLn("              pos");
                // since vertex amount stored in byte we can't have more then 256 vertexes and 256*3 indexes respectively
				static u16 indexBuffer[1024]; 
                u16 *pIndex = &indexBuffer[0];

				u16 vertexCount = dataSize / (sizeof(u16) * 4);
                u16 indexesCount = 0;
                for (u16 iVertex = 0; iVertex < vertexCount; iVertex++) {
					if ((data[iVertex*8 + 7] & 0x80) == 0) {
                        indexesCount += 3;
						*(pIndex++) = iVertex;
                        *(pIndex++) = iVertex - 1;
                        *(pIndex++) = iVertex - 2;
#ifndef NDEBUG
                        if (uintptr_t(pIndex) >= uintptr_t(indexBuffer) + sizeof(indexBuffer)) {
                            DevCon.Error("INDEX BUFFER OVERFLOW: 0x%x", (u32(pIndex) - u32(indexBuffer))/sizeof(indexBuffer[0]));
						}
#endif
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
                handled = true;
                state.rgba = buffer;
				// DevCon.WriteLn("              color");
			}
		} else if (componentsCount == 3) {
			if (width == 8) { // normals int8[3]
                handled = true;
                state.normals = buffer;
				// DevCon.WriteLn("              norm");
			}
		} else {  // uv int16 | int32
            handled = true;
            // DevCon.WriteLn("              uv %d", width);
            state.uv = buffer; 
			state.uvWidth = width;
		}
    }

	if (!handled) {
		DevCon.Error("UNHANDLED VIF PACKET %x", vif);
	}

	core->Renderer()->CheckErrors("vif unpack parsed");
	return dataSize;
}

void MeshObject::decompileVifProgram(raw::stVifTag *vifStart, raw::stVifTag *vifEnd, dma_program_t &program) {
    stDecomileState state;
	state.reset();

	for (raw::stVifTag *vif = vifStart; vif != vifEnd; vif++) {
		auto cmd = vif->cmd();
        // DevCon.WriteLn("vif 0x%x cmd 0x%x", vif, cmd);
		if (cmd > 0x60) {
			// vif unpack
			// create vbo/ebo
            vif = pointer_add(vif, align_ceil(decompileVifUnpack(vif, state), 4));
        } else if (cmd == raw::stVifTag::CMD_MSCAL) {
			// call rendering program
			// create vao
			//DevCon.WriteLn("created program for vif 0x%x", vif);
			program.push_back(state.getProgram());
			state.reset();
        } else if (cmd == raw::stVifTag::CMD_STROW) {
            vif = pointer_add(vif, 0x10);
        } else if (cmd == raw::stVifTag::CMD_NOP || cmd == raw::stVifTag::CMD_STCYCL || cmd == raw::stVifTag::CMD_STMOD) {
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

void  MeshObject::decompileDmaProgram(raw::stDmaTag *dmaProgram, dma_program_t &program) {
    for (raw::stDmaTag *tag = dmaProgram;; tag = pointer_add(tag, 0x10)) {
		switch (tag->id()) {
            case raw::stDmaTag::ID_RET:
				return;
            case raw::stDmaTag::ID_REF: {
					raw::stVifTag *vifProgramBegin = pmem<raw::stVifTag>(tag->addr());
					raw::stVifTag *vifProgramEnd = pointer_add(vifProgramBegin, tag->qwc() * 0x10);

					//DevCon.Error("gow: mesh: dma tag 0x%x (0x%x) (0x%x) vif prog 0x%x:0x%x",
					//			 tag->id(), tag->_tag, tag, vifProgramBegin, vifProgramEnd);

					decompileVifProgram(vifProgramBegin, vifProgramEnd, program);
					static int totalvao = 0;
					totalvao += program.size();
#ifdef GOW_MESH_DEBUG
                    DevCon.Error("created vao count %d (total: %d)", program.size(), totalvao);
					//for (auto i = program.begin(); i != program.end(); i++) {
					//	DevCon.WriteLn("vao 0x%x", i->vao);
					//}
#endif
				}
				break;
            default:
				DevCon.Error("gow: mesh: Unknown dma tag 0x%x (0x%x) (0x%x)", tag->id(), tag->_tag, tag);
				return;
		}
    }
}

void MeshObject::decompilePackets() {
    // DevCon.WriteLn("decompiling object 0x%x", object);
	for (u32 iLayer = 0; iLayer < object->textureLayersCount; iLayer++) {
		for (u32 iInstance = 0; iInstance < object->instancesCount; iInstance++) {
           // DevCon.WriteLn("         layer 0x%x instance 0x%x", iLayer, iInstance);

			auto dmaTag = pointer_add(object->dmaTags, iLayer * iInstance * 0x10);

			//DevCon.WriteLn("         dma prog offset 0x%x", dmaTag);
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

void MeshManager::HookInstanceCtor() {
    auto offset = cpuRegs.GPR.n.s0.UL[0];
    auto name = pmemz<char>(cpuRegs.GPR.n.s3);
    /*
	if (strcmp(name, "Shell_0") && strcmp(name, "HUD_0")) {
        return;
    }
	*/

	auto allocatorOffset = offsets::allocatorsStack.headOffset();

    auto mesh = new Mesh(offset, allocatorOffset, name);
    if (!meshes.insert(std::pair<u32, Mesh *>(offset, mesh)).second) {
		DevCon.Error("Wasn't able to insert mesh: key %x already exists", offset);
    } else {
#ifdef GOW_MESH_DEBUG
		DevCon.WriteLn("gow: mesh: inserter new mesh %x %s", offset, name);
#endif
    };
}

void MeshManager::HookAllocatorDtor(u32 allocatorOffset) {
	int removedMeshes = 0;

	for (auto i = meshes.begin(); i != meshes.end();) {
        if (i->second->GetAllocatorOffset() == allocatorOffset) {
#ifdef GOW_MESH_DEBUG
            DevCon.Error("gow: mesh: removing: %x", i->first);
#endif
			delete i->second;
            removedMeshes++;
			i = meshes.erase(i);
		} else {
			++i;
		}
	}
    DevCon.WriteLn(Color_Magenta, "gow: mesh: Allocator 0x%.7x (%s) destroying. Removed %d meshes. Left %d meshes.",
		allocatorOffset, pmem<dynmem::stStackAllocator>(allocatorOffset)->name, removedMeshes, meshes.size());
}

