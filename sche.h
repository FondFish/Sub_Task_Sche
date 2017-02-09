

#ifndef _OSS_SCHELIB_H
#define _OSS_SCHELIB_H

#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <semaphore.h>
#include "type.h"
#include "comm.h"

typedef VOID (*UniBTSProcEntry_Type)(WORD16, WORD16, VOID *, VOID *);
typedef VOID (*TaskEntryProto)(LPVOID); /* ÈÎÎñÈë¿ÚÔ­ÐÍ */

/*ÏûÏ¢¶ÓÁÐÏà¹Ø*/
#define VOS_CREATE_QUEUE_FAILURE    (WORD32)0xFFFFFFFF  /* ´´½¨¶ÓÁÐÊ§°Ü */
#define MAX_QUEUE_COUNT             (WORD32)10
#define VOS_QUEUE_CTL_USED          (BYTE)0x01  /* ¶ÓÁÐ¿ØÖÆ¿é±»Ê¹ÓÃ */
#define VOS_QUEUE_CTL_NO_USED       (BYTE)0x10  /* ¶ÓÁÐ¿ØÖÆ¿éÎ´±»Ê¹ÓÃ */
#define VOS_QUEUE_ERROR             (SWORD32)0xEFFFFFFF /* ±íÃ÷³ö¶Ó²Ù×÷Ê§°Ü */

#define MSQNAME "/OSS_MQ%d"

#define    SCHE_TASK_NUM              2
#define    TASK_NAME_LEN               60
#define    QUEUE_NAME_LEN              (WORD32)40   /* ¶ÓÁÐÃû×Ö³¤¶È */ 

/* ½ø³Ì×´Ì¬±êÊ¶ */
#define    PROC_STATUS_READY                ((BYTE)1)  /* ½ø³ÌÎ»ÓÚ¾ÍÐ÷¶ÓÁÐ */
#define    PROC_STATUS_RUNNING              ((BYTE)2)  /* ½ø³ÌÕýÔÚÔËÐÐ */
#define    PROC_STATUS_BLOCK                ((BYTE)3)  /* ½ø³Ì´¦ÓÚ×èÈû¶ÓÁÐ */
/* ½ø³Ì×èÈûµÈ´ýÔ­Òò */
#define    WAIT_MSG                         (0)  /* ÎÞµÈ´ý */
#define    WAIT_REMOTECALL                  (1)  /* Ô¶³Ì¹ý³Ìµ÷ÓÃµÈ´ý */
#define    WAIT_SYNCMSG                     (2)  /* Í¬²½ÏûÏ¢µÈ´ý */
#define    WAIT_SYNCTIMER                   (3)  /* Í¬²½¶¨Ê±Æ÷µÈ´ý */

#define EV_STARTUP                              (WORD16)100

#define WEOF               (WORD16)(0xFFFF)

#define SUCCESS      0
#define ERROR_UNKNOWN     1

#define  YES     1
#define  NO      0

#define TRUE      1
#define FALSE    0

#define  S_StartUp     0

#define TASK_NAME_LENGTH        21      /* ÈÎÎñÃû³¤¶È */
#define PROC_NAME_LENGTH        30      /* ½ø³ÌÃû×Ö·û´®³¤¶È*/
#define INVALID_SYS_TASKID          (WORD32)0   /* ÎÞÐ§µÄÏµÍ³ÈÎÎñID */

#define PROC_TYPE_INDEX_NUM     ((WORD16)0xFFFF)

typedef struct tagT_ProcessName
{
    WORD16  wProcType;               /* ½ø³ÌÀàÐÍºÅ */
    CHAR    acName[PROC_NAME_LENGTH];/* ½ø³ÌÃû */
}T_ProcessName;

typedef struct tagT_TaskConfig
{
    BYTE    ucIsUse;                  /* ÈÎÎñÊÇ·ñÊ¹ÓÃ */
    CHAR    acName[TASK_NAME_LENGTH]; /* ÈÎÎñÃû */
    WORD16  wPriority;                /* ÈÎÎñÓÅÏÈ¼¶ */
    WORD32  dwStacksize;              /* ÈÎÎñÕ»´óÐ¡£¬Ö»ÄÜÅäÖÃ³ÉÒ³´óÐ¡µÄÕûÊý±¶ */
    LPVOID  pEntry;                   /* ÈÎÎñÈë¿Úº¯Êý */
}T_TaskConfig ;

