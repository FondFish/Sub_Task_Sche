#include "stdio.h"
#include "string.h"
#include "sche.h"
#include "config.h"

__thread ucontext_t scheTask;
__thread T_PCB * gptSelfPCBInfo;

int public_para = 100;

extern  WORD16 GetTaskScheProcCounts(WORD16 wScheTaskNo);

T_PCB *R_GetSelfPCBPtr()
{
    return gptSelfPCBInfo;
}

ULONG GetCurTaskID()
{
    ULONG dwSysTaskId;

    dwSysTaskId = pthread_self();
    return dwSysTaskId;
}

VOID NormalReturnToTask(T_PCB *ptPCB)
{
    setcontext(&scheTask);
}

VOID AbnormalReturnToTask(T_PCB *ptPCB)
{
    ucontext_t op;
    
    ptPCB->dwProcSP = (ULONG)&op;
    swapcontext(&op, &scheTask);
}

void SetNextPCBState(WORD16 wState)
{
    T_PCB       *ptSelfPCB;

    ptSelfPCB = R_GetSelfPCBPtr();

    ptSelfPCB->wRetState = wState;

    NormalReturnToTask(ptSelfPCB);
}

VOID UniProcEntry (T_PCB *ptPCB)
{
    T_MSG *pCurMsg = ptPCB->ptCurMsg;
    BYTE  *pucInputMsgBody = (BYTE *)(pCurMsg + 1);; /*待处理消息体*/
    
    ((UniBTSProcEntry_Type)(ptPCB->pEntry))(ptPCB->wState,
                                                pCurMsg->wMsgId,
                                                (LPVOID)pucInputMsgBody,
                                                (LPVOID)ptPCB->pucDataRegion);	
}

VOID ResumeProcess(T_PCB *ptPCB)
{
    swapcontext(&scheTask,(ucontext_t *)ptPCB->dwProcSP);
}

VOID BeginProcess(T_PCB *ptPCB)
{
    ucontext_t op;
    getcontext(&op);
    
    ptPCB->dwProcSP = (ULONG)ptPCB->pucStack + ptPCB->dwStackSize;

    op.uc_stack.ss_sp =  ptPCB->pucStack;
    op.uc_stack.ss_size = ptPCB->dwStackSize;//指定栈空间大小  
    op.uc_stack.ss_flags = 0;  
    op.uc_link = &scheTask;

    makecontext(&op,(void (*)(void))UniProcEntry, 1, ptPCB);
    swapcontext(&scheTask,&op);
}
VOID RunProcess(T_PCB *ptPCB)
{
    if (WAIT_MSG == ptPCB->ucBlockReason)
    {
        BeginProcess(ptPCB);
    }
    else
    {
        ResumeProcess(ptPCB);
    }
}

int ReleasePCBFromBlockQueue(WORD16 wPCBIndex)
{
    WORD16        wPrev;
    WORD16        wNext;
    T_PCB         *ptDestPCB;
    T_ScheTCB     *ptScheTCB;

    if (wPCBIndex >= M_ptCoreData->wPCBCounts)
    {
        return ERROR_UNKNOWN;
    }

    ptDestPCB    = M_atPCB + wPCBIndex;
    ptScheTCB = M_atScheTCB + ptDestPCB->wTno;
    
    wPrev        = ptDestPCB->wPrevByStatus;
    wNext        = ptDestPCB->wNextByStatus;

    if ((WEOF == ptScheTCB->wBlockHead) || (0 == ptScheTCB->wBlockCounts))
    {
        /*不存在阻塞OP*/
        return ERROR_UNKNOWN;
    }

    if ((wPrev == wPCBIndex) && (wNext == wPCBIndex))
    {
        ptScheTCB->wBlockHead = WEOF;
    }
    else
    {
        /*从阻塞队列摘除*/
        M_atPCB[wPrev].wNextByStatus   = wNext;
        M_atPCB[wNext].wPrevByStatus   = wPrev;
        if (wPCBIndex == ptScheTCB->wBlockHead)
        {
            ptScheTCB->wBlockHead = wNext;
        }
    }
    /*将该PCB重设为原始态，等待调度*/
    ptDestPCB->wNextByStatus  = WEOF;
    ptDestPCB->wPrevByStatus  = WEOF;
    ptScheTCB->wBlockCounts --;
    
    return SUCCESS;
}

