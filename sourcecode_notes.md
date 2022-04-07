
### 文件结构

1. kernel目录
   1. `defs.h` 所有函数的定义
   2. syscall实现相关
      1. `syscall.h` 系统调用编号的宏定义
      2. `syscall.c` 和参数读取相关的函数，以及所有系统调用的代理函数syscall
      3. `sysproc.c` 和进程相关的系统调用的实现，exec getpid wait...
      4. `sysfile.c` 和文件操作相关的系统调用实现，read write...
   3. `memlayout.h` riscv的内存布局 宏定义
   4. `vm.c` 页表操作 mappages freewalk...
   5. `kalloc.c` 物理地址分配相关 kinit kalloc kfree
   6. `riscv.h` 汇编指令的wrapper
   7. `proc.c/h` 和进程、页表相关的实现 proc_pagetabl allocproc freeproc...
2. user目录
   1. `usys.pl` 系统调用的入口


### asm汇编相关

1. `auipc` 将立即数与PC的upper20位相加，并保存到目的寄存器 -> `auipc rd, imm`
2. `jalr` 跳转并link寄存器，跳转到某个标签并将下一个pc的值放到目的寄存器 -> `jalr rd, offset`
