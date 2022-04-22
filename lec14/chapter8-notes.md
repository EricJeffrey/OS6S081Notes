
8 File Systems

- 文件系统用来共享数据，需要持久化保存数据
- 文件系统的挑战
  - 磁盘上的数据结构设计：文件/目录的树形结构，记录哪些块可用
  - 崩溃回复crash recovery：断电重启能够保持正常工作
  - 多个处理器并发访问，需要协调以保证正确性invariants
  - 需要对常用数据进行缓存


8.1 Overview

- 七层
  - Disk层 操作硬件
  - Buffer cache层 同步不同进程的操作，并且确保某个时刻只会有一个内核进程修改数据
  - log层 将多个block的操作合并为 事务transaction，确保在crash的时候block更新的原子性atomically
  - inode层 将每个文件使用inode数据结构表示，每个inode有个唯一的整数id
  - directory层 是一种特殊的inode，表示文件夹这种数据结果
  - pathname层 提供了层次化的路径名
  - file descriptor层对资源进行抽象
- 磁盘硬件通常由一系列的扇区sector组成，每个sector大小为512Byte
- 操作系统的block通常更大一点，一般是 sector 大小的整数倍
- 文件系统在磁盘上的结构，如下图所示
  - 第0个block存储boot信息，不适用
  - 第1个block为 超级块superblock，存储了元信息（block的大小、数量、inode数量、log中block数量等）
  - 第2个block开始，为日志log信息，由多个block组成
  - 之后为inode，每个block有多个inode
  - 之后为bitmap，记录了每个数据block是否在使用
  - 最后为数据block
![](struct_of_fs.png)

8.2 Buffer cache layer

- buffer cache的工作
  - 同步磁盘访问操作
  - 缓存常用的磁盘块
- 每个buffer有一个对应的sleep-lock，确保每次只会有一个thread使用
- 每次执行完操作后需要使用brelse释放锁
- buffer不够的时候会使用LRU算法，回收一个块

8.3 Code: Buffer cache

- buffer cache 是一个双向链表
- 每个buffer有一个disk标记为，表示当前内容是否已经从磁盘中读取进来，或者已经写入磁盘
  - virtio_disk_rw 会修改disk属性
- bget的时候使用`b->valid=0`确保buf的内容会重新从磁盘读取
- 磁盘上每个sector对应的buffer只能有**最多一个**
- `b->refcnt`不为0的时候，该buf不会被重用
- `bcache.lock`保护了哪个磁盘块被缓存的信息，`b->lock`保护了对应的缓存buf
- 修改buf后，调用者必须使用`bwrite`将更改写到磁盘上，并且在使用完后使用`brelse`释放这个快
- `brelse`会将块放到双向链表的最前端
  - `b->next` 记录了most recently used
  - `b->prev` 记录了least recently used

问题 
- 什么情况下bget会出现 panic(no buffer)？所有buf都busy的时候，更好的方式是 *等待一个buf可用*

8.4 Logging layer

- crash恢复：许多文件系统的操作涉及多次磁盘写，因此需要确保磁盘突然断电重启/故障后能真保持正常状态
- xv6使用日志logging解决这个问题
  - 写操作不是直接操作磁盘，而是记录到logging中
  - 一个syscall的所有写操作完成后，需要发起一个 **commit** 操作，将所有内容写到磁盘上
  - 如果系统故障，则根据logging中的内容恢复
    - 若logging完整，则按照记录重放写操作
    - 若不完整则忽略
  - 从而保证磁盘处于正确的状态

8.5 Log design

- 超级快中包含了logging的位置
- log的header首部包含：扇区数组和log block的数量
  - log block的数量要么是0，要么非0，表示有一个完整的commit
- xv6在将数据写会到磁盘后将log block的数量设置为0
- 每个系统调用都指明需要保持原子性的一系列write操作
- logging只会在没有对文件系统进行操作的系统调用时进行commit
- logging会聚合多个事务，提高效率
- xv6使用固定大小来存储日志
  - 系统调用的写操作不能超过日志的大小
    - write可能超过：xv6将超过日志大小的写操作分成多个小操作
    - unlink可能需要更改许多bitmanp和inode：xv6实际上只会用到一个bitmap块
  - 只在所需写入的数据量不超过logging中剩余数量时，logging才会允许这个system call进行写入，否则需要等待