int PutPCBToBlockQueueTail(WORD16 wPCBIndex)
{
    WORD16        wHead;
    WORD16        wTail;
    T_PCB         *ptDestPCB;
    T_PCB         *ptHeadPCB;
    T_ScheTCB     *ptScheTCB;

    /*索引应该小于单板的PCB池总数*/
    if (wPCBIndex >= M_ptCoreData->wPCBCounts)
    {
        printf("Invalid PCBIndex in PutPCBToBlockQueueTail()\n\r");
        return ERROR_UNKNOWN;
    }

    /*根据索引取得目标PCB指针*/
    ptDestPCB    = M_atPCB + wPCBIndex;

    if ((WEOF != ptDestPCB->wPrevByStatus) || (WEOF != ptDestPCB->wNextByStatus))
    {
        printf("Put the invalid PCB into block queue in PutPCBToBlockQueueTail()\n\r");
        return ERROR_UNKNOWN;
    }

    /*取得目的PCB所属的调度任务T_ScheTCB指针*/
    ptScheTCB    = M_atScheTCB + ptDestPCB->wTno;

    /*获取调度任务的阻塞队列的头PCB*/
    wHead        = ptScheTCB->wBlockHead;
    ptHeadPCB    = M_atPCB + wHead;

    if (WEOF == wHead)
    {
        /*阻塞队列为空，则欲加入的PCB即入阻塞队列头*/
        ptScheTCB->wBlockHead     = wPCBIndex;
        ptDestPCB->wPrevByStatus  = wPCBIndex;
        ptDestPCB->wNextByStatus  = wPCBIndex;
    }
    else
    {
        /*获取阻塞队列的尾PCB索引*/
        wTail = ptHeadPCB->wPrevByStatus;

        /*将欲加入PCB放在当前尾PCB之后*/
        M_atPCB[wTail].wNextByStatus   = wPCBIndex;
        ptHeadPCB->wPrevByStatus      = wPCBIndex;
        ptDestPCB->wPrevByStatus      = wTail;
        ptDestPCB->wNextByStatus      = wHead;
    }

    ptScheTCB->wBlockCounts ++;
    ptDestPCB->ucRunStatus  = PROC_STATUS_BLOCK;

    return SUCCESS;
}

WORD16 GetPCBFromReadyQueueHead(T_ScheTCB *ptScheTCB)
{
    WORD16        wHead;
    WORD16        wPrev;
    WORD16        wNext;
    T_PCB         *ptHeadPCB;

    /*取得调度任务就绪队列的头PCB索引*/
    wHead    = ptScheTCB->wReadyHead;

    if (WEOF == wHead)
    {
        printf("not found ready pcb head in \n\r");
        return 0xffff;
    }

    /*取得头PCB*/
    ptHeadPCB    = M_atPCB + wHead;

    //printf("start Run Proc=0x%x,TCB:%s\n",ptHeadPCB->wProcType,ptScheTCB->acName);

    /*找到就绪队列头的前一个PCB和后一个PCB索引*/
    wPrev        = ptHeadPCB->wPrevByStatus;
    wNext        = ptHeadPCB->wNextByStatus;

    if ((wPrev == wHead) && (wNext == wHead))
    {
        /*如果就绪队列只有一个PCB*/
        ptScheTCB->wReadyHead = 0xffff;
    }
    else
    {
        M_atPCB[wPrev].wNextByStatus   = wNext;
        M_atPCB[wNext].wPrevByStatus   = wPrev;

        /*下一个PCB作为就绪队列的头PCB*/
        ptScheTCB->wReadyHead          = wNext;
    }
    /*被摘出来后悬空处理*/
    ptHeadPCB->wNextByStatus       = WEOF;
    ptHeadPCB->wPrevByStatus       = WEOF;

    ptScheTCB->wReadyCounts --;
    return wHead;
}
int PutPCBToReadyQueueTail(WORD16 wPCBIndex)
{
    WORD16        wHead;
    WORD16        wTail;
    T_PCB         *ptDestPCB;
    T_PCB         *ptHeadPCB;
    T_ScheTCB     *ptScheTCB;

    if (wPCBIndex >= M_ptCoreData->wPCBCounts)
    {
        printf("Invalid PCBIndex in PutPCBToReadyQueueTail()\n\r");
        return 1;
    }

    ptDestPCB    = M_atPCB + wPCBIndex;

    if ((WEOF != ptDestPCB->wPrevByStatus)
            || (WEOF != ptDestPCB->wNextByStatus))
    {
        printf("Put the invalid PCB into ready queue tail in PutPCBToReadyQueueTail()\n\r");
        return 1;
    }

    ptScheTCB    = M_atScheTCB + ptDestPCB->wTno;
    wHead        = ptScheTCB->wReadyHead;
    ptHeadPCB    = M_atPCB + wHead;

    if (WEOF == wHead)
    {
        //printf("first Proc=0x%x,TCB:%s\n",ptDestPCB->wProcType,ptScheTCB->acName);
        ptScheTCB->wReadyHead     = wPCBIndex;
        ptDestPCB->wPrevByStatus  = wPCBIndex;
        ptDestPCB->wNextByStatus  = wPCBIndex;
    }
    else
    {  
        wTail = ptHeadPCB->wPrevByStatus;
        
        //printf("Now Proc=0x%x,TCB:%s\n",ptDestPCB->wProcType,ptScheTCB->acName);
        //printf("Prev Proc=0x%x,TCB:%s\n",M_atPCB[wTail].wProcType,ptScheTCB->acName);

        M_atPCB[wTail].wNextByStatus  = wPCBIndex;
        ptHeadPCB->wPrevByStatus      = wPCBIndex;
        ptDestPCB->wPrevByStatus      = wTail;
        ptDestPCB->wNextByStatus      = wHead;
    }

    ptScheTCB->wReadyCounts ++;

    ptDestPCB->ucRunStatus  = PROC_STATUS_READY;

    return 0;
}

