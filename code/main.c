#include "stdio.h"
#include "sche.h"
#include <sys/time.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <mqueue.h>
#include <string.h>
#include <sys/prctl.h>
#include "config.h"

pthread_mutex_t        s_mutex = PTHREAD_MUTEX_INITIALIZER;
extern  VOID InitTCBPool(VOID);

WORD32 Vos_CreateQueue(CHAR *pucQueueName,WORD32  dwEntryCount,WORD32  dwEntrySize)
{
    WORD32 dwQueCtlLoop   = 0;
    int dwQueueCtrlId      = VOS_CREATE_QUEUE_FAILURE;/* 标识找到的可用队列ID */
    T_QueueCtl *pQueueCtl = NULL;
    WORD32      dwQueueId     = 0;
    struct      mq_attr attr;

    dwEntryCount += 1; /* 补足队列长度 */
    
    pthread_mutex_lock(&s_mutex);
    
    /* 查找空闲队列控制块 */
    for (dwQueCtlLoop = 0; dwQueCtlLoop <MAX_QUEUE_COUNT; dwQueCtlLoop++)
    {
        if (s_atQueueCtl[dwQueCtlLoop].ucUsed == VOS_QUEUE_CTL_NO_USED)
        {
            /* 找到可用的队列控制块 */
            dwQueueCtrlId = dwQueCtlLoop;
            break;
        }
    }
    
    if (dwQueCtlLoop<MAX_QUEUE_COUNT)
        pQueueCtl = &s_atQueueCtl[dwQueCtlLoop];

    pQueueCtl->dwEntrySize = dwEntrySize;
    /* 假设队列长度为100个元素，实际只能存放99个元素 */
    pQueueCtl->dwTotalCount = dwEntryCount - 1;
    pQueueCtl->dwUsedCount = 0;
    pQueueCtl->ucUsed = VOS_QUEUE_CTL_USED;
    
    pthread_mutex_unlock(&s_mutex);

   /* 保证唯一性,避免打开已经打开的队列 */
    sprintf((CHAR *)(pQueueCtl->aucQueueName), MSQNAME, dwQueueCtrlId);

    /* 清除消息队列，防止使用旧的消息队列 */
    mq_unlink((CHAR*)(pQueueCtl->aucQueueName));

    memset(&attr, 0, sizeof(attr));
    attr.mq_maxmsg  = dwEntryCount;
    attr.mq_msgsize = dwEntrySize;

    dwQueueId = mq_open((CHAR*)(pQueueCtl->aucQueueName),O_RDWR | O_CREAT,0666,&attr);

    if (((WORD32)-1) == dwQueueId)
    {
        printf("[%s:%d]Fail to create message queue of Linux, error:%d:%s\n", __FUNCTION__, __LINE__, errno,strerror(errno));
        return VOS_CREATE_QUEUE_FAILURE;
    }
    printf("create mq:%s SUCC,QueID:%d\n",pucQueueName,dwQueueId);
    
    pQueueCtl->dwQueueId = dwQueueId;
    
    return dwQueueCtrlId;
}
void R_UniThreadCleanup()
{
    /*待实现*/
    return;
}
CHAR *getThreadNameByID(WORD32  threadid)
{
    int i;

    for(i=0; i<SCHE_TASK_NUM; i++)
    {
        if((g_threadInfo[i].thread_id==threadid)&&(g_threadInfo[i].flag!=0))
        {
            return g_threadInfo[i].name;
        }
    }
    return "Unknow thread";
}
VOID  Vos_UniThreadEntry(INT idx)
{
    TaskEntryProto      pFun;
    VOID                *arg;
    pthread_attr_t      attr;
    VOID                *pStackBase = NULL;
    size_t              dwStackSize = 0;
    WORD32              dwGuardSize = 0;
    INT                 dOldCancelType;

    g_threadInfo[idx].thread_id  = pthread_self();

    /* 登记本线程的堆栈信息 */
    pthread_getattr_np(g_threadInfo[idx].thread_id, &attr);   
    pthread_attr_getstack(&attr,&pStackBase,(size_t *)&dwStackSize);
    pthread_attr_getguardsize(&attr, (size_t *)&dwGuardSize);             


    /* 现在由pStackBase，dwTaskStackSize，dwGuardSize三个变量所勾画的任务堆栈布局：

          低地址 ---------------------<--------堆栈增长方向------------------------------<- 高地址   

               |<------------------------dwTaskStackSize-------------------------------->| 
    -----------|----------------------|--------------------------------------------------|-------------
               | 保护页(dwGuardSize)  |                实际可用的堆栈                    |
    -----------|----------------------|--------------------------------------------------|-------------
               ^ 
          pStackBase                          
    */
    g_threadInfo[idx].stackaddr = (ULONG)pStackBase;
    g_threadInfo[idx].stackguardsize = dwGuardSize;
    g_threadInfo[idx].thread_tid = gettid();

    pFun    = g_threadInfo[idx].pFun;
    arg     = g_threadInfo[idx].arg;

    /*set thread name*/ 
    prctl(PR_SET_NAME,getThreadNameByID(g_threadInfo[idx].thread_id),0,0,0);
    /*自动释放资源*/
    pthread_cleanup_push(R_UniThreadCleanup,g_threadInfo + idx);
    /*修改线程取消类型之前，先保存原始取消类型(POSIX标准中为PTHREAD_CANCEL_DEFERRED)*/
    if (0 != pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &dOldCancelType))
    {
        printf("[OSS:%s:%d]setcanceltype failed: %s [so you can't stop me]\n", __FUNCTION__, __LINE__, strerror (errno));
        return;
    } 

    (*pFun)((VOID*)arg);

    /*pthread_cleanup_pop会首先取消pthread_cleanup_push注册的cancel展开点，
      为了防止R_UniThreadCleanup执行中被cancel(可能造成内存泄露)，这里禁止异步取消*/
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
    pthread_cleanup_pop(1);
    /*恢复进入Vos_UniThreadEntry函数时的线程取消类型*/
    pthread_setcanceltype(dOldCancelType,NULL);
    return;
}

