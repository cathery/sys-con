# sys-con

#### A Nintedo Switch sysmodule for third-party controller support. No man-in-the-middle required! 
###### \[Switch FW 5.0.0+\]


## Description
This sysmodule aims to provide complete functionality for most popular game controllers not supported by Nintendo Switch.
At the current moment, **only USB connection** is supported. 

This app is missing a lot of features. For more information, see the [issues page](https://github.com/cathery/sys-con/issues).

### ⚠ I can't support your generic 3rd party HID controller yet. It is a limitation of the firmware and I'm looking to work around it.

### ⚠ If you get the error 2003-0008 (0x1003), you're running too many sysmodules. Disable other memory demanding sysmodules like sys-ftpd or ldn_mitm.



## Install

Grab the latest zip from the [releases page](https://github.com/cathery/sys-con/releases). Extract it in your SD card and boot/reboot your switch.

## Config

sys-con comes with a config folder located at `sdmc:/config/sys-con/`. It contains options for adjusting stick/trigger deadzone, as well as remapping inputs. For more information, see `example.ini` in the same folder. All changes to the files will be updated in real time.

## Progress roadmap
- [x] **~~Docked USB Support~~**
- [x] **~~\[5.0.0-7.0.0\] FW Version Support~~**
- [x] **~~Xbox 360 Controller Support~~**
- [x] **~~Xbox One X/S Controller Support~~**
- [x] **~~Dualshock 3 Support~~**
- [x] **~~Undocked USB Support~~** Works with a USB-C OTG adapter. Some knock-off brands may not support OTG.
- [x] **~~Xbox 360 Wireless adapter~~**
- [x] **~~Dualshock 4 Support~~**
- [ ] **[Xbox One Wireless adapter](https://github.com/cathery/sys-con/issues/36)**
- [ ] **[Rumble Support](https://github.com/cathery/sys-con/issues/1)**
- [ ] **[Bluetooth Support](https://github.com/cathery/sys-con/issues/5)**
- [ ] **[Motion Controls Support](https://github.com/cathery/sys-con/issues/9)**
- [ ] **Config application**

## Building (For developers)

If you want to build this sysmodule yourself, you need to download the latest build of [libnx](https://github.com/switchbrew/libnx) and build it, then put the library and includes into your devkitpro libnx directory. Make a backup first. 
After that, you can either type `make -f MakefileSysmodule -j8` to build a sysmodule nsp file, or `make -f MakefileApplet -j8` to build an applet nro file. Sorry that there's two makefiles, I can't be bothered to figure out the make language right now.

## Support
[![ko-fi](https://www.ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/H2H316ZQV)

If you wish to see added support for more controllers in the future, consider funding my project on Ko-fi!
