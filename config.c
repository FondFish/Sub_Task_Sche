
#include "stdio.h"
#include "string.h"
#include "sche.h"
#include "config.h"
#include <pthread.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <sys/syscall.h>

#define    SCH0       0
#define    SCH1       1
#define    SCH2       2

#define    Proc_Type_Test1      0x1000
#define    Proc_Type_Test2      0x2000
#define    Proc_Type_Test3      0x3000
#define    Proc_Type_Test4      0x4000


T_CoreData*         M_ptCoreData; 

#define M_atScheTCB                         (M_ptCoreData->atScheTCB)
#define M_atPCB                             (M_ptCoreData->ptPCB)

T_QueueCtl   s_atQueueCtl[MAX_QUEUE_COUNT]; 

pthread_mutex_t                   g_threadTableMutex = PTHREAD_MUTEX_INITIALIZER;
T_ThreadInfo                      g_threadInfo[SCHE_TASK_NUM];

int g_test1_cnt = 0;
int g_test2_cnt = 0;
int g_test3_cnt = 0;
int g_test4_cnt = 0;


void test1_Entry(WORD wState, WORD wSignal, void *pSignalPara, void *pVarP);
void test2_Entry(WORD wState, WORD wSignal, void *pSignalPara, void *pVarP);
void test3_Entry(WORD wState, WORD wSignal, void *pSignalPara, void *pVarP);
void test4_Entry(WORD wState, WORD wSignal, void *pSignalPara, void *pVarP);


T_TaskConfig    gaScheTaskConfig[SCHE_TASK_NUM] = {
    { YES, "SCH0",  170,    4096*2,   &ScheTaskEntry},
    { YES, "SCH1",  160,    4096*2,   &ScheTaskEntry}
};

int g_TaskNum = sizeof(gaScheTaskConfig)/sizeof(gaScheTaskConfig[0]);

T_ProcessConfig gaProcCfgTbl[] =
{
    {YES,  Proc_Type_Test1,    SCH0,    test1_Entry,  4096*1,  4096*2},
    {YES,  Proc_Type_Test2,    SCH0,    test2_Entry,  4096*1,  4096*2},
    {YES,  Proc_Type_Test3,    SCH1,    test3_Entry,  4096*1,  4096*2},
    {YES,  Proc_Type_Test4,    SCH1,    test4_Entry,  4096*1,  4096*2}
};

int g_procNum = sizeof(gaProcCfgTbl)/sizeof(gaProcCfgTbl[0]);

void test1_Entry(WORD wState, WORD wSignal, void *pSignalPara, void *pVarP)
{
    int i;
    int num=0;
    
    g_test1_cnt ++;
    
    for(i=0;i<g_test1_cnt;i++)
        num += i;
    
    printf("test1 Entry cnt=%d,Num addr=0x%x\n",g_test1_cnt,&num);

    NormalReturnToTask(gptSelfPCBInfo);
    
    return;
}

void test2_Entry(WORD wState, WORD wSignal, void *pSignalPara, void *pVarP)
{
    g_test2_cnt ++;
    printf("test2 Entry cnt=%d\n",g_test2_cnt);
    
    NormalReturnToTask(gptSelfPCBInfo);
    
    return;
}

void test3_Entry(WORD wState, WORD wSignal, void *pSignalPara, void *pVarP)
{
    g_test3_cnt ++;
    printf("test3 Entry cnt=%d\n",g_test3_cnt);
    
    NormalReturnToTask(gptSelfPCBInfo);
    
    return;
}

void test4_Entry(WORD wState, WORD wSignal, void *pSignalPara, void *pVarP)
{
    g_test4_cnt ++;
    printf("test4 Entry cnt=%d\n",g_test4_cnt);
    
    NormalReturnToTask(gptSelfPCBInfo);
    
    return;
}

int AddThreadInfo(WORD32 threadid,const CHAR *name,TaskEntryProto Entry,VOID *parg,INT pri,INT stacksize)
{
    int i;

    if(pthread_mutex_lock(&g_threadTableMutex)!=0)
    {
        return -1;
    }

    for(i=0; i<SCHE_TASK_NUM; i++)
    {
        if(0 == g_threadInfo[i].flag)
        {
            break;
        }
    }
    
    if(SCHE_TASK_NUM <= i)
    {
        pthread_mutex_unlock(&g_threadTableMutex);
        return -1;
    }
    
    strncpy(g_threadInfo[i].name,name,TASK_NAME_LEN);
    
    g_threadInfo[i].thread_id   = threadid;
    g_threadInfo[i].pri         = pri;
    g_threadInfo[i].stacksize   = stacksize;
    g_threadInfo[i].flag        = 1;
    g_threadInfo[i].pFun        = Entry;
    g_threadInfo[i].arg         = parg;
    g_threadInfo[i].tSpareInfo  = 0;
    g_threadInfo[i].pbySigAltStack = NULL;

    pthread_mutex_unlock(&g_threadTableMutex);
    return i;
}

int SetThreadIDInfo(int index, WORD32 threadid)
{
    if(pthread_mutex_lock(&g_threadTableMutex)!=0)
    {
        printf("[%s:%d]sub_SetThreadIDInDebugTable:mutex_lock failed!\n" , __FUNCTION__, __LINE__);
        return -1;
    }

    if(threadid != INVALID_SYS_TASKID )
    {
        g_threadInfo[index].thread_id  = threadid;
    }

    pthread_mutex_unlock(&g_threadTableMutex);
    return index;
}
pid_t gettid(VOID)
{
    static  __thread  pid_t tSelfTid = 0;

    if (0 == tSelfTid)
    {
        tSelfTid = (pid_t)syscall(__NR_gettid);
    }
    return tSelfTid;
}

WORD16 GetTaskScheProcCounts(WORD16 wScheTaskNo)
{
    WORD16 wIndex;
    WORD16 wResult = 0;

    if (wScheTaskNo >= SCHE_TASK_NUM)
    {
        printf("[%s:%d]Invalid TaskNo (%d) in GetProcTypeCounts()\n\r", __FUNCTION__, __LINE__, wScheTaskNo);
        return wResult;
    }

    for (wIndex = 0; wIndex<g_procNum; wIndex ++)
    {
        if (gaProcCfgTbl[wIndex].wTno == wScheTaskNo)
        {
            wResult ++;
        }
    }
    return wResult;
}
