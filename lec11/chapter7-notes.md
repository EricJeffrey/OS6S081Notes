
7 Scheduling

- 使用 multiplexing 技术实现进程时间片

7.1 Multiplexing

- xv6切换CPU的两种情况
  1. 当程序 等待IO、等待子进程退出、调用sleep系统调用时，通过sleep wakeup机制切换进程
  2. xv6每隔一段时间就强制切换进程
- multiplexing的挑战
  1. 如何从一个进程切换到另一个进程 - 上下文切换
  2. 如何使**强制切换**对用户代码来说是透明的
  3. 所有CPU共享相同的进程集合，需要一定的加锁机制
  4. 进程的内存等资源需要回收，但进程自身无法回收所有资源（例如进程不能在内核栈上执行的同时释放内核栈）
  5. 每个CPU核心都需要记录它在执行的进程
  6. 需要仔细设计 sleep 和 wakeup，确保wakeup不会丢失

7.2 Code: Context switching

- xv6的 scheduler 在每个CPU上都有一个专用的执行线程（保存的寄存器和栈）
  - 在进程的内核栈上执行scheduler不安全，如果其它CPU唤醒了当前进程，那就有两个CPU核心**同时**在一个栈上运行，不安全
- 进程切换的时候会保存stack pointer和PC，因此会切换执行的代码和栈
- `swtch(kernel/swtch.S)`仅保存RISC-V的一些寄存器，称之为 上下文context
  - `swtch(struct context* old, struct context *new)`将当前寄存器保存在`old`中，并将`new`中的寄存器恢复
- `yield(kernel/proc.c)` -> `sched` -> `swtch`让出CPU
  - 会将当前的寄存器保存在当前进程的`p->context`中，并将`mycpu()->context`加载出来
- `swtch`不保存程序计数器，**只保存返回地址寄存器**`ra`
  - 通过`ra`和`sp`寄存器，`swtch`返回时能够回到 `new` 上下文对应的指令位置（new 线程先前调用swtch时的位置），以及 `new` 的执行栈上
  - yield中的`swtch(p->context, mycpu()->context)`会返回到scheduler中，也就是CPU专用的 scheduler 栈上

7.3 Code: Scheduling

- `scheduler` 在`main.c`中首次被调用，之后不断地寻找一个可以运行的进程，然后运行那个进程
  - `scheduler`中的`swtch(&c->context, &p->context)`会将CPU的执行流转移到`p->context`对应的位置
  - 只有在上述进程`p`让出CPU时（时钟中断、主动yield、exit...），`scheduler`的执行流才会恢复
- `sched`之前需要获得p的锁，但是`scheduler`中也`acquire`了进程的锁，为何不会死锁？
  - 当前进程p，一定是由`scheduler`获得锁然后`swtch`才开始执行的，因此切换到CPU调度器中是，会回到`scheduler`中swtch的位置，也就是进程p执行后的位置

- `sched`和`scheduler`类似于协程，sched总是回到`scheduler`中，后者在暂停的位置继续执行；而后者在调度一个进程后，又会因为yield、exit等回到`sched`中

- 进程在让出CPU之前，必须先 **获取自己的锁**，并**释放其它锁**
  - `sched`会检查，同时确保**中断是禁用的**（参考上一节，拥有锁的时候应当禁用中断，否则可能死锁）
  - `sched`调用`swatch`保存当前进程p的上下文，然后 *回到* `scheduler`上一次调度的位置
- 锁的控制权会从 `sched` 传递到切换后的代码中
  - 必要性：p->lock 保护了进程`p->state`和`p->contex`，在`swtch`切换上下文时的 **不变式invariants**
  - 不变式invariants
    - 如果 p->state == RUNNING: CPU寄存器必须保存进程的寄存器，`c->proc`指向这个进程
    - 如果 p->state == RUNNABLE: `p->context`必须保存进程的寄存器，没有任何CPU在进程的内核栈上执行，没有任何CPU的`c->proc`指向这个进程
  - 上述两个invariants在`sched`和`scheduler`可能会被打破，因此需要加锁
- 例外情况，对于使用fork新创建的进程
  - `allocproc`会将 返回地址寄存器`ra`设置为 `forkret`，因此`scheduler`在第一次使用`swtch`时会“返回到” `forkret`，用来释放进程的锁
  - 也可以将`ra`设置为`usertrapret`保证进程能够回到用户空间

- scheduler的一个示意图
```

             循环调度进程
         ---------------------
         ↓                    ↑
main.c------> schduler.... ---^
              ↓   ↑----------<------------- 
              ↓                           ↑ back to where scheduler PAUSE
              ↓ runnable found            ↑
              ↓ PAUSE, start p          sched/swtch
            swtch                         ↑
              ↓                           ↑
              ↓                           ↑ exit/yield
              process ----->--------------^
 
```

