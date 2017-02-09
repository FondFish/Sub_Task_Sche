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
    BYTE  *pucInputMsgBody = (BYTE *)(pCurMsg + 1);; /*´ý´¦ÀíÏûÏ¢Ìå*/
    
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
    op.uc_stack.ss_size = ptPCB->dwStackSize;//Ö¸¶¨Õ»¿Õ¼ä´óÐ¡  
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
        /*²»´æÔÚ×èÈûOP*/
        return ERROR_UNKNOWN;
    }

    if ((wPrev == wPCBIndex) && (wNext == wPCBIndex))
    {
        ptScheTCB->wBlockHead = WEOF;
    }
    else
    {
        /*´Ó×èÈû¶ÓÁÐÕª³ý*/
        M_atPCB[wPrev].wNextByStatus   = wNext;
        M_atPCB[wNext].wPrevByStatus   = wPrev;
        if (wPCBIndex == ptScheTCB->wBlockHead)
        {
            ptScheTCB->wBlockHead = wNext;
        }
    }
    /*½«¸ÃPCBÖØÉèÎªÔ­Ê¼Ì¬£¬µÈ´ýµ÷¶È*/
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

    /*Ë÷ÒýÓ¦¸ÃÐ¡ÓÚµ¥°åµÄPCB³Ø×ÜÊý*/
    if (wPCBIndex >= M_ptCoreData->wPCBCounts)
    {
        printf("Invalid PCBIndex in PutPCBToBlockQueueTail()\n\r");
        return ERROR_UNKNOWN;
    }

    /*¸ù¾ÝË÷ÒýÈ¡µÃÄ¿±êPCBÖ¸Õë*/
    ptDestPCB    = M_atPCB + wPCBIndex;

    if ((WEOF != ptDestPCB->wPrevByStatus) || (WEOF != ptDestPCB->wNextByStatus))
    {
        printf("Put the invalid PCB into block queue in PutPCBToBlockQueueTail()\n\r");
        return ERROR_UNKNOWN;
    }

    /*È¡µÃÄ¿µÄPCBËùÊôµÄµ÷¶ÈÈÎÎñT_ScheTCBÖ¸Õë*/
    ptScheTCB    = M_atScheTCB + ptDestPCB->wTno;

    /*»ñÈ¡µ÷¶ÈÈÎÎñµÄ×èÈû¶ÓÁÐµÄÍ·PCB*/
    wHead        = ptScheTCB->wBlockHead;
    ptHeadPCB    = M_atPCB + wHead;

    if (WEOF == wHead)
    {
        /*×èÈû¶ÓÁÐÎª¿Õ£¬ÔòÓû¼ÓÈëµÄPCB¼´Èë×èÈû¶ÓÁÐÍ·*/
        ptScheTCB->wBlockHead     = wPCBIndex;
        ptDestPCB->wPrevByStatus  = wPCBIndex;
        ptDestPCB->wNextByStatus  = wPCBIndex;
    }
    else
    {
        /*»ñÈ¡×èÈû¶ÓÁÐµÄÎ²PCBË÷Òý*/
        wTail = ptHeadPCB->wPrevByStatus;

        /*½«Óû¼ÓÈëPCB·ÅÔÚµ±Ç°Î²PCBÖ®ºó*/
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

    /*È¡µÃµ÷¶ÈÈÎÎñ¾ÍÐ÷¶ÓÁÐµÄÍ·PCBË÷Òý*/
    wHead    = ptScheTCB->wReadyHead;

    if (WEOF == wHead)
    {
        printf("not found ready pcb head in \n\r");
        return 0xffff;
    }

    /*È¡µÃÍ·PCB*/
    ptHeadPCB    = M_atPCB + wHead;

    //printf("start Run Proc=0x%x,TCB:%s\n",ptHeadPCB->wProcType,ptScheTCB->acName);

    /*ÕÒµ½¾ÍÐ÷¶ÓÁÐÍ·µÄÇ°Ò»¸öPCBºÍºóÒ»¸öPCBË÷Òý*/
    wPrev        = ptHeadPCB->wPrevByStatus;
    wNext        = ptHeadPCB->wNextByStatus;

    if ((wPrev == wHead) && (wNext == wHead))
    {
        /*Èç¹û¾ÍÐ÷¶ÓÁÐÖ»ÓÐÒ»¸öPCB*/
        ptScheTCB->wReadyHead = 0xffff;
    }
    else
    {
        M_atPCB[wPrev].wNextByStatus   = wNext;
        M_atPCB[wNext].wPrevByStatus   = wPrev;

        /*ÏÂÒ»¸öPCB×÷Îª¾ÍÐ÷¶ÓÁÐµÄÍ·PCB*/
        ptScheTCB->wReadyHead          = wNext;
    }
    /*±»Õª³öÀ´ºóÐü¿Õ´¦Àí*/
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
    /*¸ù¾Ý²ÎÊý»ñµÃµ÷¶ÈÈÎÎñµÄTCBÖ¸Õë*/
    ptSelfTCB               = M_atScheTCB + dwTno;
    ptSelfTCB->dwScheCounts = 0;
    
    printf("\n [%s:%d]ScheTaskEntry Tno = %d\n",  __FUNCTION__, __LINE__,dwTno);

    for(;;)
    {
        /*ÈÎÎñËÀÑ­»·¿ªÊ¼*/
        while (0 < ptSelfTCB->wReadyCounts)
        {
            /*ÔÚÈÎÎñµÄ¾ÍÐ÷¶ÓÁÐÖÐÈ¡³öÍ·PCB*/
            wPCBIndex = GetPCBFromReadyQueueHead(ptSelfTCB);

            /*ÉèÖÃµ÷¶ÈÈÎÎñµ±Ç°ÔËÐÐµÄ½ø³ÌË÷Òý*/
            ptSelfTCB->wRunning = wPCBIndex;

            /*¸ù¾ÝË÷ÒýÕÒµ½Ä¿µÄPCB*/
            ptDestPCB           = M_atPCB + wPCBIndex;

            ptDestPCB->ucRunStatus = PROC_STATUS_RUNNING;/*ÉèÔËÐÐ×´Ì¬ÎªÕýÔÚÔËÐÐ*/

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
            
            /*×èÈûÔ­ÒòÊÇÍ¬²½µ÷ÓÃ×èÈû£¬Ôò°ÑPCB·ÅÈë×èÈû¶ÓÁÐÎ²*/
            if (WAIT_MSG != ptDestPCB->ucBlockReason)
            {
                PutPCBToBlockQueueTail(wPCBIndex);
            }
            else
            {
                /* ´ËÊ±PCB²ÅÊÇÕæÕýµØÔËÐÐÍê±Ï µ±Ç°ÏûÏ¢ÎÞÓÃ£¬¿ÉÒÔÊÍ·Åµô */
                FreeCurMsg(ptDestPCB);

                /*¸üÐÂÒµÎñ×´Ì¬*/
                ptDestPCB->wState     = ptDestPCB->wRetState;
                if (NULL == ptDestPCB->ptCurMsg)
                {
                    /*Èç¹û½ø³ÌµÄµ±Ç°ÏûÏ¢Îª¿Õ£¬ÔòÖÃÈë×èÈû¶ÓÁÐÎ²£¬²¢ÉèÖÃ×èÈûÔ­ÒòÎªÎÞÏûÏ¢*/
                    PutPCBToBlockQueueTail(wPCBIndex);
                    ptDestPCB->ucBlockReason = WAIT_MSG;
                }
                else
                {
                    /*ÓÐµ±Ç°ÏûÏ¢£¬Ôò½«½ø³Ì·ÅÈë¾ÍÐ÷¶ÓÁÐÎ²*/
                    PutPCBToReadyQueueTail(wPCBIndex);
                }
            }
            
            sleep(1);
        }
        printf("ready queue is empty\n");
        
        /*ÔÚÈÎÎñµÄÓÊÏäÀïÈ¡³öÒ»ÌõÏûÏ¢Ö¸Õë£¬ÎÞÏûÏ¢×èÈûÈÎÎñ*/
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

        ptDestPCB    = M_atPCB + wIndex;/*Ä¿µÄPCBÖ¸Õë*/

        /*ÎªÄ¿µÄPCB¶ÑÕ»Ìá½»ÄÚ´æ£¬´óÐ¡ÓÉËùÊô½ø³ÌÀàÐÍÖ¸¶¨*/
        ptDestPCB->pucStack = malloc(ptCurType->dwStackSize);

        if (NULL == ptDestPCB->pucStack)
        {
            printf("[%s:%d]Alloc Proc Stack ERROR in InitPCBPool()\n" , __FUNCTION__, __LINE__);
            return   ERROR_UNKNOWN;
        }
        
        /* ½ø³ÌÕ»ÐèÒª±£Áô16¸ö×Ö½Ú */
        ptDestPCB->dwStackSize      = ptCurType->dwStackSize - 16;

        /*ÉèÖÃ½ø³ÌµÄË½ÓÐÊý¾ÝÇø´óÐ¡*/
        ptDestPCB->dwDataRegionSize = ptCurType->dwDataRegionSize;

        /*³õÊ¼»¯¸Ã½ø³Ì¿ØÖÆ¿éµÄ³ÉÔ±±äÁ¿*/
        ptDestPCB->ucIsUse           = YES;
        ptDestPCB->wInstanceNo   = 1;
        ptDestPCB->ucBlockReason       = WAIT_MSG;
        ptDestPCB->wProcType     = ptCurType->wProcType;
        ptDestPCB->wTno              = ptCurType->wTno;
        ptDestPCB->pEntry             = ptCurType->pEntry;
        ptDestPCB->wState              = S_StartUp;
        ptDestPCB->wRetState         = S_StartUp;
        ptDestPCB->pucDataRegion = NULL;/*ÏÈÉè³É¿Õ*/
        /* ½ø³ÌÕ»Ö¸Õë³õÊ¼Ö¸ÏòÕ»µ× */
        ptDestPCB->dwProcSP            = (ULONG)ptDestPCB->pucStack + ptDestPCB->dwStackSize;  
        ptDestPCB->dwTaskSP            = 0;
        ptDestPCB->wPrevByStatus     = WEOF;
        ptDestPCB->wNextByStatus     = WEOF;
        /*³õÊ¼»¯PCBÏûÏ¢Á´±í*/
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

    /*ÒÔÏÂforÑ­»·±éÀúµ÷¶ÈÈÎÎñ£¬²¢³õÊ¼»¯µ÷¶ÈÈÎÎñTCBµÄ³ÉÔ±±äÁ¿ */
    for (wIndex = 0; wIndex <SCHE_TASK_NUM; wIndex ++)
    {
        /*»ñµÃµ÷¶ÈÈÎÎñ¿ØÖÆ¿éÖ¸Õë*/
        ptScheTCB    = M_atScheTCB + wIndex;

        /*½«µ÷¶ÈÈÎÎñÅäÖÃ±íÖÐµÄÅäÖÃÐÅÏ¢¿½±´½øÈÎÎñ¿ØÖÆ¿é*/
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


