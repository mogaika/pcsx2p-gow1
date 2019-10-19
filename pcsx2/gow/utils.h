#pragma once

#include "x86/iR5900.h"

namespace gow {

const u32 __ps2_mem_offset = 0x20000000;

template <typename T> T *pmem(u32 offset) { return (T *)(__ps2_mem_offset + offset); };
template <typename T> T *pmemz(u32 offset) { return offset ? pmem<T>(offset) : NULL; };
template <typename T> T rmem(u32 offset) { return *pmem<T>(offset); };

template <typename T> T *pmem(GPR_reg reg) { return pmem<T>(reg.UL[0]); };
template <typename T> T *pmemz(GPR_reg reg) { return pmemz<T>(reg.UL[0]); };
template <typename T> T rmem(GPR_reg reg) { return *pmem<T>(reg); };

static inline u32 revmem(u32 ptr) { return ptr - __ps2_mem_offset; }
static inline u32 revmem(void *ptr) { return revmem((u32)ptr); }

typedef u8 gap_t;
typedef s16 server_id_t;

#pragma pack(push, 1)
struct stIInstance {
	server_id_t managerId;
	server_id_t serverId;
	u16 params;
	u16 instanceIndex;
	u32 list_next;
	u32 list_prev;

	inline u32 offset() { return revmem(this); }
};
static_assert(sizeof(stIInstance) == 0x10, "IInstance size");
#pragma pack(pop)

} // namespace gow
