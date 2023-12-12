Which platforms support vsgopenmw?
==================================

Currently there are no releases or downloadable packages of vsgopenmw. 


Where and how can I build vsgopenmw?
====================================

You can build vsgopenmw the usual cmake way on any platform that comes with a standards-compliant C++20 environment and Vulkan support. In addition to OpenMW's dependencies, you'll need VulkanSceneGraph-1.0.9 and vsgXchange installed the usual cmake way. OpenSceneGraph headers (not libraries) are still required for the time being.


How do I use vsgopenmw on an Linux-on-<OS> environment?
=======================================================

You can simply install a minimal version of Linux, also known as Linux-on-<OS> on top of your OS. You can get native performance with this method because Vulkan drivers load in userspace. To ease the process, vsgopenmw includes the VSGOPENMW_INDIRECT fall back to indirect rendering (not to be confused with software rendering) in case a direct Vulkan surface is not available.


Android
=======

Credit for this guide goes to Grima04 and the XDA-Developers community.

On Android, we can build and run vsgopenmw using Termux with no need for a PC or rooted device. Termux doesn't officially support hardware acceleration, but it provides a Vulkan loader library and that's all we need.

We can either use Termux' vulkan-loader-android to access the Android system's Vulkan driver or we can install a full proot-distro Linux system and use a self built driver with it (e.g. Freedreno-KGSL). For graphical output we can use XServer-XSDL, VNC or Termux-X11 in VSGOPENMW_INDIRECT mode. Direct rendering is yet to be explored.
