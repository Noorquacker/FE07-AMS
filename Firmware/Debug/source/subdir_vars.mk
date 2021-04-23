################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
CMD_SRCS += \
../source/sys_link.cmd 

ASM_SRCS += \
../source/dabort.asm \
../source/sys_core.asm \
../source/sys_intvecs.asm \
../source/sys_mpu.asm \
../source/sys_pmu.asm 

C_SRCS += \
../source/FE_AMS.c \
../source/adc.c \
../source/can.c \
../source/crc.c \
../source/dcc.c \
../source/emac.c \
../source/errata_SSWF021_45.c \
../source/esm.c \
../source/gio.c \
../source/het.c \
../source/i2c.c \
../source/lin.c \
../source/mdio.c \
../source/mibspi.c \
../source/notification.c \
../source/phy_dp83640.c \
../source/pinmux.c \
../source/pl455.c \
../source/rti.c \
../source/sci.c \
../source/spi.c \
../source/sys_dma.c \
../source/sys_main.c \
../source/sys_pcr.c \
../source/sys_phantom.c \
../source/sys_pmm.c \
../source/sys_selftest.c \
../source/sys_startup.c \
../source/sys_vim.c \
../source/system.c \
../source/usb.c \
../source/usb_serial_structs.c \
../source/usbbuffer.c \
../source/usbdcdc.c \
../source/usbdcdesc.c \
../source/usbdconfig.c \
../source/usbdenum.c \
../source/usbdesc.c \
../source/usbringbuf.c 

C_DEPS += \
./source/FE_AMS.d \
./source/adc.d \
./source/can.d \
./source/crc.d \
./source/dcc.d \
./source/emac.d \
./source/errata_SSWF021_45.d \
./source/esm.d \
./source/gio.d \
./source/het.d \
./source/i2c.d \
./source/lin.d \
./source/mdio.d \
./source/mibspi.d \
./source/notification.d \
./source/phy_dp83640.d \
./source/pinmux.d \
./source/pl455.d \
./source/rti.d \
./source/sci.d \
./source/spi.d \
./source/sys_dma.d \
./source/sys_main.d \
./source/sys_pcr.d \
./source/sys_phantom.d \
./source/sys_pmm.d \
./source/sys_selftest.d \
./source/sys_startup.d \
./source/sys_vim.d \
./source/system.d \
./source/usb.d \
./source/usb_serial_structs.d \
./source/usbbuffer.d \
./source/usbdcdc.d \
./source/usbdcdesc.d \
./source/usbdconfig.d \
./source/usbdenum.d \
./source/usbdesc.d \
./source/usbringbuf.d 

OBJS += \
./source/FE_AMS.obj \
./source/adc.obj \
./source/can.obj \
./source/crc.obj \
./source/dabort.obj \
./source/dcc.obj \
./source/emac.obj \
./source/errata_SSWF021_45.obj \
./source/esm.obj \
./source/gio.obj \
./source/het.obj \
./source/i2c.obj \
./source/lin.obj \
./source/mdio.obj \
./source/mibspi.obj \
./source/notification.obj \
./source/phy_dp83640.obj \
./source/pinmux.obj \
./source/pl455.obj \
./source/rti.obj \
./source/sci.obj \
./source/spi.obj \
./source/sys_core.obj \
./source/sys_dma.obj \
./source/sys_intvecs.obj \
./source/sys_main.obj \
./source/sys_mpu.obj \
./source/sys_pcr.obj \
./source/sys_phantom.obj \
./source/sys_pmm.obj \
./source/sys_pmu.obj \
./source/sys_selftest.obj \
./source/sys_startup.obj \
./source/sys_vim.obj \
./source/system.obj \
./source/usb.obj \
./source/usb_serial_structs.obj \
./source/usbbuffer.obj \
./source/usbdcdc.obj \
./source/usbdcdesc.obj \
./source/usbdconfig.obj \
./source/usbdenum.obj \
./source/usbdesc.obj \
./source/usbringbuf.obj 

ASM_DEPS += \
./source/dabort.d \
./source/sys_core.d \
./source/sys_intvecs.d \
./source/sys_mpu.d \
./source/sys_pmu.d 

