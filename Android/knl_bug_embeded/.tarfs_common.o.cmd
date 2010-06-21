cmd_fs/tarfs/tarfs_common.o := ../prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/arm-eabi-gcc -Wp,-MD,fs/tarfs/.tarfs_common.o.d  -nostdinc -isystem /mnt/lvm/android/platform_2.1_r2/prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/../lib/gcc/arm-eabi/4.4.0/include -Iinclude  -I/mnt/lvm/android/platform_2.1_r2/kernel/arch/arm/include -include include/linux/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-goldfish/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Os -marm -fno-omit-frame-pointer -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -D__LINUX_ARM_ARCH__=5 -march=armv5te -mtune=arm9tdmi -msoft-float -Uarm -Wframe-larger-than=1024 -fno-stack-protector -fno-omit-frame-pointer -fno-optimize-sibling-calls -Wdeclaration-after-statement -Wno-pointer-sign -fwrapv -fno-dwarf2-cfi-asm  -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(tarfs_common)"  -D"KBUILD_MODNAME=KBUILD_STR(tarfs)"  -c -o fs/tarfs/tarfs_common.o fs/tarfs/tarfs_common.c

deps_fs/tarfs/tarfs_common.o := \
  fs/tarfs/tarfs_common.c \
  fs/tarfs/tarfs_common.h \
  fs/tarfs/tarfs_osdep.h \
  include/asm-generic/errno-base.h \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbd.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  include/linux/posix_types.h \
  include/linux/stddef.h \
  include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  include/linux/compiler-gcc4.h \
  /mnt/lvm/android/platform_2.1_r2/kernel/arch/arm/include/asm/posix_types.h \
  /mnt/lvm/android/platform_2.1_r2/kernel/arch/arm/include/asm/types.h \
  include/asm-generic/int-ll64.h \
  fs/tarfs/tarfs_meta.h \

fs/tarfs/tarfs_common.o: $(deps_fs/tarfs/tarfs_common.o)

$(deps_fs/tarfs/tarfs_common.o):