7.4 Code: mycpu and myproc

- xv6确保在内核态的时候，`tp`寄存器保存CPU核心hartid
  - `mstart`在machine mode设置tp
  - `usertrapret`将tp保存到trampoline中
  - 从用户态进入内核态时，`uservec`加载trampoline的tp
  - 编译器保证从不使用tp
- `cpuid`和`mycpu`获取当前cpu的id和进程信息
  - 需要禁用中断，否则进程可能被调度到另一个CPU上，导致获得错误的hartid
- `myproc`禁用中断并调用`mycpu`获取进程信息，然后启用中断
  - 获得的proc信息不会受中断影响：即便调度了另一个CPU上，proc结构仍然是同一个

7.5 Sleep and wakeup

- PV原语实现
  - 忙等 busy waiting 浪费CPU
  - 直接使用 `while (cnt == 0) sleep(semaphore)` 可能会出现 `wake lost`
  - 需要使用 `acquire(lock); while (cnt == 0) sleep(semaphore, lock)` 的方式，

7.6 Code: Sleep and wakeup

- sleep获取**进程锁**和传入的**PV锁**，标记进程为SLEEPING，之后**释放PV锁**，然后使用`sched`让出进程，`sched`会释放掉进程的锁
- wakeup获取这两个锁，标记进程为RUNNABLE
- 如何保证不会丢失wakeup
  - 睡眠中的进程在被标记为睡眠之前，要么拥有 PV锁，要么拥有 进程锁，或者两者都有
  - wakeup在唤醒时，必须获取 进程锁
  - 生产者可以保证，在消费者检查条件前完成对条件的修改，在消费者睡眠之后才唤醒睡眠中的进程
- 对于多个等待的pipe数据的进程，可能出现 虚假的spurious 唤醒
  - 唤醒了但是数据被另一个先wake的进程读完了
  - 因此sleep总是在一个`while`循环中调用
- `sleep/wakeup`的神奇之处
  - 轻量（不用特殊数据结构和忙等）
  - 提供了一个中间抽象层，调用者无需知道细节

7.7 Pipe

- 循环队列，读指针+写指针
- 如果写满了就sleep当前进程然后wakeup读的进程

7.8 Code: Wait, exit, and kill

- parent进程退出的时候使用 reparent 将子进程交个init
- `kernel/proc.c:wait` 使用全局的锁 wait_lock 保证子进程的wakeup不会丢失
  - 为避免死锁，需要先获取wait_lock然后p->lock
- exit既获取wait_lock也获取p->lock，前者为了保证wakeup不丢失，后者保证进程的状态正确改变
  - 先wakeup再设置状态是可以的，因为锁还未释放
- kill将`p->killed`设置为1，如果进程在slepp就唤醒它
  - 进程可能在执行系统调用，因此不能直接结束它
  - p->killed在usertrap或中断的时候会检查，所以用户进程或者在退出内核态的时候结束，或者在时钟中断/设备中断的时候结束
- xv6有些sleep的调用并不会检查p->killed，这是因为这些代码需要在多个阶段的系统调用中保持原子性
  - 磁盘IO只有在完成当前的系统调用后，再usertrapret时才会发现p->killed

7.9 Process Locking

- p->lock可以认为是，在读写 `state chan killed xstate pid` 时必须获取的锁
- 列举了p->lock保护的操作/不变式
- p->parent 由全局锁 wait_lock 保护，只有进程的parent才会修改进程的parent
- wait_lock 序列化了parent和child 对exit的调用，保证子进程的wakeup不会丢失
- wait_lock为什么是一个全局锁？因为只有在进程获取它了之后，进程才知道其parent进程是哪个（不会被reparent）

7.10 Real world

- xv6的调度 round robin
- 真实系统可以由优先级priority，但会带来其它问题
  - 优先级反转priority inversion：两个不同优先级进程用同一个锁，低优先级的先获取到锁了，违反了优先级的策略
  - 护航肖莹convoy：耗时较少的潜在资源消费者被排在重量级的资源消费者之后，而后者的优先级可能并不高
- Linux内核使用明确的进程等待队列
- 惊群：所有在等待的进程都会唤醒，race竞争资源
  - 多数条件变量会使用两种原语：唤醒一个和唤醒所有
- 信号量semaphore的计数可以用来记录wakeup的数量，避免 假唤醒 和 惊群效应
- xv6没有实现信号，信号可以唤醒sleep的进程
- xv6的kill不够完善，有的sleep循环可能需要检查`p->killed`
- 真是操作系统也应该使用 free 队列记录可用的进程块

