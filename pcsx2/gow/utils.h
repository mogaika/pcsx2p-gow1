#pragma once

#include "x86/iR5900.h"

namespace gow {

template <typename T> T *pmem(u32 offset) { return (T *)(0x20000000 + offset); };
template <typename T> T *pmem(GPR_reg reg) { return pmem<T>(reg.UL[0]); };
template <typename T> T rmem(u32 offset) { return *pmem<T>(offset); };
template <typename T> T rmem(GPR_reg reg) { return *pmem<T>(reg); };

} // namespace gow
