#ifndef _COMM_H
#define _COMM_H

#include "type.h"
#include "sche.h"

#define VOS_URGENT_MSG              (BYTE)0x01  /* 插入紧迫消息 */
#define VOS_NOT_URGENT_MSG          (BYTE)0x10  /* 插入非紧迫消息 */

#define NO_WAIT         (WORD32)0
#define WAIT_FOREVER    (WORD32)0xFFFFFFFF

#define OK      0
#define ERROR          (-1) 

/* 消息类型定义 */
#define COMM_ASYN_NORMAL        (BYTE)1 /* 异步普通消息 */
#define COMM_ASYN_URGENT        (BYTE)2 /* 异步紧迫消息 */
#define COMM_TIMER_OUT          (BYTE)3 /* 定时器到消息 */
#define COMM_CONTROL_MSG        (BYTE)4 /* 通信层的控制消息 */
#define COMM_SYSTEM_MSG         (BYTE)5 /* 系统消息 */ 
#define COMM_SYNC_MSG           (BYTE)6 /* 同步消息 */ 
#define COMM_SYN_URGENT         (BYTE)7 /* 同步紧急消息 */
#define COMM_SYNC_PROCESS  (BYTE)8    /*进程间同步消息*/
/*是否保留*/
#define COMM_NOT_SAVED          0   /* 不是保留消息*/
#define COMM_IS_SAVED           1   /* 是保留消息*/

/* 协议类型 */                                                                         
#define COMM_RELIABLE           (BYTE)0 /* 可靠协议 */                                 
#define COMM_UNRELIABLE         (BYTE)1 /* 不可靠协议 */  

/*错误码*/ 
#define ERR_COMM_SEND_ASYN_MSG_FAILED   101

typedef struct tagPID
{ 
    WORD16  wPno;         
    BYTE        ucRouteType;    
    BYTE       rsv[3];
}PID;

typedef struct tagMsg
{
    PID             tSender;        /* 发送进程PID  */
    PID             tReceiver;      /* 接收进程PID  */
    WORD16          wMsgId;         /* 消息事件号   */
    WORD16          wMsgLen;       
    BYTE            ucMsgType;      /* 消息类型     */
    BYTE            ucIsSave;       /* 是否保留消息 */
    BYTE            ucSynIndex;     /* 同步发送次数 */
    BYTE            ucWndSize;      /* 窗口扩大标志 */
    struct tagMsg   *ptNextMsg;     /* 指向下一消息的指针 */
}T_MSG;


#endif
