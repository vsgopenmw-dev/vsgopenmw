Which platforms does vsgopenmw support?
=======================================

vsgopenmw doesn't support platforms.

Which platforms support vsgopenmw?
==================================

vsgopenmw should appear in your Linux or FreeBSD package repository of choice soon. In the meantime, you can build vsgopenmw the usual cmake way on any platform that comes with a standards-compliant C++20 environment and Vulkan support. Clang is recommended.

Which platforms don't support vsgopenmw?
========================================

Windows doesn't support vsgopenmw because Windows is not standards compliant and Windows doesn't support applications; applications support Windows.

Android doesn't support vsgopenmw because Android doesn't support applications; applications support Android.

Mac OS X and iOS don't support vsgopenmw because Apple doesn't support Vulkan.

How do I use vsgopenmw on an unsupporting platform?
===================================================

You can simply install a minimal version of Linux, also known as proot or Linux-on-<platform> on top of your platform. You can get native performance with this method because Vulkan drivers load in userspace. To ease the process, vsgopenmw includes the VSGOPENMW_INDIRECT fall back to indirect rendering (not to be confused with software rendering) in case a direct Vulkan surface is not available.

Android (no root required and no PC required): On Android, you can build and run vsgopenmw using your Termux proot-distro of choice in combination with XServer-XSDL, VNC or Termux-X11. On Adreno GPU devices, you can get hardware acceleration with mesa-turnip-kgsl.
