
#include "sche.h"
#include "comm.h"
#include "config.h"
#include <mqueue.h>
#include <errno.h>

T_ScheTCB * Vos_GetCurTaskSchTCB(WORD32 * pdwTno)
{
    WORD32 dwTno;
    T_PCB *   ptSelfPCB;

    ptSelfPCB = R_GetSelfPCBPtr();
    dwTno = ptSelfPCB->wTno;
    
    if((dwTno >= SCHE_TASK_NUM) || (WEOF == dwTno))
    {
        return NULL;
    }

    *pdwTno = dwTno;
    return &M_atScheTCB[dwTno];
}

WORD32 Vos_GetQueueCurrentCount(WORD32 dwQueueId)
{
    struct mq_attr attr;

    if (MAX_QUEUE_COUNT <= dwQueueId)
    {
        /* ¶ÓÁÐ¿ØÖÆ¿éID²»ºÏ·¨ */
        printf("[%s:%d] Value of QueueID is invalid!\n" , __FUNCTION__, __LINE__);
        return -1;
    }
    
    if (-1 == mq_getattr(s_atQueueCtl[dwQueueId].dwQueueId, &attr))
    {
        printf("[%s:%d] Fail to get message queue info of Linux!\n" , __FUNCTION__, __LINE__);
        return -1;
    }
    return attr.mq_curmsgs;
}

SWORD32 VOS_MsgQReceive(WORD32 dwOSMsgQID, BYTE *pMsg, WORD32 dwMsgLen, WORD32 dwMsgPrio)
{
    SWORD32 sdwRevLen;
    
    do
    {
        sdwRevLen = mq_receive(dwOSMsgQID, (CHAR*)pMsg, dwMsgLen, &dwMsgPrio);
    }while((-1 == sdwRevLen) && ((errno == EINTR) || (errno == 512)));  

    return sdwRevLen;
}

SWORD32 VOS_TimedMsgQSend(WORD32 dwOSMsgQID, BYTE *pMsg, WORD32 dwMsgLen, WORD32 dwMsgPrio, WORD32 dwTimeOut)
{
    struct timespec  sTimeSpec;
    
    if (NO_WAIT == dwTimeOut)
    {
        sTimeSpec.tv_sec = -1;
        sTimeSpec.tv_nsec = -1;
    }
    else
    {
        /* »ñÈ¡¾ø¶ÔÊ±¼ä */
        clock_gettime(0, &sTimeSpec);
        sTimeSpec.tv_sec  += dwTimeOut/1000;
        sTimeSpec.tv_sec += ((dwTimeOut%1000)*1000*1000 + sTimeSpec.tv_nsec)/(1000*1000*1000);
        sTimeSpec.tv_nsec = ((dwTimeOut%1000)*1000*1000 + sTimeSpec.tv_nsec)%(1000*1000*1000);
    }
    return mq_timedsend(dwOSMsgQID, (CHAR*)pMsg, dwMsgLen, dwMsgPrio, &sTimeSpec);
}

WORD16  R_GetPCBIndexByPNO(WORD16 wPno)
{
    BYTE   index;

    for(index=0;index<GetAllProcNum();index++)
    {
        if(wPno == gaProcCfgTbl[index].wProcType)
            return index;
    }
    return WEOF;
}

T_PCB  *R_GetPCBPtrByPNO(WORD16 wPno)
{
    BYTE   index = R_GetPCBIndexByPNO(wPno);
    
    return M_atPCB + index;
}

