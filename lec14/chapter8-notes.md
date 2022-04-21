
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


