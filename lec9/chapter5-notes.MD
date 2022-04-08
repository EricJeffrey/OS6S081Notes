
Interrupts and device drivers

- 设备通常使用中断与系统通信
- 许多设备驱动包含两个部分
  - 上半部分：运行在内核中，一般由进程发起系统调用触发
  - 下半部分：在中断时运行，读取控制台输入/将输出写到控制台等，用来确定哪些操作完成了
- PLIC - the riscv Platform Level Interrupt Controller (PLIC). 中断控制器，硬件设备

### 5.1 Code: Console input

- UART串口硬件，Universal Asynchronous Receiver/transmitter 通用异步接收/传输设备，RISC-V使用它来模拟终端
  - UART与16550芯片通信
  - UART表现为一个 memory-mapped control register，内存映射的寄存器；使用虚拟地址直接和硬件的寄存器通信
  - UART包含很多控制寄存器，每个宽度为一个字节
  - UART在 接收到数据/传输完成 时会产生一个中断
- xv6启动时使用`consoleinit`初始化终端，它
  - 调用`uartinit`配置UART硬件
  - 将`devsw`的read/write设置为 `consoleread/consolewrite`
- 程序调用`read`读取标准输入时
  - `file_read(kernel/file.c)`根据文件描述符确定是否为console
  - 然后调用 `devsw[].read` 即 consoleread 读取控制台数据
  - consoleread 等待用户输入。consoleread被唤醒后将数据拷贝回去，然后按照系统调用结束的方式返回到用户态
- 用户输入的时候会产生中断
  - PLIC硬件能够确定中断的来源
  - consoleintr将数据加到缓冲区中，并且在一行完成时唤醒consoleread

### 5.2 Code: Console output

- `uartputc`负责将字符写到UART驱动
  - 使用缓冲区`uart_tx_buf`缓冲数据
  - 缓冲区慢的时候才等待，否则一直向缓冲区加数据
  - UART完成一次写后会产生中断，`uartintr`会处理中断
- decouple 通过中断和缓冲实现了 进程活动 与 设备活动 的解耦
  - 即便没有进程活动，设备也能够读取数据（放到缓冲区）
  - 能够增加性能

### 5.3 Cocurrency in drivers

- consleread/conslewrite 都需要`acquire`加锁，主要为防止三种并发问题
  - 多个CPU可能同时调用`consoleread`
  - 设备可能在CPU执行`consoleread`的时候产生中断
  - 一个CPU执行consoleread的时候，硬件设备可能向另一个CPU传递中断
- 使用锁机制保护硬件的数据结构避免竞争问题
- 另外，发生中断的时候，当前的进程可能已经变了，需要使用锁来唤醒在等待的进程

### 5.4 Timer interrupts

- 时钟中断实现时间片`yield`
- RISC-V要求时钟中断在 machine mode 处理
- `kernel/start.c`
  - 设置CLINT(Core-Local INTerruptor)硬件每隔一段时间产生中断
  - 设置`scratch`区域用来保存寄存器和他们的地址（类似于trapframe）
  - 将`mtvec`执行`kernel/kernelvec.S:timervec`启用时钟中断
- 内核执行的时候无法禁用时钟中断，因此时钟中断处理必须 **不能干扰到内核代码**
- xv6使用 **软中断** 实现，`timervec`会配置CLINT下一次中断的时间，之后raise软中断

### 5.5 Real World

- xv6内核会被timer中断，因此内核代码可能会被挂起，之后在不同的CPU上恢复，从而造成了一定的复杂性
- UART的通过control register来读取数据的方式成为 *programmed I/O*，与之相对的是 *DMA(Direct Memory Access)*，效率更高
- 中断的方式不适合高速设备，网络或磁盘等，通常会使用一些trick来加速
  - 缓冲一批数据后再产生中断
  - 或禁用中断但是周期性的检查设备状态，称之为 *polling*
- UART需要 **设备存储 -> 内核态 -> 用户态** 两次拷贝，效率低，其它操作系统通常支持 **设备存储 -> 用户态** 一次拷贝
- Xv6不适合 硬实时（时间完全可控，不允许超过deadline） 和 软实时（允许偶尔的超过deadline，但还是可控） 任务
  - 内核中包含一段较长的（中断禁用时的）执行路径，因此不可控