/* ½ø³ÌÅäÖÃ±íÏî½á¹¹ */
typedef struct tagT_ProcessConfig
{
    BYTE    ucIsUse;                 /* ½ø³ÌÊÇ·ñÊ¹ÓÃ */
    WORD16  wProcType;               /* ½ø³ÌÀàÐÍºÅ */
    WORD16  wTno;                    /* ½ø³ÌËùÊôÓÚµÄµ÷¶ÈÈÎÎñºÅ */
    LPVOID  pEntry;                  /* ½ø³ÌÈë¿Úº¯Êý */
    WORD32  dwStackSize;             /* ½ø³ÌÕ»´óÐ¡£¬Ö»ÄÜÅäÖÃ³ÉÒ³´óÐ¡µÄÕûÊý±¶ */
    WORD32  dwDataRegionSize;        /* ½ø³ÌÊý¾ÝÇø´óÐ¡£¬Ö»ÄÜÅäÖÃ³ÉÒ³´óÐ¡µÄÕûÊý±¶ */
}T_ProcessConfig;

typedef struct tagT_ScheTCB
{
    BYTE     ucIsUse;
    CHAR     acName[TASK_NAME_LENGTH];
    WORD16   wPriority;
    WORD32  dwStacksize;
    void (*pEntry)(WORD32);
    WORD32   dwMailBoxId;
    ULONG   dwTaskId;

    WORD16  wBlockHead;             /* ÈÎÎñÖÐ×èÈû½ø³Ì¶ÓÁÐÍ·Ë÷Òý */
    WORD16  wReadyHead;             /* ÈÎÎñÖÐ¾ÍÐ÷½ø³Ì¶ÓÁÐÍ·Ë÷Òý */
    WORD16  wRunning;               /* ÈÎÎñÕýÔÚÔËÐÐµÄ½ø³ÌµÄË÷Òý */

    WORD16  wReadyCounts;           /* ±¾ÈÎÎñÖÐµÄready½ø³ÌÊý */
    WORD16  wBlockCounts;           /* ±¾ÈÎÎñÖÐµÄblock½ø³ÌÊý */

    WORD32  dwScheCounts;           /* ÈÎÎñ±»µ÷¶È´ÎÊý */    
    WORD16  wProcTypeCounts;        /* ±¾ÈÎÎñÖÐµÄ½ø³ÌÀàÐÍÊý */

    BYTE	bEnterProc; 			/*ÊÇ·ñ¿ªÊ¼µ÷¶È½ø³Ì*/
}T_ScheTCB;

typedef struct tagT_PCB
{
    BYTE        ucIsUse;            /* ½ø³ÌÊÇ·ñÊ¹ÓÃ */
    BYTE        ucRunStatus;        /* Ready(Î»ÓÚ¾ÍÐ÷¶ÓÁÐ),Block(Î»ÓÚ×èÈû¶ÓÁÐ),Run*/
    WORD16      wTno;               /* ½ø³ÌËùÔÚµÄÈÎÎñ±êÊ¶ */

    /*×èÈû/ÔËÐÐÁ´±í,¸ù¾ÝÕâ¸öPCB×´Ì¬¶ø¶¨*/
    WORD16      wPrevByStatus;   /*Ç°Ò»¸öÔËÐÐ»ò×èÈûµÄPCB*/
    WORD16      wNextByStatus;   /*ºóÒ»¸öÔËÐÐ»ò×èÈûµÄPCB*/

    WORD16      wState;             /* ½ø³ÌµÄÒµÎñ×´Ì¬ */
    WORD16      wRetState;             /* ½ø³ÌµÄ·µ»Ø×´Ì¬ */
    WORD16      wInstanceNo;        /*½ø³ÌÊµÀýºÅ */
    WORD16      wProcType;          /* ½ø³ÌÀàÐÍ */

    VOID     (*pEntry)(void);        
    ULONG      dwProcSP;           /* ½ø³Ì¶ÑÕ»¶¥Ö¸Õë */
    ULONG      dwTaskSP;           /* ÈÎÎñ¶ÑÕ»¶¥Ö¸Õë */
    BYTE        *pucStack;          /* ½ø³Ì¶ÑÕ»µ×Ö¸Õë */
    WORD32      dwStackSize ;       /* ¶ÑÕ»´óÐ¡ */
    
    BYTE        *pucDataRegion;     /* Ë½ÓÐÊý¾ÝÇøÖ¸Õë */
    WORD32      dwDataRegionSize;

    BYTE        ucBlockReason;      /* PCB×èÈûµÈ´ýÔ­Òò */
    WORD32      dwScheCounts;           /* ¾àÉÏ´Î½ø³ÌÆµ¶ÈÍ³¼ÆÊ±µÄµ÷¶È´ÎÊý */
    
    T_MSG       *ptCurMsg;   /* µ±Ç°ÏûÏ¢ */
    WORD32      dwMsgQCounts;            /* ½ø³ÌÏûÏ¢¶ÓÁÐÖÐµÄÏûÏ¢Êý */
    /* ½ø³ÌÏûÏ¢¶ÓÁÐÖ¸Õë */
    T_MSG       *ptPrevMsg;         /* ½ø³Ìµ±Ç°ÏûÏ¢µÄÇ°Ò»¸öÏûÏ¢Ö¸Õë */
    T_MSG       *ptFirstMsg ;       /* ½ø³ÌÏûÏ¢¶ÓÁÐµÄµÚÒ»ÌõÏûÏ¢ */
    T_MSG       *ptLastMsg ;        /* ½ø³ÌÏûÏ¢¶ÓÁÐµÄ×îºóÒ»ÌõÏûÏ¢ */
}T_PCB;

