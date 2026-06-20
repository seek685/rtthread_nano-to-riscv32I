根据makefile构建编译
mingw32-make.exe 就是 GNU Make 的 Windows 编译版本
输入ming32-make即可编译 编译完成有很多警告无须理会 都是很多函数没有被调用到
也可以打开vscode的bin文件找到ming32-make.exe然后重命名为make.exe
之后只需输入make即可编译成功
编译的时候多了.d文件 d文件可以追踪哪些文件被修改了需要重新编译 不需要重新输入make clean清除旧文件重新编译省去了清除这一步 每次只需输入make即可
之后终端输入qemu-system-riscv32 -machine virt -nographic -bios none -kernel rtthread.elf 即可成功在qemu上跑
