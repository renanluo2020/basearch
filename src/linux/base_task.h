/************************************************************************
*                                                                     
*                                                           
*                                                                  
*  Project Code:                                  
*  Create Date:                                      
*  Author:                                  
*  Modify Date:                                          
*  Module Name:                                  
*  Others:                                                            
*                                                                      
*                         
*                                             
*                                                                      
************************************************************************/


#ifndef BASE_TASK_H
#define BASE_TASK_H
#include <stdlib.h>
#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _TASK_TYPES_
#define _TASK_TYPES_

#define TASK_MOD_NAME            TRACE_M_OS
#define TASK_ERR(s...)       base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_ERR, s)
#define TASK_WRN(s...)      base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_WRN, s)
#define TASK_INF(s...)       base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_INF, s)

#define OSIX_SUCCESS               0
#define OSIX_FAILURE               1

#define  OSIX_MAX_TSKS         100

#define  OSIX_MAX_SEMS         120000

#define  OSIX_MAX_QUES         200

typedef UINT32               tOsixQId;
typedef sem_t *             tOsixSemId;
typedef UINT32               tOsixTaskId;
typedef UINT32               tOsixSysTime;
typedef tCRU_BUF_CHAIN_DESC tOsixMsg;
typedef struct OsixRscTskStruct
{

    pthread_t           pthread_id;
    UINT32               Events;
    UINT32               u4Arg;
    UINT32               u4Prio;
    UINT32               u4StackSize;
    pthread_mutex_t     TskMutex;
    pthread_cond_t      EvtCond;
    pthread_mutex_t     EvtMutex;
    void                (*pTskStrtAddr) (INT4);
    UINT2               u2Free;
    UINT2               u2Pad;
    CHAR               au1Name[OSIX_NAME_LEN + 4];
}
tOsixTsk;


typedef VOID* base_task_id;

typedef VOID *(*base_task_func_t)(void *);

typedef struct base_task_info_struct
{
    CHAR                     name[BASE_TASK_NAME_LEN];
    base_task_func_t    func;
    void *                    funcargs;
    UINT32                  stacksize;
    UINT32                  policy;
    UINT32                  priority;   
    UINT32                  events;
    pthread_cond_t      eventstCond;
    pthread_mutex_t    eventsmutex;
    pthread_mutex_t    mutex;
    pthread_t               tidp;
    UINT32                  used;       

}base_task_info_t;

typedef struct OsixRscQueStruct
{
    UINT32               u4RscId;
    UINT2               u2Free;
    UINT2               u2Filler;
    CHAR               au1Name[OSIX_NAME_LEN + 4];
}
tOsixQue;
typedef struct OsixRscSemStruct
{
    sem_t               SemId;
    UINT2               u2Free;
    UINT2               u2Filler;
    CHAR               au1Name[OSIX_NAME_LEN + 4];
}
tOsixSem;

#endif

#ifdef __cplusplus
} /* extern C */
#endif

#endif 