VOID AddMsgToProcQueue(T_PCB *ptPCB, T_MSG *ptMsg)
{
    if((NULL == ptPCB->ptFirstMsg )&&(NULL == ptPCB->ptLastMsg))
    {
        /* ½ø³ÌÏûÏ¢¶ÓÁÐÎª¿Õ */
        ptPCB->ptFirstMsg = ptMsg;
        ptPCB->ptLastMsg  = ptMsg;
        ptPCB->dwMsgQCounts = 0;
    }
    else
    {
        /* ½ø³ÌÏûÏ¢¶ÓÁÐ·Ç¿Õ */
        if (COMM_ASYN_URGENT != ptMsg->ucMsgType)
        {
            /*·Ç½ô¼±ÏûÏ¢Ö±½Ó·Å¶ÓÎ²*/
            ptPCB->ptLastMsg->ptNextMsg  = ptMsg;
            ptPCB->ptLastMsg  = ptMsg;
        }
        else
        {   
            if (ptMsg->tSender.wPno == ptMsg->tReceiver.wPno)
            {
                /* Í¬Ò»½ø³Ì·¢ËÍµÄ½ô¼±Òì²½ÏûÏ¢£¬·ÅÔÚµ±Ç°ÏûÏ¢Ö®ºó */
                if (ptPCB->ptCurMsg != NULL)
                {
                    ptMsg->ptNextMsg = ptPCB->ptCurMsg->ptNextMsg;
                    ptPCB->ptCurMsg->ptNextMsg = ptMsg;
                    if (ptPCB->ptCurMsg == ptPCB->ptLastMsg)
                    {
                        ptPCB->ptLastMsg = ptMsg;
                    }
                }
                else
                {
                    /* ²»Ó¦¸Ã×ßÈë¸Ã·ÖÖ§ */
                    ptPCB->ptLastMsg->ptNextMsg  = ptMsg;
                    ptPCB->ptLastMsg  = ptMsg;
                }
            }
            else
            {
                if (  (ptPCB->ucBlockReason == WAIT_SYNCMSG)
                        || (ptPCB->ucBlockReason == WAIT_SYNCTIMER)
                        || (ptPCB->ucBlockReason == WAIT_REMOTECALL))
                {
                    /* ½ÓÊÕ·½ÕýÔÚ·¢ÉúÍ¬²½²Ù×÷£¬Ôò½«ÏûÏ¢·ÅÔÚµ±Ç°ÏûÏ¢ºó */
                    ptMsg->ptNextMsg = ptPCB->ptCurMsg->ptNextMsg;
                    ptPCB->ptCurMsg->ptNextMsg = ptMsg;
                    if (ptPCB->ptCurMsg == ptPCB->ptLastMsg)
                    {
                        ptPCB->ptLastMsg = ptMsg;
                    }
                }
                else
                {
                    if (NULL == ptPCB->ptPrevMsg)
                    {
                        /* ½«¼±ÆÈÏûÏ¢·ÅÔÚ×îÇ°Ãæ */
                        ptMsg->ptNextMsg = ptPCB->ptFirstMsg;
                        ptPCB->ptFirstMsg  = ptMsg;
                        ptPCB->ptCurMsg   = ptMsg;
                    }
                }
            }
        }
    }

    if (NULL == ptPCB->ptCurMsg)
    {
        ptPCB->ptCurMsg   = ptMsg;
    }
    ptPCB->dwMsgQCounts++;
}

int DispatchMsg(T_MSG *ptTaskMsg, WORD16 wTno)
{
    WORD16            wPCBIndex;
    T_PCB             *ptDestPCB;
    T_ScheTCB  *ptSelfTCB;

    ptSelfTCB = M_atScheTCB + wTno;

    /*¸ù¾Ý´ýÅÉ·¢ÏûÏ¢µÄ½ÓÊÕ½ø³ÌºÅÈ¡µÃ¸Ã½ÓÊÕ½ø³ÌµÄPCBË÷Òý*/
    wPCBIndex    = R_GetPCBIndexByPNO(ptTaskMsg->tReceiver.wPno);

    if (WEOF == wPCBIndex)
    {
        /* Ä¿µÄ½ø³ÌPCBË÷ÒýÎÞÐ§ ÊÍ·ÅµôÎÞ½ÓÊÜÕßµÄÏûÏ¢*/
        printf("Dest proc 0x:%x is Invalid in DispatchMsg()\n",ptTaskMsg->tReceiver.wPno);
        free(ptTaskMsg);
        return ERROR_UNKNOWN;
    }

    /*»ñÈ¡Ä¿µÄ½ø³ÌµÄPCBÖ¸Õë*/
    ptDestPCB    = M_atPCB + wPCBIndex;

    if (NO == ptDestPCB->ucIsUse)
    {
        /* Ä¿µÄ½ø³Ì²»ÊÇÒ»¸ö»î¶¯½ø³Ì */
        printf("Dest proc(0x%x) is not in use in DispatchMsg()\n",ptTaskMsg->tReceiver.wPno);
        free(ptTaskMsg);
        return ERROR_UNKNOWN;
    }

    if(wTno != ptDestPCB->wTno)
    {
        printf("[%s:%d]\nwTno=%d,ptDestPCB->wTno %d,\n", __FUNCTION__, __LINE__,wTno,ptDestPCB->wTno);
    }

    /*¸ù¾Ý´ýÅÉ·¢µÄÏûÏ¢ÏûÏ¢ÀàÐÍ½øÐÐÅÉ·¢ÅÐ¶Ï*/
    switch (ptTaskMsg->ucMsgType)
    {
    case COMM_TIMER_OUT   :
    case COMM_ASYN_NORMAL :
    case COMM_ASYN_URGENT :
    case COMM_CONTROL_MSG :
    case COMM_SYNC_MSG :
    case COMM_SYNC_PROCESS:
    case COMM_SYN_URGENT:
        AddMsgToProcQueue(ptDestPCB, ptTaskMsg);
        if (PROC_STATUS_BLOCK == ptDestPCB->ucRunStatus)
        {
            /* °Ñ×èÈûÔ­ÒòÎªµÈ´ýÏûÏ¢µÄ½ø³Ì´ÓBlock¶ÓÁÐÖÐÈ¡³ö£¬·ÅÈëReady¶ÓÁÐÎ² */
            if (WAIT_MSG == ptDestPCB->ucBlockReason)
            {
                ReleasePCBFromBlockQueue(wPCBIndex);
                PutPCBToReadyQueueTail(wPCBIndex);
            }
            /*×èÈûÔ­ÒòÎªÆäËüµÄ²»ÓÃ»½ÐÑ*/
        }
        return  SUCCESS;

    default:
        printf("Invalid msg type in DispatchMsg()\n\r");
        /*ÏûÏ¢ÎªÎÞÐ§ÏûÏ¢£¬ÊÍ·Åµô·µ»Ø*/
        free(ptTaskMsg);
        return ERROR_UNKNOWN;
    }
    return SUCCESS;
}

