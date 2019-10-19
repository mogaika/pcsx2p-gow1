#pragma once

#include <stdint.h>

#include "x86/iR5900.h"

namespace gow {

template<typename T>
T *pmem(uint32_t pointer) {
	return (T*)(0x200000 + pointer);
}

template <typename T>
T pmem(GPR_reg reg) {
    return pmem<T>(reg.UL[0]);
}

template <typename T>
T rmem(uint32_t offset) {
    return *pmem<T>(offset);
}

template <typename T>
T rmem(GPR_reg reg) {
    return rmem<T>(reg.UL[0]);
}

} // namespace gow
