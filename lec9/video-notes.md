
Video about interrupts

- 现在就知道 `top` 的输出，VIRT指的是进程的内存空间大小，RES指的是实际驻留在物理内存的大小，SHR指共享库之类的内存
- 流程：保存现场；处理中断；恢复现场
- 中断的特点
  - 异步asynchroonous
  - 并发concurrency
  - 编程设备program device
- PLIC对中断的分发由内核控制，PLIC_CLAIM
- 可编程设备通常使用 Memory Mapped IO，设备地址被映射到到RAM的特定地址，厂商决定
- UART不保证每个字符都会被写出，需要由驱动的编码保证
- 中断相关寄存器
  - SIE(Supervisor Interrupt Enable)寄存器，能够控制外部中断、软中断和定时器中断
  - SSTAUSE 禁用和启用某个内核上的中断
  - SIP(Supervisor Interrupt Pending) 查看中断类型
  - SSCAUSE、STVEC、STVAL
- plicinit之后才会实际产生中断
- plicinithart针对每个CPU核心配置所关心的中断
- uart使用了一个循环缓冲区，并且根据 消费者指针 和 生产者指针 的位置判断缓冲区是否已满。如果满了就sleep等待
- 中断和并发
  - device和cpu是并行执行的
    - 生产者/消费者并行
  - 中断暂停了当前的进程
    - 用户态程序能够恢复
    - 内核态程序需要管理中断启用和禁用
  - driver的top/bottom部分可能是并行的
    - 使用lock，确保缓冲区正确更新
- CPU轮询对于高速设备来说实际上更高效，但对于低速设备则不然
