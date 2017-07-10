/* 
 * the drv_vinput.h is connceted with the vinput driver 
 * the vinput driver locates at kernel/driver/misc/vinput.c
 *
 * */
#ifndef __VINPUT_H__
#define __VINPUT_H__

#define IOCTL_MOUSE_STATUS      _IOW('i', 0x100, unsigned long)
#define IOCTK_KBD_STATUS        _IOW('i', 0x101, unsigned long)
#define IOCTK_TC_STATUS         _IOW('i', 0x102, unsigned long)
#define IOCTK_MUTITC_STATUS     _IOW('i', 0x103, unsigned long)

#define INPUT_UNBLOCK       0
#define INPUT_BLOCK         1
#define INPUT_HALFBLOCK     2
#define INPUT_POWER         116
#define INPUT_RESERVED      0
// sky(stby)
// Bluetooth Remote Control
// input.c  (bigfish\sdk\source\kernel\linux-3.10.y\drivers\input)
// vinput.h (bigfish\sdk\source\kernel\linux-3.10.y\include\linux)
// type 4(EV_MSC), code 4(MSC_SCAN)
#define INPUT_NVKPOWER 		0xC0030 // 116:power
#define INPUT_ALIPOWER		0x70066
#define INPUT_GENPOWER		0x10081
#define INPUT_GENSLEEP		0x10082 // 142:usb keboard
#define INPUT_BTNPOWER		0x90003
#define INPUT_BTNF11		0x70044 // 141:usb keboard
#define INPUT_NVKMETALEFT	0x700E3 // 125:meta-left(unused)

#endif

