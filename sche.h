
#ifndef _OSS_SCHELIB_H
#define _OSS_SCHELIB_H

#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <semaphore.h>

#define VOID void 
typedef void*               LPVOID;
typedef unsigned long WORD32;
typedef unsigned short      WORD16;
typedef unsigned char   BYTE;
typedef unsigned long  ULONG;
typedef char                CHAR;
typedef signed long         SWORD32;
typedef unsigned long   OSS_ULONG;
typedef BYTE    BOOLEAN;
typedef signed long       OSS_LONG;
typedef int       INT;
typedef unsigned short  WORD;

typedef VOID (*UniBTSProcEntry_Type)(WORD16, WORD16, VOID *, VOID *);
typedef VOID (*TaskEntryProto)(LPVOID); 

/*ÏûÏ¢¶ÓÁÐÏà¹Ø*/
#define VOS_CREATE_QUEUE_FAILURE    (WORD32)0xFFFFFFFF  
#define MAX_QUEUE_COUNT             (WORD32)10
#define VOS_QUEUE_CTL_USED          (BYTE)0x01  
#define VOS_QUEUE_CTL_NO_USED       (BYTE)0x10 
#define MSQNAME "/OSS_MQ%d"

#define    SCHE_TASK_NUM              2
#define    TASK_NAME_LEN               60
#define    QUEUE_NAME_LEN              (WORD32)40   

/* ½ø³Ì×´Ì¬±êÊ¶ */
#define    PROC_STATUS_READY                ((BYTE)1)  
#define    PROC_STATUS_RUNNING              ((BYTE)2) 
#define    PROC_STATUS_BLOCK                ((BYTE)3)  

#define WEOF               (WORD16)(0xFFFF)
#define OSS_SUCCESS      0
#define OSS_ERROR_UNKNOWN     1

#define  YES     1
#define  NO      0

#define TRUE      1
#define FALSE    0

#define  S_StartUp     0

#define TASK_NAME_LENGTH        21     
#define PROC_NAME_LENGTH        30     
#define INVALID_SYS_TASKID          (WORD32)0  

#define PROC_TYPE_INDEX_NUM     ((WORD16)0xFFFF)

typedef struct tagT_ProcessName
{
    WORD16  wProcType;               
    CHAR    acName[PROC_NAME_LENGTH];
}T_ProcessName;

typedef struct tagT_TaskConfig
{
    BYTE    ucIsUse;                 
    CHAR    acName[TASK_NAME_LENGTH]; 
    WORD16  wPriority;               
    WORD32  dwStacksize;             
    LPVOID  pEntry;                  
}T_TaskConfig ;

typedef struct tagT_ProcessConfig
{
    BYTE    ucIsUse;               
    WORD16  wProcType;          
    WORD16  wTno;                 
    LPVOID  pEntry;               
    WORD32  dwStackSize;          
    WORD32  dwDataRegionSize;      
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
    WORD16  wBlockHead;         
    WORD16  wReadyHead;     
    WORD16  wRunning;       
    WORD16  wReadyCounts;       
    WORD16  wBlockCounts;     
    WORD32  dwScheCounts;         
    WORD16  wProcTypeCounts;   
    BYTE	bEnterProc; 			
}T_ScheTCB;

typedef struct tagT_PCB
{
    BYTE        ucIsUse;          
    BYTE        ucRunStatus;     
    WORD16      wTno;          
    WORD16      wPrevByReady;  
    WORD16      wNextByReady;   
    WORD16      wState;           
    WORD16      wInstanceNo;      
    WORD16      wProcType;       
    VOID     (*pEntry)(void);        
    ULONG      dwProcSP;         
    ULONG      dwTaskSP;          
    BYTE        *pucStack;         
    WORD32      dwStackSize ;      
    WORD32      dwDataRegionSize;
    WORD32      dwScheCounts;          
}T_PCB;

typedef struct tQCtlBlk
{
    WORD32      dwTotalCount;       
    WORD32      dwEntrySize;       
    WORD32      dwUsedCount;      
    WORD32      dwQueueId;       
    BYTE        ucUsed;            
    CHAR        aucQueueName[QUEUE_NAME_LEN]; 
    WORD16      wLostMsgQFull;    
    CHAR        pad[2];           
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
    INT             stacksize;      
    WORD32          stackguardsize; 
    OSS_ULONG         stackaddr;     
    WORD32          tSpareInfo;
    BYTE            *pbySigAltStack;
}T_ThreadInfo;

typedef struct tagT_CoreData                    
{
    WORD32          dwCoreDataSize; 
    WORD16          wPCBCounts;       
    T_PCB           *ptPCB;          
    WORD32          dwProcNameCounts;  
    T_ProcessName   *pProcNameTable;   
    T_ScheTCB       atScheTCB[SCHE_TASK_NUM]; 
    WORD32          dwSelfPCBIndex;              
    BYTE            pIdx; 
}T_CoreData;


extern __thread T_PCB * gptSelfPCBInfo;
extern VOID ScheTaskEntry(int dwTno);
extern void NormalReturnToTask(T_PCB *ptPCB);


#endif
