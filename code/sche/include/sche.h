
#ifndef _SCHELIB_H
#define _SCHELIB_H

#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <semaphore.h>
#include "type.h"
#include "comm.h"

typedef VOID (*UniBTSProcEntry_Type)(WORD16, WORD16, VOID *, VOID *);
typedef VOID (*TaskEntryProto)(LPVOID); /* 任务入口原型 */

/*消息队列相关*/
#define VOS_CREATE_QUEUE_FAILURE    (WORD32)0xFFFFFFFF  /* 创建队列失败 */
#define MAX_QUEUE_COUNT             (WORD32)10
#define VOS_QUEUE_CTL_USED          (BYTE)0x01  /* 队列控制块被使用 */
#define VOS_QUEUE_CTL_NO_USED       (BYTE)0x10  /* 队列控制块未被使用 */
#define VOS_QUEUE_ERROR             (SWORD32)0xEFFFFFFF /* 表明出队操作失败 */

#define MSQNAME "/OSS_MQ%d"

#define    SCHE_TASK_NUM              2
#define    TASK_NAME_LEN               60
#define    QUEUE_NAME_LEN              (WORD32)40   /* 队列名字长度 */ 

/* 进程状态标识 */
#define    PROC_STATUS_READY                ((BYTE)1)  /* 进程位于就绪队列 */
#define    PROC_STATUS_RUNNING              ((BYTE)2)  /* 进程正在运行 */
#define    PROC_STATUS_BLOCK                ((BYTE)3)  /* 进程处于阻塞队列 */
/* 进程阻塞等待原因 */
#define    WAIT_MSG                         (0)  /* 无等待 */
#define    WAIT_REMOTECALL                  (1)  /* 远程过程调用等待 */
#define    WAIT_SYNCMSG                     (2)  /* 同步消息等待 */
#define    WAIT_SYNCTIMER                   (3)  /* 同步定时器等待 */

#define EV_STARTUP                              (WORD16)100

#define WEOF               (WORD16)(0xFFFF)

#define SUCCESS      0
#define ERROR_UNKNOWN     1

#define  YES     1
#define  NO      0

#define TRUE      1
#define FALSE    0

#define  S_StartUp     0

#define TASK_NAME_LENGTH        21      /* 任务名长度 */
#define PROC_NAME_LENGTH        30      /* 进程名字符串长度*/
#define INVALID_SYS_TASKID          (WORD32)0   /* 无效的系统任务ID */

#define PROC_TYPE_INDEX_NUM     ((WORD16)0xFFFF)

typedef struct tagT_ProcessName
{
    WORD16  wProcType;               /* 进程类型号 */
    CHAR    acName[PROC_NAME_LENGTH];/* 进程名 */
}T_ProcessName;

typedef struct tagT_TaskConfig
{
    BYTE    ucIsUse;                  /* 任务是否使用 */
    CHAR    acName[TASK_NAME_LENGTH]; /* 任务名 */
    WORD16  wPriority;                /* 任务优先级 */
    WORD32  dwStacksize;              /* 任务栈大小，只能配置成页大小的整数倍 */
    LPVOID  pEntry;                   /* 任务入口函数 */
}T_TaskConfig ;

/* 进程配置表项结构 */
typedef struct tagT_ProcessConfig
{
    BYTE    ucIsUse;                 /* 进程是否使用 */
    WORD16  wProcType;               /* 进程类型号 */
    WORD16  wTno;                    /* 进程所属于的调度任务号 */
    LPVOID  pEntry;                  /* 进程入口函数 */
    WORD32  dwStackSize;             /* 进程栈大小，只能配置成页大小的整数倍 */
    WORD32  dwDataRegionSize;        /* 进程数据区大小，只能配置成页大小的整数倍 */
}T_ProcessConfig;

typedef struct tagT_ScheTCB
{
    BYTE     ucIsUse;
    CHAR     acName[TASK_NAME_LENGTH];
    WORD16   wPriority;
    WORD32  dwStacksize;
    void (*pEntry)(WORD32);
    WORD32   dwMailBoxId;
    ULONG   dwTaskId;

    WORD16  wBlockHead;             /* 任务中阻塞进程队列头索引 */
    WORD16  wReadyHead;             /* 任务中就绪进程队列头索引 */
    WORD16  wRunning;               /* 任务正在运行的进程的索引 */

    WORD16  wReadyCounts;           /* 本任务中的ready进程数 */
    WORD16  wBlockCounts;           /* 本任务中的block进程数 */

    WORD32  dwScheCounts;           /* 任务被调度次数 */    
    WORD16  wProcTypeCounts;        /* 本任务中的进程类型数 */

    BYTE	bEnterProc; 			/*是否开始调度进程*/
}T_ScheTCB;

