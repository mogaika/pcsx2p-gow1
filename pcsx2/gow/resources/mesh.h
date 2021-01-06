#pragma once

#include <unordered_map>
#include <GL/GL.h>

#include "gow/utils.h"

// #define GOW_MESH_DEBUG

namespace gow {

class Mesh;
class MeshObject;

namespace raw {

#pragma pack(push, 1)
#pragma warning(push)
#pragma warning(disable : 4200)

struct stVifTag {
	u32 _tag;

	static const u8 CMD_NOP      = 0x00;
	static const u8 CMD_STCYCL   = 0x01;
	static const u8 CMD_OFFSET   = 0x02;
	static const u8 CMD_BASE     = 0x03;
	static const u8 CMD_ITOP     = 0x04;
	static const u8 CMD_STMOD    = 0x05;
	static const u8 CMD_MSKPATH3 = 0x06;
	static const u8 CMD_MARK     = 0x07;
	static const u8 CMD_FLUSHE   = 0x10;
	static const u8 CMD_FLUSH    = 0x11;
	static const u8 CMD_FLUSHA   = 0x13;
	static const u8 CMD_MSCAL    = 0x14;
	static const u8 CMD_MSCNT    = 0x17;
	static const u8 CMD_MSCALF   = 0x15;
	static const u8 CMD_STMASK   = 0x20;
	static const u8 CMD_STROW    = 0x30;
	static const u8 CMD_STCOL    = 0x31;
	static const u8 CMD_MPG      = 0x4A;
	static const u8 CMD_DIRECT   = 0x50;
	static const u8 CMD_DIRECTHL = 0x51;

	u8 cmd() { return (_tag >> 24) & 0xff; };
    u8 num() { return (_tag >> 16) & 0xff; };
    u16 imm() { return _tag & 0xffff; };
};
static_assert(sizeof(stVifTag) == 0x4, "stVifTag size");

struct stDmaTag {
    u64 _tag;

	static const u8 ID_CNTS = 0x00;
	static const u8 ID_REFE = 0x00;
	static const u8 ID_CNT  = 0x01;
	static const u8 ID_NEXT = 0x02;
	static const u8 ID_REF  = 0x03;
	static const u8 ID_REFS = 0x04;
	static const u8 ID_CALL = 0x05;
	static const u8 ID_RET  = 0x06;
	static const u8 ID_END  = 0x07;

	u32 qwc() { return (_tag >> 0) & 0xffff; };
    u8 id() { return (_tag >> 28) & 0x7; };
    u32 addr() { return (_tag >> 32) & 0x7FffFFff; };

	template <typename T>
	T *subdatab() { return reinterpret_cast<T *>(pointer_add(this, 0x8)); }
	template <typename T>
	T *subdatae() { return reinterpret_cast<T *>(pointer_add(this, 0x10)); }
};
static_assert(sizeof(stDmaTag) == 0x8, "stDmaTag size");

struct stMeshPacketMeta {
	u8 indexesCount;
	u8 _isLastBlock;
	gap_t _gap2[2];

	u32 matFlagsCount : 4;
    u32 indexOrder : 4; // idk actually
	u32 : 4;
    u32 indexOverride : 4; // idk actually, corresponed to indexOrder
	u32 texturing : 4;
	u32 _unk4_0x14 : 4;
	u32 : 4;
    u32 _matFlagsCount2 : 4;

	u32 _matFlags;

	u8 jointIndexes[2];
	gap_t _gap0xE[1];
	u8 jointFlag;

	bool isLast() { return _isLastBlock != 0; }
    u32 matFlag(u32 index) { return (_matFlags >> (index * 4)) & 0xf; }
};
static_assert(sizeof(stMeshPacketMeta) == 0x10, "stMeshPacketMeta size");

struct stMeshObject {
	u16 type;
	u16 _unk2;
	u32 dmaTagsPerProgram;
	u16 materialId;
	u16 jointsMapCountPerInstance;
	u32 instancesCount;
	u32 flags;
	u32 _unk14;
	u8 textureLayersCount;
	u8 _unk19;
	u16 nextFreeVUBuffer;
	union {
		struct {
			u16 metricQuadsCount; // anyway game not uses this
			u16 metricVertexesCount; // or triangles count? or indexes count? game not uses this either
        };
		MeshObject *pMeshObject;
    };
	stDmaTag dmaTags[0];

