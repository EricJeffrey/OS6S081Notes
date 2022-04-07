
### 实验笔记

1. 大端 big-endian 小端 little-endian
   1. 表示最高位在大端还是小端
   2. 大端的话，最高字节在开头
   3. 小端的话，最低字节在开头
   4. 比如 i=0x00646c72，实际上是 i = `0dlr`，如果是大端，则是`0`在开头；如果是小端，则`r`在开头
2. lab sigalarm
   1. sigalarm实现
      1. proc需要保存 `tick_since_last_call`, `alarm_interval`, `handler`
      2. timer每个tick会中断，需要在`usertrap`里面处理中断 `which_dev==2`，并在tick达到interval的时候触发handler
         1. `alarm_interval`为0的话不需要记录 ticks
         2. 触发时要重设`tick_since_last_call`
         3. 如何触发handler - usertrapret使用sret返回，sepc保存地址
         4. 被中断执行流程怎么恢复 - `sigreturn` + `alarmframe`保存所有寄存器
      3. `alarm_interval`需要初始化


##### lab sigalarm/sigreturn implementation

1. 设置程序的定时器`alarm_contex`
   1. proc结构保存`alarm_contex`
      1. `tick_interval` 处理器tick的间隔
      2. `handler` 间隔后触发的handler
      3. `tick_last_call` 上次触发handler后经历的tick
      4. `alarming` 是否在执行handler
      5. `alarmframe` 保存`trapframe`
   2. 如果 tick_interval 为0表示不设置handler
   3. handler 为函数地址
   4. `alarmframe`是指针还是对象 - 是否需要使用`kalloc`分配，还是直接在栈上
2. 程序调用`sigalarm(tick_interval, handler)`
   1. handler值可能为0（地址可能从0开始），所以仅使用`tick_interval`判断
   2. 如果`tick_interval == 0`
   3. 真：将两者设置为0
   4. 假：将两者设置为对应的值
3. 每个CPU的tick时触发时钟中断 -> usertrap中 `which_dev == 2`
   1. 检查当前进程的 `tick_interval == 0`？
   2. 真：什么都不干，跳转到 yield
   3. 假：继续
   4. 检查当前进程是否在执行handler，即 `alarming == 1`？
   5. 真：什么都不干，跳转到 yield
   6. 假：继续
   7. `tick_last_call+=1`
   8. 检查 `tick_last_call == tick_interval`？
   9.  假：什么都不干，跳转到 yield
   10. 真：继续
   11. 设置 `alarming = 1`
   12. 拷贝 `p->trapframe` 到 `p->alarmframe`
   13. 设置 `p->trapframe->epc = handler`
   14. 执行 `intr_on()` 打开中断
   15. 执行 `usertrapret` -> PC开始执行handler...
   16. `yield`: 调用`yield`让出CPU...
4. 程序调用了`sigreturn`
   1. 检查 `alarming == 1`
   2. 假：跳转到 nothing
   3. 真：继续
   4. 设置 `alarming = 0; tick_last_call = 0;`
   5. 拷贝 `alarmframe` 到 `trapframe`
   6. 清空 `alarmframe`
   7. 调用 `usertrapret` -> PC回到上次中断的地方
   8. `nothing`：或者什么的不干，或者panic？
