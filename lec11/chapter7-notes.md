
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