	bool isUseInvertedMatrix() { return flags & 0x40; }
	void setMeshObject(MeshObject *meshObject) { this->pMeshObject = meshObject; }
	MeshObject *meshObject() { return pMeshObject; }
	u32 totalDmaTagsCount() { return u32(dmaTagsPerProgram) * u32(instancesCount) * u32(textureLayersCount); }
    u32 *jointMaps(){ return (u32 *)(u32(this) + sizeof(*this) + totalDmaTagsCount() * 0x10); }
	u32 *jointMap(u32 instanceIndex) { return &jointMaps()[instanceIndex * u32(jointsMapCountPerInstance)]; }
};
static_assert(sizeof(stMeshObject) == 0x20, "stMeshObject size");

struct stMeshGroup {
	float lodDist;
	u32 objectsCount;
	u32 haveBBoxes;
    u32 objectOffsets[0];

	static const float LOD_MAX;
	stMeshObject *getObject(u32 index) { return (stMeshObject*) pointer_add(this, objectOffsets[index]); }
};
static_assert(sizeof(stMeshGroup) == 0xc, "stMeshGroup size");

struct stMeshPart {
	u16 visible; // 0 - not visible, 1 - visible. probably other meaning or some typing
	u16 groupsCount;
	u32 groupOffsets[0];

	stMeshGroup *getGroup(u32 index) { return (stMeshGroup *)pointer_add(this, groupOffsets[index]); }
};
static_assert(sizeof(stMeshPart) == 0x4, "stMeshPart size");

struct stMesh {
	u32 magic;
	u32 meshDataSize;
	u32 partsCount;
	gap_t _gap[0x28];
	u32 unkBoneIndex;
	char name[0x18];
	u32 partOffsets[0];

	stMeshPart *getPart(u32 index) { return (stMeshPart*)(u32(this) + partOffsets[index]);}
};
static_assert(sizeof(stMesh) == 0x50, "stMesh size");

#pragma warning(pop)
#pragma pack(pop)

} // namespace raw

class MeshObject {
protected:
    struct stProgram {
        GLuint vao;
        GLuint ebo;
        u32 indexesCount;
    };

    struct stDecomileState {
        GLuint xyzw;
        GLuint uv;
        u32 uvWidth;
        GLuint rgba;
        GLuint jointIndexes;
        GLuint normals;
        GLuint element;
        u16 indexesCount;

		void reset() { memset(this, 0, sizeof(stDecomileState)); };
        stProgram getProgram();
    };

	struct stPacketMeta {
		u8 trianglesCount;
		u8 blockId;
		gap_t _gap2[2];
		u32 flags1;
		u32 flags2;
		u8 jointId1;
		u8 jointId2;
		gap_t _gapE[1];
		u8 jointFlag;
	};
	static_assert(sizeof(stPacketMeta) == 0x10, "stPacketMeta size");

	// key - source packet offset (relative to stMeshObject)
	// value - gl buffer that stores data on this offset (can be vertex/uv/normal/indexes)
    std::unordered_map<raw::stVifTag *, GLuint> buffers;

    // represents dma programs
	// keys - instance/layer index
	// value - pair<gl array buffer, indexes count>
    typedef std::vector<stProgram> dma_program_t;

    std::vector<dma_program_t> arrays; 
	raw::stMeshObject *object;
	Mesh *mesh;

	// returns vif data size
    u32 decompileVifUnpack(raw::stVifTag *vif, stDecomileState& state);
    void decompileVifProgram(raw::stVifTag *vifStart, raw::stVifTag *vifEnd, dma_program_t &program, stDecomileState &state);
    void decompileDmaProgram(raw::stDmaTag *dmaProgram, dma_program_t &program);
    void decompilePackets();
public:
    MeshObject(Mesh *mesh, raw::stMeshObject *object);
	~MeshObject();

	std::vector<dma_program_t> &GetPrograms() { return arrays; }
	dma_program_t &GetProgram(u32 instanceId, u32 materialLayer) { return arrays[instanceId  + materialLayer]; }
	Mesh *getMesh() { return mesh; }
};

class Mesh {
protected:
    u32 allocatorOffset;
	raw::stMesh *mesh;
	char name[24];

public:
    Mesh(u32 offset, u32 serverOffset, char* name);
    ~Mesh();

	u32 GetAllocatorOffset() { return allocatorOffset; }
    const char *getName() { return name; }
};

class MeshManager {
protected:
	std::unordered_map<u32, Mesh*> meshes;
public:
    MeshManager(){};

	Mesh *GetMesh(u32 offset);

    void HookInstanceCtor();
    void HookAllocatorDtor(u32 allocatorOffset);
};

} // namespace gow
