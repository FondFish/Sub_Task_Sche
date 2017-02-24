  程序说明：主要完成linux“进程-任务-子任务”二级调度示例，其中用户级“子任务”调度使用消息驱动方式调度，已完成。
目前的子任务调度方式为轮询调度。现已改为消息驱动。
   编译：gcc sche.h config.h sche.c config.c main.c -Wl,-lpthread -lrt  -o test
已使用makefile进行编译生成。或者执行build下的脚本，需要安装cmake，exe会生成在output目录。
   运行示例：
hp@hp:~/SCHE$ ./test 
create mq:SCH0 SUCC,QueID:3
task:SCH0 create SUCC,thread id:0xb0b12700
create mq:SCH1 SUCC,QueID:4
task:SCH1 create SUCC,thread id:0xb0311700

 [ScheTaskEntry:304] Tno = 1 Start  Sche PCB

 ScheTaskEntry Tno = 1,cnt=1 

 [ScheTaskEntry:304] Tno = 0 Start  Sche PCB

 ScheTaskEntry Tno = 0,cnt=1 
test3 Entry cnt=1
test3 Recv EV_STARTUP
test1 Entry cnt=1
test1 Recv EV_STARTUP
Send Msg=1003 to Pno=0x3000 Succ

 ScheTaskEntry Tno = 1,cnt=2 
test4 Entry cnt=1
test4 Recv EV_STARTUP

 ScheTaskEntry Tno = 0,cnt=2 
test2 Entry cnt=1
test2 Recv EV_STARTUP
Send Msg=2004 to Pno=0x4000 Succ
ready queue is empty

 ScheTaskEntry Tno = 1,cnt=3 
test3 Entry cnt=2
Pno=0x3000 Recv msg:1003
Send Msg=3002 to Pno=0x2000 Succ
ready queue is empty

 ScheTaskEntry Tno = 0,cnt=3 
test2 Entry cnt=2
Pno=0x2000 Recv msg:3002
Send Msg=2004 to Pno=0x4000 Succ
main running
ready queue is empty

 ScheTaskEntry Tno = 1,cnt=4 
test4 Entry cnt=2
Pno=0x4000 Recv msg:2004
Send Msg=4001 to Pno=0x1000 Succ
ready queue is empty

 ScheTaskEntry Tno = 0,cnt=4 
test1 Entry cnt=2
Pno=0x1000 Recv msg:4001
Send Msg=1003 to Pno=0x3000 Succ
ready queue is empty

 ScheTaskEntry Tno = 1,cnt=5 
test4 Entry cnt=3
Pno=0x4000 Recv msg:2004
Send Msg=4001 to Pno=0x1000 Succ
ready queue is empty

 ScheTaskEntry Tno = 0,cnt=5 
test1 Entry cnt=3
Pno=0x1000 Recv msg:4001
Send Msg=1003 to Pno=0x3000 Succ
ready queue is empty

 ScheTaskEntry Tno = 1,cnt=6 
test3 Entry cnt=3
Pno=0x3000 Recv msg:1003
Send Msg=3002 to Pno=0x2000 Succ
ready queue is empty

 ScheTaskEntry Tno = 0,cnt=6 
test2 Entry cnt=3
Pno=0x2000 Recv msg:3002
Send Msg=2004 to Pno=0x4000 Succ
main running
ready queue is empty

 ScheTaskEntry Tno = 1,cnt=7 
test3 Entry cnt=4
Pno=0x3000 Recv msg:1003
Send Msg=3002 to Pno=0x2000 Succ
ready queue is empty

 ScheTaskEntry Tno = 0,cnt=7 
test2 Entry cnt=4
Pno=0x2000 Recv msg:3002
Send Msg=2004 to Pno=0x4000 Succ
ready queue is empty

 ScheTaskEntry Tno = 1,cnt=8 
test4 Entry cnt=4
Pno=0x4000 Recv msg:2004
Send Msg=4001 to Pno=0x1000 Succ
ready queue is empty

 ScheTaskEntry Tno = 0,cnt=8 
test1 Entry cnt=4
Pno=0x1000 Recv msg:4001
Send Msg=1003 to Pno=0x3000 Succ
ready queue is empty

 ScheTaskEntry Tno = 1,cnt=9 
test4 Entry cnt=5
Pno=0x4000 Recv msg:2004
Send Msg=4001 to Pno=0x1000 Succ
ready queue is empty

 ScheTaskEntry Tno = 0,cnt=9 
test1 Entry cnt=5
Pno=0x1000 Recv msg:4001
Send Msg=1003 to Pno=0x3000 Succ
main running
ready queue is empty

 ScheTaskEntry Tno = 1,cnt=10 
test3 Entry cnt=5
Pno=0x3000 Recv msg:1003
Send Msg=3002 to Pno=0x2000 Succ
ready queue is empty

 ScheTaskEntry Tno = 0,cnt=10 
test2 Entry cnt=5
Pno=0x2000 Recv msg:3002
Send Msg=2004 to Pno=0x4000 Succ
ready queue is empty

 ScheTaskEntry Tno = 1,cnt=11 
test3 Entry cnt=6
Pno=0x3000 Recv msg:1003
Send Msg=3002 to Pno=0x2000 Succ
ready queue is empty

 ScheTaskEntry Tno = 0,cnt=11 
test2 Entry cnt=6
Pno=0x2000 Recv msg:3002
Send Msg=2004 to Pno=0x4000 Succ
ready queue is empty

 ScheTaskEntry Tno = 1,cnt=12 
test4 Entry cnt=6
Pno=0x4000 Recv msg:2004
Send Msg=4001 to Pno=0x1000 Succ
ready queue is empty

