#include "stdio.h"
#include "sche.h"

T_CoreData*         M_ptCoreData; 

int g_test1_cnt = 0;
int g_test2_cnt = 0;

void test1(WORD wState, WORD wSignal, void *pSignalPara, void *pVarP)
{
    g_test1_cnt ++;
    printf("test1 Entry cnt=%d\n",g_test1_cnt);
    
    NormalReturnToTask(gptSelfPCBInfo);
    
    return;
}

void test2(WORD wState, WORD wSignal, void *pSignalPara, void *pVarP)
{
    g_test2_cnt ++;
    printf("test1 Entry cnt=%d\n",g_test2_cnt);
    
    NormalReturnToTask(gptSelfPCBInfo);
    
    return;
}

void Init()
{
    M_ptCoreData = (T_CoreData *)OSS_Malloc(sizeof(T_CoreData));
    memset(M_ptCoreData, 0, sizeof(T_CoreData));
}
