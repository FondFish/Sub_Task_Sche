#include "stdio.h"
#include "string.h"
#include "sche.h"
#include "config.h"

__thread ucontext_t scheTask;
__thread T_PCB * gptSelfPCBInfo;

int public_para = 100;

extern  WORD16 GetTaskScheProcCounts(WORD16 wScheTaskNo);

VOID NormalReturnToTask(T_PCB *ptPCB)
{
    setcontext(&scheTask);
}

VOID UniProcEntry (T_PCB *ptPCB)
{
    ((UniBTSProcEntry_Type)(ptPCB->pEntry))(ptPCB->wState,
                                                100,
                                                (LPVOID)&public_para,
                                                NULL
                                               );	
}

VOID RunProcess(T_PCB *ptPCB)
{
    ucontext_t op;
    getcontext(&op);
    
    ptPCB->dwProcSP = (OSS_ULONG)ptPCB->pucStack + ptPCB->dwStackSize;

    op.uc_stack.ss_sp =  ptPCB->pucStack;
    op.uc_stack.ss_size = ptPCB->dwStackSize;
    op.uc_stack.ss_flags = 0;  
    op.uc_link = &scheTask;

    makecontext(&op,(void (*)(void))UniProcEntry, 1, ptPCB);
    swapcontext(&scheTask,&op);
}

WORD16 GetPCBFromReadyQueueHead(T_ScheTCB *ptScheTCB)
{
    WORD16        wHead;
    WORD16        wPrev;
    WORD16        wNext;
    T_PCB         *ptHeadPCB;

    wHead    = ptScheTCB->wReadyHead;

    if (0xffff == wHead)
    {
        printf("not found ready pcb head in \n\r");
        return 0xffff;
    }
    ptHeadPCB    = M_atPCB + wHead;

    //printf("start Run Proc=0x%x,TCB:%s\n",ptHeadPCB->wProcType,ptScheTCB->acName);
    wPrev        = ptHeadPCB->wPrevByReady;
    wNext        = ptHeadPCB->wNextByReady;

    if ((wPrev == wHead) && (wNext == wHead))
    {
        ptScheTCB->wReadyHead = 0xffff;
    }
    else
    {
        M_atPCB[wPrev].wNextByReady   = wNext;
        M_atPCB[wNext].wPrevByReady   = wPrev;
        ptScheTCB->wReadyHead          = wNext;
    }
    ptHeadPCB->wNextByReady       = WEOF;
    ptHeadPCB->wPrevByReady       = WEOF;

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

    if ((0xffff != ptDestPCB->wPrevByReady)
            || (0xffff != ptDestPCB->wNextByReady))
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
        ptDestPCB->wPrevByReady  = wPCBIndex;
        ptDestPCB->wNextByReady  = wPCBIndex;
    }
    else
    {  
        wTail = ptHeadPCB->wPrevByReady;
        
        //printf("Now Proc=0x%x,TCB:%s\n",ptDestPCB->wProcType,ptScheTCB->acName);
        //printf("Prev Proc=0x%x,TCB:%s\n",M_atPCB[wTail].wProcType,ptScheTCB->acName);

        M_atPCB[wTail].wNextByReady  = wPCBIndex;
        ptHeadPCB->wPrevByReady      = wPCBIndex;
        ptDestPCB->wPrevByReady      = wTail;
        ptDestPCB->wNextByReady      = wHead;
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

    ptSelfTCB               = M_atScheTCB + dwTno;
    ptSelfTCB->dwScheCounts = 0;
    
    printf("\n [%s:%d]ScheTaskEntry Tno = %d\n",  __FUNCTION__, __LINE__,dwTno);

    for(;;)
    {
        while (0 < ptSelfTCB->wReadyCounts)
        {
            wPCBIndex = GetPCBFromReadyQueueHead(ptSelfTCB);
            ptSelfTCB->wRunning = wPCBIndex;
            ptDestPCB           = M_atPCB + wPCBIndex;

            ptDestPCB->ucRunStatus = PROC_STATUS_RUNNING;

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
            
            PutPCBToReadyQueueTail(wPCBIndex);
            
            sleep(1);
        }
        
        printf("ready queue is empty\n");
        break;
    }

    printf("Task (%s) exit loop!\n",ptSelfTCB->acName);

}

int InitPCBPool(VOID)
{
    WORD16            wIndex;
    T_PCB             *ptDestPCB;
    T_ProcessConfig     *ptCurType;

    for (wIndex = 0; wIndex < g_procNum; wIndex ++)
    {
        ptCurType    = gaProcCfgTbl + wIndex;
        ptDestPCB    = M_atPCB + wIndex;
        ptDestPCB->pucStack = malloc(ptCurType->dwStackSize);

        if (NULL == ptDestPCB->pucStack)
        {
            printf("[%s:%d]Alloc Proc Stack ERROR in InitPCBPool()\n" , __FUNCTION__, __LINE__);
            return   OSS_ERROR_UNKNOWN;
        }
        
        ptDestPCB->dwStackSize      = ptCurType->dwStackSize - 16;
        ptDestPCB->dwDataRegionSize = ptCurType->dwDataRegionSize;
        ptDestPCB->ucIsUse           = YES;
        ptDestPCB->wInstanceNo   = 1;
        ptDestPCB->wProcType     = ptCurType->wProcType;
        ptDestPCB->wTno              = ptCurType->wTno;
        ptDestPCB->pEntry             = ptCurType->pEntry;
        ptDestPCB->wState              = S_StartUp;
        ptDestPCB->dwProcSP            = (OSS_ULONG)ptDestPCB->pucStack + ptDestPCB->dwStackSize;  
        ptDestPCB->dwTaskSP            = 0;
        ptDestPCB->wPrevByReady     = WEOF;
        ptDestPCB->wNextByReady     = WEOF;

        printf("put ProcType=0x%x into Ready Queue,Tno=%d,Stack=0x%x,SP=0x%x\n",
            ptDestPCB->wProcType,ptDestPCB->wTno,ptDestPCB->pucStack,ptDestPCB->dwProcSP);
        
        PutPCBToReadyQueueTail(wIndex);
    }

    return OSS_SUCCESS;
}

VOID InitTCBPool(VOID)
{
    WORD16      wIndex;
    T_ScheTCB   *ptScheTCB;
    
    for (wIndex = 0; wIndex <SCHE_TASK_NUM; wIndex ++)
    {
        ptScheTCB    = M_atScheTCB + wIndex;
        memcpy(ptScheTCB,gaScheTaskConfig + wIndex,sizeof(T_TaskConfig));
        
        ptScheTCB->dwMailBoxId        = VOS_CREATE_QUEUE_FAILURE;
        ptScheTCB->dwTaskId              = 0;
        ptScheTCB->wBlockHead         = WEOF;
        ptScheTCB->wReadyHead        = WEOF;
        ptScheTCB->wRunning             = WEOF;
        ptScheTCB->wReadyCounts     = 0;
        ptScheTCB->wBlockCounts       = 0;
        ptScheTCB->dwScheCounts      = 0;

        /*»ñµÃ¸ÃÈÎÎñ¹ÒÔØµÄ½ø³ÌÀàÐÍÊýÄ¿*/
        ptScheTCB->wProcTypeCounts = GetTaskScheProcCounts(wIndex);
    }
}
