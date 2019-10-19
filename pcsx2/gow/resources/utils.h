#pragma once

#include "x86/iR5900.h"

namespace gow {

template <typename T> T *pmem(u32 pointer) { return (T*)(0x20000000 + pointer); }
template <typename T> T *pmem(GPR_reg reg) { return pmem<T>(reg.UL[0]); }
template <typename T> T rmem(u32 offset) { return *pmem<T>(offset); }
template <typename T> T rmem(GPR_reg reg) { return rmem<T>(reg.UL[0]); }

} // namespace gow
