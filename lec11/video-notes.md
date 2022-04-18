
Scheduling Threads

- Thread不是线程，而是指一个执行流 `serial execution`
  - PC计数器、寄存器Reg、栈
  - interleave 
- 对于线程的实现来说，是否共享内存是一个不同之处
  - xv6内核的线程是共享内核的内存
  - 用户态的进程/线程是不共享内存的
  - 现代系统允许单个进程有多个线程，这些是共享内存的
- thread实现切换的方式：timer interrupt
  - pre emptive schedule 抢占式调度
- xv6中thread之间的切换不会直接发生在不同的用户thread之间
  - 先进入内核，内核thread通过schedule切换到不同的内核thread，再回到用户thread
- Linux内每个线程实际上是一个独立的 *进程*，同一个进程的不同线程共享进程的内存空间