void DispatchMsgListToPCB(T_MSG* ptMsgList,WORD16 dwTno)
{
    T_MSG* ptNextMsg = NULL;
    
    while (NULL != ptMsgList)
    {
        ptNextMsg = ptMsgList->ptNextMsg;
        ptMsgList->ptNextMsg = NULL;
        DispatchMsg(ptMsgList, dwTno);
        ptMsgList = ptNextMsg;
    }
}

SWORD32 Vos_PopQueue(WORD32 dwQueueId,WORD32 dwTimeout, BYTE   *pucDataBuf,WORD32 dwLen)
{
    T_QueueCtl  *ptQueueCtl;
    SWORD32     sdwRevLen;
    struct timespec timeout;

    if (MAX_QUEUE_COUNT <= dwQueueId)
    {
        /* ¶ÓÁÐ¿ØÖÆ¿éID²»ºÏ·¨ */
        printf("[%s:%d]Value of QueueID is invalid!\n" , __FUNCTION__, __LINE__);
        return VOS_QUEUE_ERROR;
    }
    ptQueueCtl = &s_atQueueCtl[dwQueueId];
    if (VOS_QUEUE_CTL_USED != ptQueueCtl->ucUsed)
    {
        printf("[%s:%d]Value of QueueID is invalid!\n" , __FUNCTION__, __LINE__);
        return VOS_QUEUE_ERROR;
    }

    if (dwTimeout == (WORD32)WAIT_FOREVER)
    {
        do
        {
            sdwRevLen = VOS_MsgQReceive(ptQueueCtl->dwQueueId, (BYTE*)pucDataBuf, dwLen, 0);
        }while((-1 == sdwRevLen) && ((errno == EINTR) || (errno == 512) || (errno == EAGAIN)));
        
        if (-1 == sdwRevLen)
        {
            printf("[%s:%d]Fail to receive message queue of Linux, error %d %s \n", __FUNCTION__, __LINE__,errno,strerror(errno));
            return VOS_QUEUE_ERROR;
        }
    }
    else
    {
        /* »ñÈ¡¾ø¶ÔÊ±¼ä */
        clock_gettime(0, &timeout);
        timeout.tv_sec  += dwTimeout/1000;
        timeout.tv_sec += ((dwTimeout%1000)*1000*1000 + timeout.tv_nsec)/(1000*1000*1000);
        timeout.tv_nsec = ((dwTimeout%1000)*1000*1000 + timeout.tv_nsec)%(1000*1000*1000);

        sdwRevLen = mq_timedreceive(ptQueueCtl->dwQueueId, (CHAR*)pucDataBuf, dwLen, NULL, &timeout);
        if (-1 == sdwRevLen)
        {
            if (110 != errno)
            {
                printf("[%s:%d]Fail to receive message queue of Linux, error %d\n", __FUNCTION__, __LINE__,errno);
            }
            return VOS_QUEUE_ERROR;
        }
    }
    return sdwRevLen;
}
BOOLEAN Vos_AppendQueue(WORD32 dwQueueId,BYTE   *pucDataBuf, WORD32 dwLen,BYTE  ucIfUrgent,WORD32 dwTimeOut)
{
    T_QueueCtl *ptQueueCtl;
    T_ScheTCB * ptCurSchTCB;
    WORD32 dwTno;
    BYTE            msg_prio = 0;
    struct timespec timeout;

    /* ºÏ·¨ÐÔ¼ì²é */
    if (MAX_QUEUE_COUNT <= dwQueueId)
    {
        /* ¶ÓÁÐ¿ØÖÆ¿éID²»ºÏ·¨ */
        printf("[%s:%d] Value of QueueID:%d is invalid!\n" , __FUNCTION__, __LINE__,dwQueueId);
        return FALSE;
    }
    
    ptQueueCtl = &s_atQueueCtl[dwQueueId];
    
    if (VOS_QUEUE_CTL_USED != ptQueueCtl->ucUsed)
    {
        printf("[%s:%d] ptQueueCtl of QueueID:%d is invalid!\n" , __FUNCTION__, __LINE__,dwQueueId);
        return FALSE;
    }

    if (VOS_URGENT_MSG == ucIfUrgent)
    {
        msg_prio = 1;
    }

    if (dwTimeOut == (WORD32)WAIT_FOREVER)
    {
        ptCurSchTCB = Vos_GetCurTaskSchTCB(&dwTno);

        if ((NULL != ptCurSchTCB) && (dwQueueId == ptCurSchTCB->dwMailBoxId))
        {
            /*Ó¦¸Ã²»»á×ßÈë¸ÃÁ÷³Ì*/
            printf("[%s:%d] Msg To self Task\n",__FUNCTION__, __LINE__);
        }
        else /* ²»ÊÇ×Ô¼º¸ø×Ô¼º·¢ */
        {
            if (-1 == mq_send(ptQueueCtl->dwQueueId, (CHAR *)pucDataBuf, dwLen, msg_prio))
            {
                printf("[%s:%d]Fail to send message queue of Linux, error %d (%s), msgQueueId:%d, QueueId:%d, name:%s\n",
                    __FUNCTION__, __LINE__, errno,strerror(errno),
                    dwQueueId, ptQueueCtl->dwQueueId, ptQueueCtl->aucQueueName);
                return FALSE;
            }
            else
            {
                return TRUE;
            }
        }
    }
    else
    {
        if (-1 == VOS_TimedMsgQSend(ptQueueCtl->dwQueueId, pucDataBuf, dwLen, msg_prio, dwTimeOut))
        {    
            printf("[%s:%d]Fail to send message queue of Linux, error %d (%s), msgQueueId:%d, QueueId:%d, name:%s\n",
                __FUNCTION__, __LINE__, errno,strerror(errno),
                dwQueueId, ptQueueCtl->dwQueueId, ptQueueCtl->aucQueueName);
            return FALSE;
        }
    }
    return TRUE;
}

