一些问题

1. LEC3的[视频](https://www.youtube.com/watch?v=o44d---Dk4o&ab_channel=DavidMorejon)中提到指令集的用户模式和特权模式通过一个标志位(flag)来区分，CPU根据这个状态位确定当前的模式，而更改这个标志位是**特权操作**，也就是需要内核模式才能修改。那么，一旦CPU设定为用户模式，不就无法再进入内核模式了吗？
    
    RISC-V指令集提供了`ecall`指令，该指令使CPU从用户模式转为内核模式(Supervisor mode)，并执行到内核指定的入口点程序。

2. LEC3中提到了对CPU的复用(multiplex)，实际上是对CPU进行时间片复用。那么`epoll, select, poll`这些I/O多路复用，复用的是什么？

    I/O多路复用是通过一个事件循环监听多个文件描述符，相比多线程的方式，复用的是线程

3. kernel/exec.c中`exec`是如何执行加载的程序的？

   通过修改当前的程序计数器PC实现程序执行。例如内核启动的第一个程序init，其执行了`exec("/init")`的汇编指令来启动init进程，在kernel/exec.c:115可以看到其设置了`p->trapframe->epc=elf.entry`。

4. 书中3.8节第37页中描述的`ustack`的前三个入口的`fake return program counter`是什么？

5. gdb调试出现`.gdbinit:2: Error in sourced command file:`错误？

   使用`gdb-multiarch`