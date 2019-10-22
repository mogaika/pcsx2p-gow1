#pragma once

#include <unordered_map>
#include <GL/GL.h>

#include "gow/utils.h"

namespace gow {

class Mesh;
class MeshObject;

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
static_assert(sizeof(stVifTag) == 0x4, "mesh vif packet size");

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
    u32 addr(){ return (_tag >> 32) & 0x7FffFFff; };
};
static_assert(sizeof(stDmaTag) == 0x8, "mesh dma packet size");

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

	MeshObject *&refMeshObject() { return pMeshObject; }
};
static_assert(sizeof(stMeshObject) == 0x20, "meshobject size");

struct stMeshGroup {
	u32 _unk0;
	u32 objectsCount;
	u32 _unk8;
    u32 objectOffsets[0];

	stMeshObject *getObject(u32 index) { return (stMeshObject*) pointer_add(this, objectOffsets[index]); }
};
static_assert(sizeof(stMeshGroup) == 0xc, "meshgroup size");

struct stMeshPart {
	u16 visible; // 0 - not visible, 1 - visible. probably other meaning or some typing
	u16 groupsCount;
	u32 groupOffsets[0];

	stMeshGroup *getGroup(u32 index) { return (stMeshGroup *)pointer_add(this, groupOffsets[index]); }
};
static_assert(sizeof(stMeshPart) == 0x4, "meshpart size");

struct stMesh {
	u32 magic;
	union {
		u32 meshCommentStart; // used only at start for allocation size detection. we can use this
		Mesh *pMesh;
    };
	u32 partsCount;
	gap_t _gap[0x28];
	u32 unkBoneIndex;
	char name[0x18];
	u32 partOffsets[0];

	stMeshPart *getPart(u32 index) { return (stMeshPart*)(u32(this) + partOffsets[index]);}
    Mesh *&refMesh() { return pMesh; }
};
static_assert(sizeof(stMesh) == 0x50, "mesh size");

#pragma warning(pop)
#pragma pack(pop)

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
	static_assert(sizeof(stPacketMeta) == 0x10, "packet meta size");

	// key - source packet offset (relative to stMeshObject)
	// value - gl buffer that stores data on this offset (can be vertex/uv/normal/indexes)
	std::unordered_map<stVifTag*, GLuint> buffers;

    // represents dma programs
	// keys - instance/layer index
	// value - pair<gl array buffer, indexes count>
    typedef std::vector<stProgram> dma_program_t;

    std::vector<dma_program_t> arrays; 
	stMeshObject *object;

	// returns vif data size
    u32 decompileVifUnpack(stVifTag *vif, stDecomileState& state);
	void decompileVifProgram(stVifTag *vifStart, stVifTag *vifEnd, dma_program_t &program);
    void decompileDmaProgram(stDmaTag *dmaProgram, dma_program_t &program);
    void decompilePackets();
public:
	MeshObject(stMeshObject *object);
	~MeshObject();

	std::vector<dma_program_t> &GetPrograms() { return arrays; }
};

class Mesh {
protected:
	u32 serverOffset;
	stMesh *mesh;

public:
    Mesh(u32 offset, u32 serverOffset);
    ~Mesh();

	u32 GetServerOffset() { return serverOffset; }
};

class MeshManager {
protected:
	std::unordered_map<u32, Mesh*> meshes;
public:
    MeshManager(){};

	Mesh *GetMesh(u32 offset);

    void HookInstanceCtor(u32 serverOffset);
    void HookServerDtor();
};


} // namespace gow
