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
    int dwQueueCtrlId      = VOS_CREATE_QUEUE_FAILURE;/* ±êÊ¶ÕÒµ½µÄ¿ÉÓÃ¶ÓÁÐID */
    T_QueueCtl *pQueueCtl = NULL;
    WORD32      dwQueueId     = 0;
    struct      mq_attr attr;

    dwEntryCount += 1; /* ²¹×ã¶ÓÁÐ³¤¶È */
    
    pthread_mutex_lock(&s_mutex);
    
    /* ²éÕÒ¿ÕÏÐ¶ÓÁÐ¿ØÖÆ¿é */
    for (dwQueCtlLoop = 0; dwQueCtlLoop <MAX_QUEUE_COUNT; dwQueCtlLoop++)
    {
        if (s_atQueueCtl[dwQueCtlLoop].ucUsed == VOS_QUEUE_CTL_NO_USED)
        {
            /* ÕÒµ½¿ÉÓÃµÄ¶ÓÁÐ¿ØÖÆ¿é */
            dwQueueCtrlId = dwQueCtlLoop;
            break;
        }
    }
    
    if (dwQueCtlLoop<MAX_QUEUE_COUNT)
        pQueueCtl = &s_atQueueCtl[dwQueCtlLoop];

    pQueueCtl->dwEntrySize = dwEntrySize;
    /* ¼ÙÉè¶ÓÁÐ³¤¶ÈÎª100¸öÔªËØ£¬Êµ¼ÊÖ»ÄÜ´æ·Å99¸öÔªËØ */
    pQueueCtl->dwTotalCount = dwEntryCount - 1;
    pQueueCtl->dwUsedCount = 0;
    pQueueCtl->ucUsed = VOS_QUEUE_CTL_USED;
    
    pthread_mutex_unlock(&s_mutex);

   /* ±£Ö¤Î¨Ò»ÐÔ,±ÜÃâ´ò¿ªÒÑ¾­´ò¿ªµÄ¶ÓÁÐ */
    sprintf((CHAR *)(pQueueCtl->aucQueueName), MSQNAME, dwQueueCtrlId);

    /* Çå³ýÏûÏ¢¶ÓÁÐ£¬·ÀÖ¹Ê¹ÓÃ¾ÉµÄÏûÏ¢¶ÓÁÐ */
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
    /*´ýÊµÏÖ*/
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

    pthread_getattr_np(g_threadInfo[idx].thread_id, &attr);   
    pthread_attr_getstack(&attr,&pStackBase,(size_t *)&dwStackSize);
    pthread_attr_getguardsize(&attr, (size_t *)&dwGuardSize);             


    /* ÏÖÔÚÓÉpStackBase£¬dwTaskStackSize£¬dwGuardSizeÈý¸ö±äÁ¿Ëù¹´»­µÄÈÎÎñ¶ÑÕ»²¼¾Ö£º

          µÍµØÖ· ---------------------<--------¶ÑÕ»Ôö³¤·½Ïò------------------------------<- ¸ßµØÖ·   

               |<------------------------dwTaskStackSize-------------------------------->| 
    -----------|----------------------|--------------------------------------------------|-------------
               | ±£»¤Ò³(dwGuardSize)  |                Êµ¼Ê¿ÉÓÃµÄ¶ÑÕ»                    |
    -----------|----------------------|--------------------------------------------------|-------------
               ^ 
          pStackBase                          
    */
    g_threadInfo[idx].stackaddr = (ULONG)pStackBase;
    g_threadInfo[idx].stackguardsize = dwGuardSize;
    g_threadInfo[idx].thread_tid = gettid();

    pFun    = g_threadInfo[idx].pFun;
    arg     = g_threadInfo[idx].arg;

    prctl(PR_SET_NAME,getThreadNameByID(g_threadInfo[idx].thread_id),0,0,0);

    pthread_cleanup_push(R_UniThreadCleanup,g_threadInfo + idx);

    if (0 != pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &dOldCancelType))
    {
        printf("[OSS:%s:%d]setcanceltype failed: %s [so you can't stop me]\n", __FUNCTION__, __LINE__, strerror (errno));
        return;
    } 

    (*pFun)((VOID*)arg);

    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
    pthread_cleanup_pop(1);

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
    OSS_LONG              thread_tab_index;
    int                   errcode   = 0;
    int               swMemPageSize = getpagesize();
     
    dwStacksize     += 2* swMemPageSize;    
#if 0
    pthread_attr_init(&attr);

    pthread_attr_setinheritsched(&attr,PTHREAD_EXPLICIT_SCHED);

    pthread_attr_setschedpolicy(&attr,SCHED_FIFO);
    pthread_attr_getschedparam(&attr,&scheparam);
    scheparam.__sched_priority = wPriority;
    pthread_attr_setschedparam(&attr,&scheparam);

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

    for (dwLoop = 0; dwLoop < MAX_QUEUE_COUNT; dwLoop++)
    {
        memset(&s_atQueueCtl[dwLoop], 0, sizeof(T_QueueCtl));
       
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
    
    M_ptCoreData->wPCBCounts = g_procNum;
    M_ptCoreData->ptPCB = (T_PCB *)malloc((M_ptCoreData->wPCBCounts)* sizeof(T_PCB));
}
void Init()
{
    InitCoreData();
    memset(g_threadInfo,0,sizeof(g_threadInfo));
    
    InitTCBPool();
    InitMQ();
    
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

