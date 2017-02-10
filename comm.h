#ifndef _COMM_H
#define _COMM_H

#include "type.h"
#include "sche.h"

#define VOS_URGENT_MSG              (BYTE)0x01  /* ²åÈë½ôÆÈÏûÏ¢ */
#define VOS_NOT_URGENT_MSG          (BYTE)0x10  /* ²åÈë·Ç½ôÆÈÏûÏ¢ */

#define NO_WAIT         (WORD32)0
#define WAIT_FOREVER    (WORD32)0xFFFFFFFF

#define OK      0
#define ERROR          (-1) 

/* ÏûÏ¢ÀàÐÍ¶¨Òå */
#define COMM_ASYN_NORMAL        (BYTE)1 /* Òì²½ÆÕÍ¨ÏûÏ¢ */
#define COMM_ASYN_URGENT        (BYTE)2 /* Òì²½½ôÆÈÏûÏ¢ */
#define COMM_TIMER_OUT          (BYTE)3 /* ¶¨Ê±Æ÷µ½ÏûÏ¢ */
#define COMM_CONTROL_MSG        (BYTE)4 /* Í¨ÐÅ²ãµÄ¿ØÖÆÏûÏ¢ */
#define COMM_SYSTEM_MSG         (BYTE)5 /* ÏµÍ³ÏûÏ¢ */ 
#define COMM_SYNC_MSG           (BYTE)6 /* Í¬²½ÏûÏ¢ */ 
#define COMM_SYN_URGENT         (BYTE)7 /* Í¬²½½ô¼±ÏûÏ¢ */
#define COMM_SYNC_PROCESS  (BYTE)8    /*½ø³Ì¼äÍ¬²½ÏûÏ¢*/
/*ÊÇ·ñ±£Áô*/
#define COMM_NOT_SAVED          0   /* ²»ÊÇ±£ÁôÏûÏ¢*/
#define COMM_IS_SAVED           1   /* ÊÇ±£ÁôÏûÏ¢*/

/* Ð­ÒéÀàÐÍ */                                                                         
#define COMM_RELIABLE           (BYTE)0 /* ¿É¿¿Ð­Òé */                                 
#define COMM_UNRELIABLE         (BYTE)1 /* ²»¿É¿¿Ð­Òé */  

/*´íÎóÂë*/ 
#define ERR_COMM_SEND_ASYN_MSG_FAILED   101

typedef struct tagPID
{ 
    WORD16  wPno;         
    BYTE        ucRouteType;    
    BYTE       rsv[3];
}PID;

typedef struct tagMsg
{
    PID             tSender;        /* ·¢ËÍ½ø³ÌPID  */
    PID             tReceiver;      /* ½ÓÊÕ½ø³ÌPID  */
    WORD16          wMsgId;         /* ÏûÏ¢ÊÂ¼þºÅ   */
    WORD16          wMsgLen;       
    BYTE            ucMsgType;      /* ÏûÏ¢ÀàÐÍ     */
    BYTE            ucIsSave;       /* ÊÇ·ñ±£ÁôÏûÏ¢ */
    BYTE            ucSynIndex;     /* Í¬²½·¢ËÍ´ÎÊý */
    BYTE            ucWndSize;      /* ´°¿ÚÀ©´ó±êÖ¾ */
    struct tagMsg   *ptNextMsg;     /* Ö¸ÏòÏÂÒ»ÏûÏ¢µÄÖ¸Õë */
}T_MSG;


#endif
