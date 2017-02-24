
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
        /* 队列控制块ID不合法 */
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
        /* 获取绝对时间 */
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
        /* 进程消息队列为空 */
        ptPCB->ptFirstMsg = ptMsg;
        ptPCB->ptLastMsg  = ptMsg;
        ptPCB->dwMsgQCounts = 0;
    }
    else
    {
        /* 进程消息队列非空 */
        if (COMM_ASYN_URGENT != ptMsg->ucMsgType)
        {
            /*非紧急消息直接放队尾*/
            ptPCB->ptLastMsg->ptNextMsg  = ptMsg;
            ptPCB->ptLastMsg  = ptMsg;
        }
        else
        {   
            if (ptMsg->tSender.wPno == ptMsg->tReceiver.wPno)
            {
                /* 同一进程发送的紧急异步消息，放在当前消息之后 */
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
                    /* 不应该走入该分支 */
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
                    /* 接收方正在发生同步操作，则将消息放在当前消息后 */
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
                        /* 将急迫消息放在最前面 */
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

    /*根据待派发消息的接收进程号取得该接收进程的PCB索引*/
    wPCBIndex    = R_GetPCBIndexByPNO(ptTaskMsg->tReceiver.wPno);

    if (WEOF == wPCBIndex)
    {
        /* 目的进程PCB索引无效 释放掉无接受者的消息*/
        printf("Dest proc 0x:%x is Invalid in DispatchMsg()\n",ptTaskMsg->tReceiver.wPno);
        free(ptTaskMsg);
        return ERROR_UNKNOWN;
    }

    /*获取目的进程的PCB指针*/
    ptDestPCB    = M_atPCB + wPCBIndex;

    if (NO == ptDestPCB->ucIsUse)
    {
        /* 目的进程不是一个活动进程 */
        printf("Dest proc(0x%x) is not in use in DispatchMsg()\n",ptTaskMsg->tReceiver.wPno);
        free(ptTaskMsg);
        return ERROR_UNKNOWN;
    }

    if(wTno != ptDestPCB->wTno)
    {
        printf("[%s:%d]\nwTno=%d,ptDestPCB->wTno %d,\n", __FUNCTION__, __LINE__,wTno,ptDestPCB->wTno);
    }

    /*根据待派发的消息消息类型进行派发判断*/
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
            /* 把阻塞原因为等待消息的进程从Block队列中取出，放入Ready队列尾 */
            if (WAIT_MSG == ptDestPCB->ucBlockReason)
            {
                ReleasePCBFromBlockQueue(wPCBIndex);
                PutPCBToReadyQueueTail(wPCBIndex);
            }
            /*阻塞原因为其它的不用唤醒*/
        }
        return  SUCCESS;

    default:
        printf("Invalid msg type in DispatchMsg()\n\r");
        /*消息为无效消息，释放掉返回*/
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
        /* 队列控制块ID不合法 */
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
        /* 获取绝对时间 */
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

    /* 合法性检查 */
    if (MAX_QUEUE_COUNT <= dwQueueId)
    {
        /* 队列控制块ID不合法 */
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
            /*应该不会走入该流程*/
            printf("[%s:%d] Msg To self Task\n",__FUNCTION__, __LINE__);
        }
        else /* 不是自己给自己发 */
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
        /*本任务内PCB通信*/
        DispatchMsg(ptMsg, ptRecvPCB->wTno);
    }
    else
    {   /* 任务间通讯 */
        ucUrgent = (COMM_ASYN_URGENT == ptMsg->ucMsgType || \
                 COMM_SYN_URGENT == ptMsg->ucMsgType) ?  \
                   VOS_URGENT_MSG:VOS_NOT_URGENT_MSG;

        /* 将消息发送到相应调度任务的邮箱中 */
        if(COMM_UNRELIABLE == dwParam)
            dwtimeout = NO_WAIT;
        else
            dwtimeout = WAIT_FOREVER;

	    /*发送失败，删除消息*/
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
        /* 当前消息的前一消息为空，从消息队列头部释放当前消息 */
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
        /* 从消息队列尾释放当前消息 */
        ptPCB->ptLastMsg  = ptPrevMsg;
    }

    /*删除消息，归还系统UB*/
    free(ptCurMsg);
    ptPCB->dwMsgQCounts--;

    /*如果当前业务状态与返回业务状态不等，则第一条消息为当前消息*/
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
        /* 消息积压提示信息打印 每100次打印一条 */
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
    /*填充T_MSG的基本信息*/
    ptMsg->wMsgId = wMsgId;
    ptMsg->ucMsgType = ucMsgType;
    ptMsg->ucIsSave = COMM_NOT_SAVED;
    ptMsg->ptNextMsg = NULL;
    memcpy(&ptMsg->tReceiver,ptReceiver,sizeof(PID));
    ptMsg->tSender.wPno = ptSelfPCB->wProcType;
    
    if (pData != NULL)
        memcpy((BYTE *)(ptMsg + 1), pData,dwMsgLen);  /*将填充内容拷入消息体*/
    
    /* 发送消息 */
    if (FALSE == SendInCPU(ptMsg,COMM_RELIABLE))
    {
        printf("[%s:%d]\n send msg=%d fail,Recv=%d", __FUNCTION__, __LINE__,wMsgId,ptReceiver->wPno);
        /*发送失败*/
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
    /*填充T_MSG的基本信息*/
    ptMsg->wMsgId = wMsgId;
    ptMsg->ucMsgType = ucMsgType;
    ptMsg->ucIsSave = COMM_NOT_SAVED;
    ptMsg->ptNextMsg = NULL;
    memcpy(&ptMsg->tReceiver,ptReceiver,sizeof(PID));
    
    if (pData != NULL)
        memcpy((BYTE *)(ptMsg + 1), pData,dwMsgLen);  /*将填充内容拷入消息体*/

    
    if (M_atScheTCB[ptDestPCB->wTno].dwTaskId == GetCurTaskID() ||
        M_atScheTCB[ptDestPCB->wTno].dwMailBoxId == VOS_CREATE_QUEUE_FAILURE)
    {
        DispatchMsg(ptMsg, ptDestPCB->wTno);
    }
    else
    {
        /*将消息指针放到进程所属调度任务的邮箱*/        
        if (TRUE != Vos_AppendQueue(M_atScheTCB[ptDestPCB->wTno].dwMailBoxId, (BYTE *)&ptMsg, sizeof(VOID *), VOS_NOT_URGENT_MSG, (WORD32)WAIT_FOREVER))
        {
            printf("[%s:%d]\n send msg=%d fail,Recv=0x%x\n", __FUNCTION__, __LINE__,wMsgId,ptReceiver->wPno);
            return ERR_COMM_SEND_ASYN_MSG_FAILED;
        }
    }

    return SUCCESS;
}

