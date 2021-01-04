## BCeNabler

A small library to patch Adreno GPU drivers to support BCn textures which have been supported internally forever but not exposed until the latest 865+ drivers.


Validation should be performed on the application side before running to ensure the GPU is compatable and avoid blindly corrupting the driver.

Usage:
```
bool ret = BCN_enable((void *)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties"));
```