typedef struct tagT_PCB
{
    BYTE        ucIsUse;            /* 进程是否使用 */
    BYTE        ucRunStatus;        /* Ready(位于就绪队列),Block(位于阻塞队列),Run*/
    WORD16      wTno;               /* 进程所在的任务标识 */

    /*阻塞/运行链表,根据这个PCB状态而定*/
    WORD16      wPrevByStatus;   /*前一个运行或阻塞的PCB*/
    WORD16      wNextByStatus;   /*后一个运行或阻塞的PCB*/

    WORD16      wState;             /* 进程的业务状态 */
    WORD16      wRetState;             /* 进程的返回状态 */
    WORD16      wInstanceNo;        /*进程实例号 */
    WORD16      wProcType;          /* 进程类型 */

    VOID     (*pEntry)(void);        
    ULONG      dwProcSP;           /* 进程堆栈顶指针 */
    ULONG      dwTaskSP;           /* 任务堆栈顶指针 */
    BYTE        *pucStack;          /* 进程堆栈底指针 */
    WORD32      dwStackSize ;       /* 堆栈大小 */
    
    BYTE        *pucDataRegion;     /* 私有数据区指针 */
    WORD32      dwDataRegionSize;

    BYTE        ucBlockReason;      /* PCB阻塞等待原因 */
    WORD32      dwScheCounts;           /* 距上次进程频度统计时的调度次数 */
    
    T_MSG       *ptCurMsg;   /* 当前消息 */
    WORD32      dwMsgQCounts;            /* 进程消息队列中的消息数 */
    /* 进程消息队列指针 */
    T_MSG       *ptPrevMsg;         /* 进程当前消息的前一个消息指针 */
    T_MSG       *ptFirstMsg ;       /* 进程消息队列的第一条消息 */
    T_MSG       *ptLastMsg ;        /* 进程消息队列的最后一条消息 */
}T_PCB;

typedef struct tQCtlBlk
{
    WORD32      dwTotalCount;       /* 队列元素总个数*/
    WORD32      dwEntrySize;        /* 队列元素尺寸 */

    WORD32      dwUsedCount;        /* 现有的队列元素个数 */
    WORD32      dwQueueId;        /* VxWorks队列的任务ID */

    BYTE        ucUsed;             /* 使用标志 */

    CHAR        aucQueueName[QUEUE_NAME_LEN]; /* 队列名字 */

    WORD16      wLostMsgQFull;     /*记录队列满丢弃的消息数*/ 
    CHAR        pad[2];            /*对齐填充字节*/	
    WORD32      dwUsedMaxNum;
}T_QueueCtl;

typedef  struct ThreadInfo_tag
{
    CHAR            flag;
    CHAR            pri;
    CHAR            name[TASK_NAME_LEN];
    TaskEntryProto  pFun;
    VOID            *arg;
    WORD32      thread_id;
    WORD32          thread_tid; 
    INT             stacksize;      /* 线程堆栈大小 */
    WORD32          stackguardsize; /* 线程堆栈保护页大小 */
    ULONG         stackaddr;      /* 线程堆栈低地址起始处 */
    WORD32          tSpareInfo;
    BYTE            *pbySigAltStack;/* 线程信号处理函数专用栈*/
}T_ThreadInfo;

typedef struct tagT_CoreData                    
{
    WORD32          dwCoreDataSize;               /* 进程级:核心数据区的大小 */

    WORD16          wPCBCounts;                   /* 低持械腜CB资源数量 */
    T_PCB           *ptPCB;                       /* 系统的第一个PCB块指针 */

    WORD32          dwProcNameCounts;             /* 进程级:全局进程名个数 */
    T_ProcessName   *pProcNameTable;              /* 进程级:全局进程名表 */

    T_ScheTCB       atScheTCB[SCHE_TASK_NUM];     /* 进程级:调度任务TCB数组 */

    WORD32          dwSelfPCBIndex;               /* 进程级:当前运行的进程的索引 */
    
    BYTE            pIdx;  /* 当前进程在进程索引表中的位置 */
}T_CoreData;


extern __thread T_PCB * gptSelfPCBInfo;
extern VOID ScheTaskEntry(int dwTno);
extern void NormalReturnToTask(T_PCB *ptPCB);


#endif