int Vos_StartTask(CHAR *pucName,WORD16 wPriority,WORD32 dwStacksize,TaskEntryProto tTaskEntry,ULONG dwTaskPara1)
{
    WORD32    tTaskId = 0;
    BOOLEAN  bRet = TRUE;
    pthread_attr_t        attr;
    pthread_t             threadID;
    struct sched_param    scheparam;
    LONG              thread_tab_index;
    int                   errcode   = 0;
    int               swMemPageSize = getpagesize();
     
    dwStacksize     += 2* swMemPageSize;    
#if 0
    pthread_attr_init(&attr);
    /*设置线程继承属性为非继承*/
    pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED);
    /*设置线程调度策略及相关参数*/
    pthread_attr_setschedpolicy(&attr,SCHED_FIFO);
    pthread_attr_getschedparam(&attr,&scheparam);
    scheparam.__sched_priority = wPriority;
    pthread_attr_setschedparam(&attr,&scheparam);
    /*设置线程堆栈大小以及警戒栈缓冲区*/
    pthread_attr_setstacksize(&attr,dwStacksize);
    pthread_attr_setguardsize(&attr,swMemPageSize);
#endif
    thread_tab_index = AddThreadInfo(tTaskId,(CHAR*)pucName,tTaskEntry,(VOID *)dwTaskPara1,wPriority,dwStacksize);

    errcode = pthread_create(&threadID,NULL,(VOID *)Vos_UniThreadEntry,(VOID *)thread_tab_index);

    if (0 != errcode)
    {
        printf("[%s:%d]create thread<%s> failed!errcode:%d,%s\n", __FUNCTION__, __LINE__,pucName,errcode);
        threadID = 0;
    }
    else
    {
        printf("task:%s create SUCC,thread id:0x%x\n",pucName,threadID);
    }

    SetThreadIDInfo(thread_tab_index , threadID);
    return threadID;
}
void InitMQ()
{
    WORD32 dwLoop = 0;
    /* 初始化队列控制块 */
    for (dwLoop = 0; dwLoop < MAX_QUEUE_COUNT; dwLoop++)
    {
        memset(&s_atQueueCtl[dwLoop], 0, sizeof(T_QueueCtl));
        /* 标记队列控制块没有被使用 */
        s_atQueueCtl[dwLoop].ucUsed = VOS_QUEUE_CTL_NO_USED;
    }
}
int StartAllTask()
{
    WORD16 wIndex;
    
    for (wIndex = 0; wIndex < SCHE_TASK_NUM; wIndex ++)
    {
        if ((M_atScheTCB[wIndex].dwMailBoxId =  Vos_CreateQueue(M_atScheTCB[wIndex].acName,9,sizeof(VOID *))) == 0xffffffff)
            {
                printf("[%s:%d]There is a ERROR of Vos_CreateQueue in StartAllTask()!\n" , __FUNCTION__, __LINE__);
                return -1;
            }

        if ((M_atScheTCB[wIndex].dwTaskId = Vos_StartTask(M_atScheTCB[wIndex].acName,
                                            M_atScheTCB[wIndex].wPriority,
                                            M_atScheTCB[wIndex].dwStacksize,
                                            (TaskEntryProto)(M_atScheTCB[wIndex].pEntry),
                                            wIndex)) == 0)
        {
            printf("[%s:%d]Start the %d Sche Task failure in StartAllTask()!\n", __FUNCTION__, __LINE__,wIndex);
            return -1;
        }
    }
}
void Vos_Delay(WORD32 dwTimeout)
{
    struct timespec delay;
    delay.tv_sec    = dwTimeout/1000;
    delay.tv_nsec   = (dwTimeout - delay.tv_sec*1000)*1000*1000;
    if (dwTimeout == 0)
    {
        delay.tv_nsec  = 1;
    }
    while(nanosleep (&delay, &delay) == -1 && errno == EINTR)
    {            
       continue;
    }
}
void InitCoreData()
{
    M_ptCoreData = (T_CoreData *)malloc(sizeof(T_CoreData));
    memset(M_ptCoreData, 0, sizeof(T_CoreData));
    /* 为进程控制块指针申请内存 */
    M_ptCoreData->wPCBCounts = g_procNum;
    M_ptCoreData->ptPCB = (T_PCB *)malloc((M_ptCoreData->wPCBCounts)* sizeof(T_PCB));
}
void Init()
{
    InitCoreData();
    memset(g_threadInfo,0,sizeof(g_threadInfo));/*初始化线程登记表信息*/
    /*初始化线程池及消息队列控制块*/
    InitTCBPool();
    InitMQ();
    /*初始化PCB池*/
    InitPCBPool();
}

int main()
{
    Init();
    StartAllTask();

    while(1)
    {
        Vos_Delay(3000);
        printf("main running\n");
    }
    return 0;
}