typedef struct tQCtlBlk
{
    WORD32      dwTotalCount;       /* ¶ÓÁÐÔªËØ×Ü¸öÊý*/
    WORD32      dwEntrySize;        /* ¶ÓÁÐÔªËØ³ß´ç */

    WORD32      dwUsedCount;        /* ÏÖÓÐµÄ¶ÓÁÐÔªËØ¸öÊý */
    WORD32      dwQueueId;        /* VxWorks¶ÓÁÐµÄÈÎÎñID */

    BYTE        ucUsed;             /* Ê¹ÓÃ±êÖ¾ */

    CHAR        aucQueueName[QUEUE_NAME_LEN]; /* ¶ÓÁÐÃû×Ö */

    WORD16      wLostMsgQFull;     /*¼ÇÂ¼¶ÓÁÐÂú¶ªÆúµÄÏûÏ¢Êý*/ 
    CHAR        pad[2];            /*¶ÔÆëÌî³ä×Ö½Ú*/	
    WORD32      dwUsedMaxNum;
}T_QueueCtl;

typedef  struct ThreadInfo_tag
{
    CHAR            flag;
    CHAR            pri;
    CHAR            name[TASK_NAME_LEN];
    TaskEntryProto  pFun;
    VOID            *arg;
    WORD32      thread_id;
    WORD32          thread_tid; 
    INT             stacksize;      /* Ïß³Ì¶ÑÕ»´óÐ¡ */
    WORD32          stackguardsize; /* Ïß³Ì¶ÑÕ»±£»¤Ò³´óÐ¡ */
    ULONG         stackaddr;      /* Ïß³Ì¶ÑÕ»µÍµØÖ·ÆðÊ¼´¦ */
    WORD32          tSpareInfo;
    BYTE            *pbySigAltStack;/* Ïß³ÌÐÅºÅ´¦Àíº¯Êý×¨ÓÃÕ»*/
}T_ThreadInfo;

typedef struct tagT_CoreData                    
{
    WORD32          dwCoreDataSize;               /* ½ø³Ì¼¶:ºËÐÄÊý¾ÝÇøµÄ´óÐ¡ */

    WORD16          wPCBCounts;                   /* µÍ³ÖÐµÄPCB×ÊÔ´ÊýÁ¿ */
    T_PCB           *ptPCB;                       /* ÏµÍ³µÄµÚÒ»¸öPCB¿éÖ¸Õë */

    WORD32          dwProcNameCounts;             /* ½ø³Ì¼¶:È«¾Ö½ø³ÌÃû¸öÊý */
    T_ProcessName   *pProcNameTable;              /* ½ø³Ì¼¶:È«¾Ö½ø³ÌÃû±í */

    T_ScheTCB       atScheTCB[SCHE_TASK_NUM];     /* ½ø³Ì¼¶:µ÷¶ÈÈÎÎñTCBÊý×é */

    WORD32          dwSelfPCBIndex;               /* ½ø³Ì¼¶:µ±Ç°ÔËÐÐµÄ½ø³ÌµÄË÷Òý */
    
    BYTE            pIdx;  /* µ±Ç°½ø³ÌÔÚ½ø³ÌË÷Òý±íÖÐµÄÎ»ÖÃ */
}T_CoreData;


extern __thread T_PCB * gptSelfPCBInfo;
extern VOID ScheTaskEntry(int dwTno);
extern void NormalReturnToTask(T_PCB *ptPCB);


#endif

