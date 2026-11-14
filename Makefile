# AIKON-VXP: plain-C MediaTek MRE build, no arm-none-eabi-gcc required.
# Uses clang's built-in bare-metal ARM target + lld + objcopy instead.
# Point MRE_SDK at a checkout of https://github.com/gtrxAC/mre-makefile
# (its sdk/ folder carries MediaTek's proprietary libs/headers, which
# are not redistributed here).

MRE_SDK ?= ../mre-makefile/sdk
APP = NetTest

CLANGFLAGS = -target arm-none-eabi -fpic -march=armv5te -fvisibility=hidden \
	-Os -mlittle-endian -Ifreestanding-shim -I$(MRE_SDK)/include \
	-D_MINIGUI_LIB_ -D_USE_MINIGUIENTRY -D_NOUNIX_ -D_FOR_WNC \
	-D__MRE_SDK__ -D__MRE_VENUS_NORMAL__ -D__MMI_MAINLCD_240X320__ \
	-DMRE -DGCC -D__MRE_COMPILER_GCC__ -fdata-sections -ffunction-sections \
	-ffreestanding

all: $(APP).vxp

build:
	mkdir -p build

build/%.o: src/%.c | build
	clang $(CLANGFLAGS) -c $< -o $@

build/gccmain.o: $(MRE_SDK)/../src/gccmain.c | build
	clang $(CLANGFLAGS) -c $< -o $@

build/c_fix.o: $(MRE_SDK)/../src/c_fix.c | build
	clang $(CLANGFLAGS) -Wno-return-type -c $< -o $@

build/$(APP).axf: build/gccmain.o build/c_fix.o build/net_test.o
	clang -target arm-none-eabi -nostdlib -fuse-ld=lld -Wl,-pie,--no-dynamic-linker,-z,notext,--gc-sections \
		-T $(MRE_SDK)/scat.ld -o $@ $^ $(MRE_SDK)/lib/*.a

build/$(APP)_res.axf: build/$(APP).axf
	objcopy -I elf32-little --add-section .vm_res=$(MRE_SDK)/resource.bin $< $@

$(APP).vxp: build/$(APP)_res.axf manifest.json vxp_pack.py
	python3 vxp_pack.py build/$(APP)_res.axf $@ manifest.json

clean:
	rm -rf build *.vxp

.PHONY: all clean
