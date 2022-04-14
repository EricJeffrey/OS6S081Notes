
Chapter 6 Locking

- execution可能interleave
  - 多个CPU独立执行
  - 同一个CPU多个线程
  - 中断处理函数和被中断的代码
- 并发控制：互斥锁


6.1 Race Conditions

- kfree里面往list里面添加内存块可能会有竞争条件
- lock锁保证了 **互斥**
- 需要被锁保护的成为 **临界区**
- 锁将执行 **序列化** 了，因此会降低性能
- 多个进程要求同一个锁称之为 **冲突conflict**
- 更加复杂/现代的内核会使用复杂的设计方式避免 冲突
  - 每个CPU维护一个freelist，只有在当前CPU的freelist空了的时候才找其它CPU要内存

6.2 Code: Locks

- memory barrier 内存屏障：保证内存屏障前/后的指令不会被重排到屏障后/前
- [__syncBuiltins](https://gcc.gnu.org/onlinedocs/gcc/_005f_005fsync-Builtins.html)介绍了`kernel/spinlock.c`中用到的几个同步指令
  - `__sync_synchronize (...)` 创建一个内存屏障
  - `__sync_lock_test_and_set (type *ptr, type value, ...)`：将 value 写入到 `*ptr`，并返回 `*ptr` 先前的内容
  - `void __sync_lock_release (type *ptr, ...)`：将0写入到 `*ptr` 中
  - `while(__sync_lock_test_and_set(&lk->locked, 1) != 0) ;` 与 `__sync_lock_release(&lk->locked)` 配合通过CAS实现了获取锁和释放锁
- spinlock的acquire实现
  - 使用 `push_off` 关闭中断，并且将 `noff` 值加1，记录多少个CPU执行了push_off
  - 通过`__sync_lock_test_and_set`的原子操作，不断spin尝试给locked加锁（写入1），通过其旧值是否为0判断其它CPU是否已经释放锁
  - 如果 旧值 为0，说明没有CPU在用锁，则获得锁；如果 旧值 为1，说明有CPU已经获得锁，那么这个操作也不会影响locked的值（仍然是1）
  - 同时使用内存屏障，保证临界区的指令不会被重排到获得锁之前
- spinlock的release实现
  - 使用 `__sync_lock_release` 将locked的值设为0，释放锁
  - 使用内存屏障保证指令不会被重排到释放锁之后
  - 使用 `pop_off` 将 `noff` 减1，根据是否为0还原中断状态

6.3 Code: Using locks

- 如何决定用多少锁，那些数据应当被锁保护
  - 锁应当用来避免两个CPU对同一个数据至少一个write的操作overlap
  - 如果一个锁需要保护多个内存区域，那所有这些内存区域都应当加锁保护
- `kalloc.c`中是一个粗粒度的锁，对整个freelist加锁
- xv6对每个文件都加锁
  - 更细粒度可以对每个文件的部分区域加锁

6.4 Deadlock and lock ordering

- 如果一个代码路径需要获取多个锁，那么所有的代码路径都应当以**同样的次序**获得这些锁，否则可能产生deadlock
- 锁在某种程度上来说是每个函数声明的一部分
- 有时候可能需要先获得一个锁，然后再确定下一个要获取的锁

6.5 可重入锁

- 可重入锁：如果进程已经获取了某个锁，那如果它可以再次使用acquire获取它而不是panic
- 可重入增加了推断并发过程的难度

6.6 Locks and interrupt handlers

- 如果中断处理函数使用了一个锁，那CPU绝对不能在启用中断的情况下hold那个锁
- xv6：如果CPU获取了某个锁，那么它总是禁用这个CPU上的终端
- 当前CPU上没有spinlock的时候才启用中断：使用 push_off pop_off 记录加锁的level数
- 对于acquire来说，`push_off` 必须出现在 `lk->locked` 之前，否则就会出现一个临界区（获得了锁但是中断没关）

6.7 指令重排

- CPU和编译器可能重排指令：对于顺序执行的代码结果一样，但是对于并发程序结果可能错误
- 内存模型 memory model 来管理这些
  - `__sync_synchronize()`实现内存屏障，屏障之前的指令不会到后面，后面的不会被重排到前面

6.8 Sleep locks

- 对于需要长时间获得锁的任务，spinlock的缺点
  - 浪费CPU时间（不停spin）
  - 无法在保持锁的状态下 让出yield CPU
- xv6提供了 `sleep-locks(kernel/sleeplock.c)` 实现
  - acquiresleep 使用sleep来原子地让出CPU并释放spinlock
- spinlock适合短的临界区
- sleeplock适合执行流较长的任务

6.9 Real world

- 最好将锁隐藏在更上层的数据结构中，如 synchronized queue
- POSIX threads提供了线程和同步的功能，但是实现这些功能需要kernel的支持
- 没有原子操作也能实现锁，但更加复杂且更expensive
- 许多操作系统使用了 lock-free 的数据结构和算法，但更加复杂
