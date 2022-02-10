#  目录变量 用它来存储编译生成所有的目标文件
BUILD_DIR = ./build
#  虚拟地址
ENTRY_POINT = 0xc0001500
AS = nasm
CC = gcc
LD = ld
LIB = -I lib/ -I lib/kernel/ -I lib/user/ -I kernel/ -I device/
ASFLAGS = -f elf
#  CFLAGS 中定义了-fno-builtin， 它是告诉编译器不要采用内部函数，因为咱们在以后实现中会自定义与内部函数同名的函数，如果不添加此选项的话， 编译时 gcc 会提示与内部函数冲突。-Wstrict-prototypes 选项要求函数声明中必须有参数类型，否则编译时发出警告。-Wmissing-prototypes 选项要求函数必须有声明，否则编译时发出警告。
# 增加-m32 指定gcc在32 位下进行编译
CFLAGS = -m32 -Wall $(LIB) -c -fno-builtin -fno-stack-protector -w #-Wstrict-prototypes -Wmissing-prototypes 
# 增加 -m elf_i386  ELF for i386 — 32-bit i386 binaries
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map
# 存储所有的目标文件名，以后每增加一个目标文件，直接在此变量中增加就行了，此变量用在链接阶段。位置顺序上最好还是调用在前，实现在后。
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o \
      $(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o \
      $(BUILD_DIR)/debug.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/bitmap.o \
      $(BUILD_DIR)/string.o  $(BUILD_DIR)/thread.o

##############     c代码编译     ###############
$(BUILD_DIR)/main.o: kernel/main.c lib/kernel/print.h \
        lib/stdint.h kernel/init.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/init.o: kernel/init.c kernel/init.h lib/kernel/print.h \
        lib/stdint.h kernel/interrupt.h device/timer.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c kernel/interrupt.h \
        lib/stdint.h kernel/global.h lib/kernel/io.h lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/timer.o: device/timer.c device/timer.h lib/stdint.h\
         lib/kernel/io.h lib/kernel/print.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/debug.o: kernel/debug.c kernel/debug.h \
        lib/kernel/print.h lib/stdint.h kernel/interrupt.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/string.o: lib/string.c lib/string.h lib/stdint.h kernel/global.h \
	lib/stdint.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/bitmap.o: lib/kernel/bitmap.c lib/kernel/bitmap.h \
    	kernel/global.h lib/stdint.h lib/string.h lib/stdint.h \
     	lib/kernel/print.h kernel/interrupt.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/memory.o: kernel/memory.c kernel/memory.h lib/stdint.h lib/kernel/bitmap.h \
   	kernel/global.h kernel/global.h kernel/debug.h lib/kernel/print.h \
	lib/kernel/io.h kernel/interrupt.h lib/string.h lib/stdint.h
	$(CC) $(CFLAGS) $< -o $@


$(BUILD_DIR)/thread.o: thread/thread.c thread/thread.h lib/stdint.h \
        kernel/global.h lib/kernel/bitmap.h kernel/memory.h lib/string.h \
        lib/stdint.h lib/kernel/print.h kernel/interrupt.h kernel/debug.h
	$(CC) $(CFLAGS) $< -o $@

##############    汇编代码编译    ###############
$(BUILD_DIR)/kernel.o: kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/print.o: lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@

##############    链接所有目标文件    #############
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY : mk_dir hd clean all
# 伪目标mk_dir 用来建立build 目录，
mk_dir:
	if [ ! -d $(BUILD_DIR) ];then mkdir $(BUILD_DIR);fi
# 伪目标 hd 是将 build/kernel.bin 写入硬盘，执行 make hd 是将文件写入硬盘。
hd:
	dd if=$(BUILD_DIR)/kernel.bin \
           of=/home/isam2016/opt/bochs/hd60M.img \
           bs=512 count=200 seek=9 conv=notrunc
# 伪目标 clean 是将 build 目录下的文件清空。为稳妥起见，先成功进入 build 目录后再执行“rm –f ./*”删除此目录下的所有文件，避免错删文件。执行 make clean 将会清空 build 目录下的文件。
clean:
	cd $(BUILD_DIR) && rm -f ./*
# 伪目标 build 就是编译 kernel.bin，只要执行 make build 就是编译文件。
build: $(BUILD_DIR)/kernel.bin
# 伪目标 all 是依次执行伪目标 mk_dir build hd。只要执行 make all 便完成了编译到写入硬盘的全过程
all: mk_dir build hd
