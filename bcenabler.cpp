// SPDX-License-Identifier: BSD-2-Clause
// Copyright © 2021 Billy Laws

#include <fstream>
#include <sys/mman.h>
#include <string>
#include "bcenabler.h"
#include "patch_code.h"

typedef uint8_t u8;
typedef int32_t i32;
typedef uint32_t u32;

union Branch {
    struct {
        i32 offset : 26; //!< 26-bit branch offset
        u8 sig : 6;  //!< 6-bit signature (0x25 for linked, 0x5 for jump)
    };

    u32 raw{};
};
static_assert(sizeof(Branch) == sizeof(u32), "Branch size is invalid");

/**
 * @brief Searches /proc/self/maps for the first free page after the given address
 * @return nullptr on error
 */
static void *FindFreePage(uintptr_t address) {
    std::ifstream procMaps("/proc/self/maps");

    unsigned long long end{};

    for (std::string line; std::getline(procMaps, line); ) {
        auto addressSeparator{std::find(line.begin(), line.end(), '-')};
        auto start{std::strtoull(std::string(line.begin(), addressSeparator).c_str(), nullptr, 16)};

        if (end > address && start != end)
            return reinterpret_cast<void *>(end);

        end = std::strtoull(std::string(addressSeparator + 1, std::find(line.begin(), line.end(), ' ')).c_str(), nullptr, 16);
    }

    return nullptr;
}

bool BCN_enable(void *vkGetPhysicalDeviceFormatPropertiesFn) {
    // First branch in this function is targeted at the function we want to patch
    Branch *blInst{reinterpret_cast<Branch *>(vkGetPhysicalDeviceFormatPropertiesFn)};

    constexpr u8 BranchLinkSignature{0x25};

    // Search for branch signature
    while(blInst->sig != BranchLinkSignature)
        blInst++;

    // Internal QGL format conversion function that we need to patch
    u32 *convFormatFn{reinterpret_cast<u32 *>(blInst) + blInst->offset};

    // This would normally set the default result to 0 (error) in the format not found case
    constexpr u32 ClearResultSignature{0x2a1f03e0};

    // We replace it with a branch to our own extended if statement which adds in the extra things for BCn
    u32 *clearResultPtr{convFormatFn};
    while (*clearResultPtr != ClearResultSignature)
        clearResultPtr++;

    // Find the nearest unmapped page where we can place patch code
    void *patchPage{FindFreePage(reinterpret_cast<uintptr_t>(clearResultPtr))};
    if (!patchPage)
        return false;

    void *ptr{mmap(patchPage, PAGE_SIZE,  PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, 0, 0)};
    if (ptr != patchPage)
        return false;

    // Ensure we don't write out of bounds
    if (PatchRawData_size > PAGE_SIZE)
        return false;

    // Copy the patch function to our mapped page
    memcpy(patchPage, PatchRawData, PatchRawData_size);

    // Fixup the patch code so it correctly returns back to the driver after running
    constexpr u32 PatchReturnFixupMagic{0xffffffff};
    constexpr u8 BranchSignature{0x5};

    u32 *fixupTargetPtr = clearResultPtr + 1;
    u32 *fixupPtr = reinterpret_cast<u32 *>(patchPage);
    for (long int i{}; i < (PatchRawData_size / sizeof(u32)); i++, fixupPtr++) {
        if (*fixupPtr == PatchReturnFixupMagic) {
            Branch branchToDriver{
                    {
                            .offset = static_cast<i32>((reinterpret_cast<intptr_t>(fixupTargetPtr) - reinterpret_cast<intptr_t>(fixupPtr)) / sizeof(i32)),
                            .sig = BranchSignature,
                    }
            };

            *fixupPtr = branchToDriver.raw;
        }
    }

    Branch branchToPatch{
            {
                .offset = static_cast<i32>((reinterpret_cast<intptr_t>(patchPage) - reinterpret_cast<intptr_t>(clearResultPtr)) / sizeof(i32)),
                .sig = BranchSignature,
            }
    };

    void *driverPatchPage{reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(clearResultPtr) & ~(PAGE_SIZE - 1))};

    // For some reason mprotect just breaks entirely after we patch the QGL instruction so just set perms to RWX
    if (mprotect(driverPatchPage, PAGE_SIZE,  PROT_WRITE | PROT_READ | PROT_EXEC))
        return false;

    *clearResultPtr = branchToPatch.raw;

    asm volatile("ISB");

    // Done!
    return true;
}
