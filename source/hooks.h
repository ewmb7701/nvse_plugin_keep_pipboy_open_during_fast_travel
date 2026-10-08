#pragma once
#include <Windows.h>
#include <cstdint>
#include <cstring>

// Write a five-byte x86 relative branch into executable memory.
inline void WriteRelBranch(uintptr_t address, uintptr_t target, uint8_t opcode) {
 auto* site = reinterpret_cast<void*>(address);
 DWORD protection;
 VirtualProtect(site, 5, PAGE_EXECUTE_READWRITE, &protection);
 uint8_t bytes[5] = {opcode};
 const auto displacement = static_cast<int32_t>(target - (address + 5));
 std::memcpy(bytes + 1, &displacement, 4);
 std::memcpy(site, bytes, sizeof(bytes));
 DWORD ignored;
 VirtualProtect(site, sizeof(bytes), protection, &ignored);
 FlushInstructionCache(GetCurrentProcess(), site, sizeof(bytes));
}

inline void WriteRelCall(uintptr_t address, uintptr_t target) {
 WriteRelBranch(address, target, 0xE8);
}
inline void WriteRelJump(uintptr_t address, uintptr_t target) {
 WriteRelBranch(address, target, 0xE9);
}

// Accept hook functions directly; keep address conversion in the patch writer.
template <class Function>
inline void WriteRelCall(uintptr_t address, Function *target) {
 WriteRelCall(address, reinterpret_cast<uintptr_t>(target));
}
template <class Function>
inline void WriteRelJump(uintptr_t address, Function *target) {
 WriteRelJump(address, reinterpret_cast<uintptr_t>(target));
}
