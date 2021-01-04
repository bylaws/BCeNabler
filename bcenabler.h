// SPDX-License-Identifier: BSD-2-Clause
// Copyright © 2021 Billy Laws

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/**
 * @brief Patches the Adreno graphics driver to enable support for BCn compressed formats
 * @param vkGetPhysicalDeviceFormatPropertiesFn A pointer to vkGetPhysicalDeviceFormatProperties obtained through vkGetInstanceProcAddr; this is used to find the correct function to patch
 * @return If the patching succeeded, on fail the driver will be in an undefined state
 */
bool BCN_enable(void *vkGetPhysicalDeviceFormatPropertiesFn);

#ifdef __cplusplus
}
#endif