BOOLEAN SendInCPU(BYTE *pucMsgPtr,WORD32 dwParam)
{
    T_MSG       *ptMsg = NULL;
    T_PCB       *ptSendPCB;
    T_PCB       *ptRecvPCB;
    BYTE        ucUrgent;
    BOOLEAN     bResult = TRUE;
    WORD32 dwRcvMbx;
    WORD32 dwtimeout;

    ptMsg = (T_MSG *)pucMsgPtr;

    ptMsg->ptNextMsg = NULL;

    ptSendPCB = R_GetSelfPCBPtr();
    ptRecvPCB = R_GetPCBPtrByPNO(ptMsg->tReceiver.wPno);		

    if((NULL == ptRecvPCB) || (NO == ptRecvPCB->ucIsUse))
    {
        free(ptMsg);
        return FALSE;
    }
    dwRcvMbx = M_atScheTCB[ptRecvPCB->wTno].dwMailBoxId;

    if (ptSendPCB && ptRecvPCB && (ptSendPCB->wTno == ptRecvPCB->wTno))
    {
        /*±¾ÈÎÎñÄÚPCBÍ¨ÐÅ*/
        DispatchMsg(ptMsg, ptRecvPCB->wTno);
    }
    else
    {   /* ÈÎÎñ¼äÍ¨Ñ¶ */
        ucUrgent = (COMM_ASYN_URGENT == ptMsg->ucMsgType || \
                 COMM_SYN_URGENT == ptMsg->ucMsgType) ?  \
                   VOS_URGENT_MSG:VOS_NOT_URGENT_MSG;

        /* ½«ÏûÏ¢·¢ËÍµ½ÏàÓ¦µ÷¶ÈÈÎÎñµÄÓÊÏäÖÐ */
        if(COMM_UNRELIABLE == dwParam)
            dwtimeout = NO_WAIT;
        else
            dwtimeout = WAIT_FOREVER;

	    /*·¢ËÍÊ§°Ü£¬É¾³ýÏûÏ¢*/
        if (FALSE == Vos_AppendQueue(dwRcvMbx,(BYTE *)&ptMsg, sizeof(ptMsg), ucUrgent, dwtimeout))
        {
            free(ptMsg);
            return FALSE;
        }
    }
    return bResult;
}

