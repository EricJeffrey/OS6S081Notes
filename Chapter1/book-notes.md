第一章 操作系统接口

1.1 进程和内存

- 系统调用
  - fork: 调用一次，返回两次（父进程childpid/子进程0）
  - exec: 使用其它程序替换当前内存空间，包括数据与指令
  - wait: 等待子进程退出
  - exit: 退出
- malloc使用sbrk扩张内存
- fork的COW机制: 写时复制(Copy-On-Write)

1.2 I/O和文件描述符

- I/O系统调用
  - open: 返回文件描述符
  - read
  - write
  - close
  - dup: 拷贝文件描述符
- 文件描述符
  - 文件file
  - 设备device
- 通过dup实现I/O重定向

1.3 管道pipe

- 父子进程双向通信，系统调用
  - pipe
- 管道实现shell的管道（第一个程序的输出作为第二个程序的输入

1.4 文件系统fs

- 目录结构/a/b/c
  - 绝对路径/相对路径
  - work directory
  - cd更改shell工作目录
  - inode与link
- 系统调用
  - chdir
  - mkdir
  - mknod
  - fstat -> `struct stat {device, inode-number, type, number-of-links, size-in-bytes};`
  - link/unlink
  - rm

1.5 真实世界Real World

- Unix系统调用接口标准化工作: Portable Operating System Interface (POSIX)