OBJS__QUOTED += \
"source\FE_AMS.obj" \
"source\adc.obj" \
"source\can.obj" \
"source\crc.obj" \
"source\dabort.obj" \
"source\dcc.obj" \
"source\emac.obj" \
"source\errata_SSWF021_45.obj" \
"source\esm.obj" \
"source\gio.obj" \
"source\het.obj" \
"source\i2c.obj" \
"source\lin.obj" \
"source\mdio.obj" \
"source\mibspi.obj" \
"source\notification.obj" \
"source\phy_dp83640.obj" \
"source\pinmux.obj" \
"source\pl455.obj" \
"source\rti.obj" \
"source\sci.obj" \
"source\spi.obj" \
"source\sys_core.obj" \
"source\sys_dma.obj" \
"source\sys_intvecs.obj" \
"source\sys_main.obj" \
"source\sys_mpu.obj" \
"source\sys_pcr.obj" \
"source\sys_phantom.obj" \
"source\sys_pmm.obj" \
"source\sys_pmu.obj" \
"source\sys_selftest.obj" \
"source\sys_startup.obj" \
"source\sys_vim.obj" \
"source\system.obj" \
"source\usb.obj" \
"source\usb_serial_structs.obj" \
"source\usbbuffer.obj" \
"source\usbdcdc.obj" \
"source\usbdcdesc.obj" \
"source\usbdconfig.obj" \
"source\usbdenum.obj" \
"source\usbdesc.obj" \
"source\usbringbuf.obj" 

C_DEPS__QUOTED += \
"source\FE_AMS.d" \
"source\adc.d" \
"source\can.d" \
"source\crc.d" \
"source\dcc.d" \
"source\emac.d" \
"source\errata_SSWF021_45.d" \
"source\esm.d" \
"source\gio.d" \
"source\het.d" \
"source\i2c.d" \
"source\lin.d" \
"source\mdio.d" \
"source\mibspi.d" \
"source\notification.d" \
"source\phy_dp83640.d" \
"source\pinmux.d" \
"source\pl455.d" \
"source\rti.d" \
"source\sci.d" \
"source\spi.d" \
"source\sys_dma.d" \
"source\sys_main.d" \
"source\sys_pcr.d" \
"source\sys_phantom.d" \
"source\sys_pmm.d" \
"source\sys_selftest.d" \
"source\sys_startup.d" \
"source\sys_vim.d" \
"source\system.d" \
"source\usb.d" \
"source\usb_serial_structs.d" \
"source\usbbuffer.d" \
"source\usbdcdc.d" \
"source\usbdcdesc.d" \
"source\usbdconfig.d" \
"source\usbdenum.d" \
"source\usbdesc.d" \
"source\usbringbuf.d" 

ASM_DEPS__QUOTED += \
"source\dabort.d" \
"source\sys_core.d" \
"source\sys_intvecs.d" \
"source\sys_mpu.d" \
"source\sys_pmu.d" 

C_SRCS__QUOTED += \
"../source/FE_AMS.c" \
"../source/adc.c" \
"../source/can.c" \
"../source/crc.c" \
"../source/dcc.c" \
"../source/emac.c" \
"../source/errata_SSWF021_45.c" \
"../source/esm.c" \
"../source/gio.c" \
"../source/het.c" \
"../source/i2c.c" \
"../source/lin.c" \
"../source/mdio.c" \
"../source/mibspi.c" \
"../source/notification.c" \
"../source/phy_dp83640.c" \
"../source/pinmux.c" \
"../source/pl455.c" \
"../source/rti.c" \
"../source/sci.c" \
"../source/spi.c" \
"../source/sys_dma.c" \
"../source/sys_main.c" \
"../source/sys_pcr.c" \
"../source/sys_phantom.c" \
"../source/sys_pmm.c" \
"../source/sys_selftest.c" \
"../source/sys_startup.c" \
"../source/sys_vim.c" \
"../source/system.c" \
"../source/usb.c" \
"../source/usb_serial_structs.c" \
"../source/usbbuffer.c" \
"../source/usbdcdc.c" \
"../source/usbdcdesc.c" \
"../source/usbdconfig.c" \
"../source/usbdenum.c" \
"../source/usbdesc.c" \
"../source/usbringbuf.c" 

ASM_SRCS__QUOTED += \
"../source/dabort.asm" \
"../source/sys_core.asm" \
"../source/sys_intvecs.asm" \
"../source/sys_mpu.asm" \
"../source/sys_pmu.asm" 


