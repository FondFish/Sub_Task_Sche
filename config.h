#ifndef _CONFIG_H
#define _CONFIG_H

#include <stdlib.h>
#include <stdio.h>
#include "sche.h"

extern T_CoreData*         M_ptCoreData;

#define M_atScheTCB                         (M_ptCoreData->atScheTCB)
#define M_atPCB                             (M_ptCoreData->ptPCB)

extern  T_QueueCtl   s_atQueueCtl[MAX_QUEUE_COUNT];
extern  T_ThreadInfo                      g_threadInfo[SCHE_TASK_NUM];

extern    int    g_procNum;
extern    T_ProcessConfig gaProcCfgTbl[];
extern    T_TaskConfig    gaScheTaskConfig[SCHE_TASK_NUM]; 


#endif