VOID ScheTaskEntry(int dwTno)
{
    WORD16            wPCBIndex;
    T_ScheTCB         *ptSelfTCB;
    T_PCB             *ptDestPCB;
    T_MSG             *ptMsgList;
    /*根据参数获得调度任务的TCB指针*/
    ptSelfTCB               = M_atScheTCB + dwTno;
    ptSelfTCB->dwScheCounts = 0;
    
    printf("\n [%s:%d] Tno = %d Start  Sche PCB\n",  __FUNCTION__, __LINE__,dwTno);

    for(;;)
    {
        /*任务死循环开始*/
        while (0 < ptSelfTCB->wReadyCounts)
        {
            /*在任务的就绪队列中取出头PCB*/
            wPCBIndex = GetPCBFromReadyQueueHead(ptSelfTCB);

            /*设置调度任务当前运行的进程索引*/
            ptSelfTCB->wRunning = wPCBIndex;

            /*根据索引找到目的PCB*/
            ptDestPCB           = M_atPCB + wPCBIndex;

            ptDestPCB->ucRunStatus = PROC_STATUS_RUNNING;/*设运行状态为正在运行*/

            ptSelfTCB->dwScheCounts ++;

            if(ptSelfTCB->dwScheCounts >100)
            {
                printf("Tno=%d stop shceTask\n",dwTno);
                return;
            }
            printf("\n ScheTaskEntry Tno = %d,cnt=%d \n",dwTno,ptSelfTCB->dwScheCounts);
            
            ptDestPCB->dwScheCounts++;    
            
            gptSelfPCBInfo = ptDestPCB;
            
            RunProcess(ptDestPCB);
            
            /*阻塞原因是同步调用阻塞，则把PCB放入阻塞队列尾*/
            if (WAIT_MSG != ptDestPCB->ucBlockReason)
            {
                PutPCBToBlockQueueTail(wPCBIndex);
            }
            else
            {
                /* 此时PCB才是真正地运行完毕 当前消息无用，可以释放掉 */
                FreeCurMsg(ptDestPCB);

                /*更新业务状态*/
                ptDestPCB->wState     = ptDestPCB->wRetState;
                if (NULL == ptDestPCB->ptCurMsg)
                {
                    /*如果进程的当前消息为空，则置入阻塞队列尾，并设置阻塞原因为无消息*/
                    PutPCBToBlockQueueTail(wPCBIndex);
                    ptDestPCB->ucBlockReason = WAIT_MSG;
                }
                else
                {
                    /*有当前消息，则将进程放入就绪队列尾*/
                    PutPCBToReadyQueueTail(wPCBIndex);
                }
            }
            
            sleep(1);
        }
        printf("ready queue is empty\n");
        
        /*在任务的邮箱里取出一条消息指针，无消息阻塞任务*/
        if (Vos_PopQueue(ptSelfTCB->dwMailBoxId, (WORD32)WAIT_FOREVER,
                         (BYTE *)&ptMsgList, sizeof(VOID *)) == VOS_QUEUE_ERROR )
        {
            break;
        }
        else
        {
            DispatchMsgListToPCB(ptMsgList,dwTno);
        }
    }

    printf("Task (%s) exit loop!\n",ptSelfTCB->acName);

}

