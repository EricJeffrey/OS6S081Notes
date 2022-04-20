
- xv6的规则
  - 进程在调用swtch时必须持有p->lock
  - 进程在调用swtch时不能持有**其它任何锁**
- 进程提出的时候仅仅将p->state设置为 zombie ，之后使用sched让出CPU
  - 进程的结构尚未被销毁，无法被其它的fork重用