void FreeCurMsg(T_PCB *ptPCB)
{
    T_MSG   *ptPrevMsg;
    T_MSG   *ptNextMsg;
    T_MSG   *ptCurMsg;
    static INT    count =0;

    ptCurMsg     = ptPCB->ptCurMsg;

    if (NULL == ptCurMsg)
    {
        printf("no signal can be free in FreeCurMsg()\n\r");
        return;
    }

    ptPrevMsg    = ptPCB->ptPrevMsg;
    ptNextMsg    = ptCurMsg->ptNextMsg;

    if (ptCurMsg == ptPCB->ptFirstMsg)
    {
        /* µ±Ç°ÏûÏ¢µÄÇ°Ò»ÏûÏ¢Îª¿Õ£¬´ÓÏûÏ¢¶ÓÁÐÍ·²¿ÊÍ·Åµ±Ç°ÏûÏ¢ */
        ptPCB->ptFirstMsg = ptNextMsg;
    }
    else
    {
        if(ptPrevMsg != NULL)
        {
            ptPrevMsg->ptNextMsg  = ptNextMsg;
        }
    }

    if (ptCurMsg == ptPCB->ptLastMsg)
    {
        /* ´ÓÏûÏ¢¶ÓÁÐÎ²ÊÍ·Åµ±Ç°ÏûÏ¢ */
        ptPCB->ptLastMsg  = ptPrevMsg;
    }

    /*É¾³ýÏûÏ¢£¬¹é»¹ÏµÍ³UB*/
    free(ptCurMsg);
    ptPCB->dwMsgQCounts--;

    /*Èç¹ûµ±Ç°ÒµÎñ×´Ì¬Óë·µ»ØÒµÎñ×´Ì¬²»µÈ£¬ÔòµÚÒ»ÌõÏûÏ¢Îªµ±Ç°ÏûÏ¢*/
    if ((ptPCB->wRetState!= ptPCB->wState) &&(WEOF != ptPCB->wRetState))
    {
        ptPCB->ptCurMsg   = ptPCB->ptFirstMsg;
        ptPCB->ptPrevMsg  = NULL;
        ptPCB->wState     = ptPCB->wRetState;
    }
    else
    {
        ptPCB->ptCurMsg   = ptNextMsg;
    }

    if(ptPCB->ptLastMsg != NULL)
    {
        if(ptPCB->ptLastMsg->ptNextMsg !=NULL )
        {
           ptPCB->ptLastMsg->ptNextMsg = NULL;
        }
    }

    if(ptPCB->ptFirstMsg != NULL)
    {
        if(ptPCB->ptFirstMsg->ptNextMsg != NULL)
        {
            if(ptPCB->ptFirstMsg->ptNextMsg == ptPCB->ptFirstMsg)
            {
                ptPCB->ptFirstMsg->ptNextMsg =NULL;
                if((ptPCB->ptPrevMsg == NULL)||(ptPCB->ptFirstMsg == ptPCB->ptPrevMsg))
                {
                    ptPCB->dwMsgQCounts = 1;
                }
            }
        }
    }
  
    if(ptPCB->dwMsgQCounts > 100)
    {
        /* ÏûÏ¢»ýÑ¹ÌáÊ¾ÐÅÏ¢´òÓ¡ Ã¿100´Î´òÓ¡Ò»Ìõ */
        count++;
        if (100 == count)
        {
            printf(" [%s:%d]\n ptPCB->ptCurMsg: 0x%x ptCurMsg :0x%x", __FUNCTION__, __LINE__,ptPCB->ptCurMsg,ptCurMsg);
            count = 0;
        }
    }
}

