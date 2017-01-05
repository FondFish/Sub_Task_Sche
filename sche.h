
#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>

#define VOID void 
typedef void*               LPVOID;
typedef unsigned long WORD32;
typedef unsigned short      WORD16;
typedef unsigned char   BYTE;
typedef unsigned long  ULONG;
typedef char                CHAR;

/* ½ø³Ì×´Ì¬±êÊ¶ */
#define    PROC_STATUS_READY                ((BYTE)1)  
#define    PROC_STATUS_RUNNING              ((BYTE)2)  
#define    PROC_STATUS_BLOCK                ((BYTE)3)  

#define WEOF            (WORD16)(0xFFFF)

typedef VOID (*UniBTSProcEntry_Type)(WORD16, WORD16, VOID *, VOID *);

#define TASK_NAME_LENGTH        21      
#define PROC_NAME_LENGTH        30      

#define M_atScheTCB                         (M_ptCoreData->atScheTCB)
#define M_atPCB                             (M_ptCoreData->ptPCB)

typedef struct tagT_ProcessName
{
    WORD16  wProcType;              
    CHAR    acName[PROC_NAME_LENGTH];
}T_ProcessName;

typedef struct tagT_CoreData                    
{
    WORD32          dwCoreDataSize;               

    WORD16          wPCBCounts;                  
    T_PCB           *ptPCB;                      

    WORD32          dwProcNameCounts;            
    T_ProcessName   *pProcNameTable;              

    WORD16 atProcTypeIndexTab[PROC_TYPE_INDEX_NUM]; 

    T_ScheTCB       atScheTCB[SCHE_TASK_NUM];     

    WORD32          dwSelfPCBIndex;               
    
    BYTE            pIdx;  /* µ±Ç°½ø³ÌÔÚ½ø³ÌË÷Òý±íÖÐµÄÎ»ÖÃ */
}T_CoreData;

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

    VOID     (*pEntry)(void);        
    ULONG      dwProcSP;         
    ULONG      dwTaskSP;          
    BYTE        *pucStack;         
    WORD32      dwStackSize ;       

    WORD32      dwScheCounts;           
}T_PCB;