int InitPCBPool(VOID)
{
    WORD16            wIndex;
    T_PCB             *ptDestPCB;
    T_ProcessConfig     *ptCurType;
    PID      tReceiver;

    for (wIndex = 0; wIndex < g_procNum; wIndex ++)
    {
        ptCurType    = gaProcCfgTbl + wIndex;

        ptDestPCB    = M_atPCB + wIndex;/*目的PCB指针*/

        /*为目的PCB堆栈提交内存，大小由所属进程类型指定*/
        ptDestPCB->pucStack = malloc(ptCurType->dwStackSize);

        if (NULL == ptDestPCB->pucStack)
        {
            printf("[%s:%d]Alloc Proc Stack ERROR in InitPCBPool()\n" , __FUNCTION__, __LINE__);
            return   ERROR_UNKNOWN;
        }
        
        /* 进程栈需要保留16个字节 */
        ptDestPCB->dwStackSize      = ptCurType->dwStackSize - 16;

        /*设置进程的私有数据区大小*/
        ptDestPCB->dwDataRegionSize = ptCurType->dwDataRegionSize;

        /*初始化该进程控制块的成员变量*/
        ptDestPCB->ucIsUse           = YES;
        ptDestPCB->wInstanceNo   = 1;
        ptDestPCB->ucBlockReason       = WAIT_MSG;
        ptDestPCB->wProcType     = ptCurType->wProcType;
        ptDestPCB->wTno              = ptCurType->wTno;
        ptDestPCB->pEntry             = ptCurType->pEntry;
        ptDestPCB->wState              = S_StartUp;
        ptDestPCB->wRetState         = S_StartUp;
        ptDestPCB->pucDataRegion = NULL;/*先设成空*/
        /* 进程栈指针初始指向栈底 */
        ptDestPCB->dwProcSP            = (ULONG)ptDestPCB->pucStack + ptDestPCB->dwStackSize;  
        ptDestPCB->dwTaskSP            = 0;
        ptDestPCB->wPrevByStatus     = WEOF;
        ptDestPCB->wNextByStatus     = WEOF;
        /*初始化PCB消息链表*/
        ptDestPCB->ptCurMsg = NULL;
        ptDestPCB->ptPrevMsg = NULL;
        ptDestPCB->ptFirstMsg = NULL;
        ptDestPCB->ptLastMsg = NULL;

        PutPCBToBlockQueueTail(wIndex);
        
        tReceiver.wPno = ptCurType->wProcType;
        R_SendMsgFromTask(EV_STARTUP,NULL,0,COMM_ASYN_NORMAL,&tReceiver);
    }

    return SUCCESS;
}

VOID InitTCBPool(VOID)
{
    WORD16      wIndex;
    T_ScheTCB   *ptScheTCB;

    /*以下for循环遍历调度任务，并初始化调度任务TCB的成员变量 */
    for (wIndex = 0; wIndex <SCHE_TASK_NUM; wIndex ++)
    {
        /*获得调度任务控制块指针*/
        ptScheTCB    = M_atScheTCB + wIndex;

        /*将调度任务配置表中的配置信息拷贝进任务控制块*/
        memcpy(ptScheTCB,gaScheTaskConfig + wIndex,sizeof(T_TaskConfig));
        
        ptScheTCB->dwMailBoxId        = VOS_CREATE_QUEUE_FAILURE;
        ptScheTCB->dwTaskId              = 0;
        ptScheTCB->wBlockHead         = WEOF;
        ptScheTCB->wReadyHead        = WEOF;
        ptScheTCB->wRunning             = WEOF;
        ptScheTCB->wReadyCounts     = 0;
        ptScheTCB->wBlockCounts       = 0;
        ptScheTCB->dwScheCounts      = 0;

        /*获得该任务挂载的进程类型数目*/
        ptScheTCB->wProcTypeCounts = GetTaskScheProcCounts(wIndex);
    }
}