T_MSG *R_CreateMsg(WORD32 dwMsgLen)
{
    BYTE*  ptMsg = NULL;
    
    ptMsg = malloc(dwMsgLen+sizeof(T_MSG));
    memset(ptMsg,0,dwMsgLen+sizeof(T_MSG));
    
    return (T_MSG*)ptMsg;
}

int R_SendMsg(WORD16  wMsgId,VOID *pData,WORD32 dwMsgLen,BYTE ucMsgType,PID *ptReceiver)
{
    T_MSG  *ptMsg = NULL;
    T_PCB *   ptSelfPCB = NULL;

    if(NULL == pData)
        dwMsgLen = 0;

    ptSelfPCB = R_GetSelfPCBPtr();
    ptMsg = R_CreateMsg(dwMsgLen);
    /*Ìî³äT_MSGµÄ»ù±¾ÐÅÏ¢*/
    ptMsg->wMsgId = wMsgId;
    ptMsg->ucMsgType = ucMsgType;
    ptMsg->ucIsSave = COMM_NOT_SAVED;
    ptMsg->ptNextMsg = NULL;
    memcpy(&ptMsg->tReceiver,ptReceiver,sizeof(PID));
    ptMsg->tSender.wPno = ptSelfPCB->wProcType;
    
    if (pData != NULL)
        memcpy((BYTE *)(ptMsg + 1), pData,dwMsgLen);  /*½«Ìî³äÄÚÈÝ¿½ÈëÏûÏ¢Ìå*/
    
    /* ·¢ËÍÏûÏ¢ */
    if (FALSE == SendInCPU(ptMsg,COMM_RELIABLE))
    {
        printf("[%s:%d]\n send msg=%d fail,Recv=%d", __FUNCTION__, __LINE__,wMsgId,ptReceiver->wPno);
        /*·¢ËÍÊ§°Ü*/
        return ERR_COMM_SEND_ASYN_MSG_FAILED;
    }

    return SUCCESS;
}
int R_SendMsgFromTask(WORD16  wMsgId,VOID *pData,WORD32 dwMsgLen,BYTE ucMsgType,PID *ptReceiver)
{
    T_MSG  *ptMsg = NULL;
    T_PCB*    ptDestPCB = NULL;
  
    if(NULL == pData)
        dwMsgLen = 0;

    ptDestPCB = R_GetPCBPtrByPNO(ptReceiver->wPno);
    ptMsg = R_CreateMsg(dwMsgLen);
    /*Ìî³äT_MSGµÄ»ù±¾ÐÅÏ¢*/
    ptMsg->wMsgId = wMsgId;
    ptMsg->ucMsgType = ucMsgType;
    ptMsg->ucIsSave = COMM_NOT_SAVED;
    ptMsg->ptNextMsg = NULL;
    memcpy(&ptMsg->tReceiver,ptReceiver,sizeof(PID));
    
    if (pData != NULL)
        memcpy((BYTE *)(ptMsg + 1), pData,dwMsgLen);  /*½«Ìî³äÄÚÈÝ¿½ÈëÏûÏ¢Ìå*/

    
    if (M_atScheTCB[ptDestPCB->wTno].dwTaskId == GetCurTaskID() ||
        M_atScheTCB[ptDestPCB->wTno].dwMailBoxId == VOS_CREATE_QUEUE_FAILURE)
    {
        DispatchMsg(ptMsg, ptDestPCB->wTno);
    }
    else
    {
        /*½«ÏûÏ¢Ö¸Õë·Åµ½½ø³ÌËùÊôµ÷¶ÈÈÎÎñµÄÓÊÏä*/        
        if (TRUE != Vos_AppendQueue(M_atScheTCB[ptDestPCB->wTno].dwMailBoxId, (BYTE *)&ptMsg, sizeof(VOID *), VOS_NOT_URGENT_MSG, (WORD32)WAIT_FOREVER))
        {
            printf("[%s:%d]\n send msg=%d fail,Recv=0x%x\n", __FUNCTION__, __LINE__,wMsgId,ptReceiver->wPno);
            return ERR_COMM_SEND_ASYN_MSG_FAILED;
        }
    }

    return SUCCESS;
}

