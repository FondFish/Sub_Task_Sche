#include "stdio.h"
#include "sche.h"

__thread ucontext_t scheTask;
__thread T_PCB * gptSelfPCBInfo;

int public_para = 100;
extern T_CoreData*         M_ptCoreData;

VOID NormalReturnToTask(T_PCB *ptPCB)
{
    setcontext(&scheTask);
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

VOID UniProcEntry (T_PCB *ptPCB)
{
        ((UniBTSProcEntry_Type)(ptPCB->pEntry))(ptPCB->wState,
                                                100,
                                                (LPVOID)&public_para,
                                                NULL
                                               );	
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

    /*È¡µÃÍ·PCB*/
    ptHeadPCB    = M_atPCB + wHead;

    /*ÕÒµ½¾ÍÐ÷¶ÓÁÐÍ·µÄÇ°Ò»¸öPCBºÍºóÒ»¸öPCBË÷Òý*/
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

    if ((0xffff != ptDestPCB->wPrevByStatus)
            || (0xffff != ptDestPCB->wNextByStatus))
    {
        printf("Put the invalid PCB into ready queue tail in PutPCBToReadyQueueTail()\n\r");
        return 1;
    }

    ptScheTCB    = M_atScheTCB + ptDestPCB->wTno;
    wHead        = ptScheTCB->wReadyHead;
    ptHeadPCB    = M_atPCB + wHead;

    if (WEOF == wHead)
    {
        ptScheTCB->wReadyHead     = wPCBIndex;
        ptDestPCB->wPrevByReady  = wPCBIndex;
        ptDestPCB->wNextByReady  = wPCBIndex;
    }
    else
    {
        wTail = ptHeadPCB->wPrevByReady;

        M_atPCB[wTail].wNextByReady  = wPCBIndex;
        ptHeadPCB->wPrevByReady      = wPCBIndex;
        ptDestPCB->wPrevByReady      = wTail;
        ptDestPCB->wNextByReady      = wHead;
    }

    ptScheTCB->wReadyCounts ++;

    ptDestPCB->ucRunStatus  = PROC_STATUS_READY;

    return 0;
}

__attribute__ ((hot)) VOID ScheTaskEntry(WORD32 dwTno)
{
    WORD16            wPCBIndex;
    T_ScheTCB         *ptSelfTCB;
    T_PCB             *ptDestPCB;

    ptSelfTCB               = M_atScheTCB + dwTno;
    ptSelfTCB->dwScheCounts = 0;
    printf("\n [OSS:%s:%d]ScheTaskEntry Tno = %d\n",  __FUNCTION__, __LINE__,dwTno);

    for (;;)
    {
        while (0 < ptSelfTCB->wReadyCounts)
        {
            wPCBIndex = GetPCBFromReadyQueueHead(ptSelfTCB);
            ptSelfTCB->wRunning = wPCBIndex;
            ptDestPCB           = M_atPCB + wPCBIndex;

            ptDestPCB->ucRunStatus = PROC_STATUS_RUNNING;

            ptSelfTCB->dwScheCounts ++;

            ptDestPCB->dwScheCounts++;    
            
            gptSelfPCBInfo = ptDestPCB;
            
            RunProcess(ptDestPCB);
            
            PutPCBToReadyQueueTail(wPCBIndex);
        }
        
        printf("ready queue is empty\n");
        break;
    }/* end while */

    printf("Task (%s) exit loop!\n",ptSelfTCB->acName);

}
