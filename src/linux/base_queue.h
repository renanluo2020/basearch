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


#ifndef BASE_QUEUE_H
#define BASE_QUEUE_H
#include <stdlib.h>
#include <types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _QUEUE_TYPES_
#define _QUEUE_TYPES_

#define QUEUE_MOD_NAME            TRACE_M_OS
#define QUEUE_ERR(s...)       base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_ERR, s)
#define QUEUE_WRN(s...)      base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_WRN, s)
#define QUEUE_INF(s...)       base_trace_print(MEM_MOD_NAME, BASE_TRACE_LEVEL_MASK_INF, s)


/* A circular queue is simulated using an allocated linear memory */
/* region. Read and write pointers are used to take out and put   */
/* messages in the queue. All messages are the same size only.    */
/* So, a task or thread reads messages from this queue to service */
/* the requests one by ine i.e. one command or activity at a time */

/* The description of fields used in struct below are as follows: */
/*   pQBase - linear memory location holding messages             */
/*   pQEnd - pointer after last byte of the queue                 */
/*   pQRead - pointer where next message can be read from         */
/*   pQWrite - pointer where next message can be written to       */
/*   u4MsgLen - the length of messages on this queue              */
/*   QueCond  - Conditional variable to synch. Send/Receive       */
/*   QueMutex - semaphore for mutual exclusion during writes      */
typedef struct
{
    UINT1              *pQBase;
    UINT1              *pQEnd;
    UINT1              *pQRead;
    UINT1              *pQWrite;
    UINT4               u4MsgLen;
    UINT4               u4OverFlows;
    pthread_cond_t      QueCond;
    pthread_mutex_t    *QueMutex;
}
tPthreadQ;
typedef tPthreadQ  *tPthreadQId;


#ifdef __cplusplus
} /* extern C */
#endif

#endif /* BASE_QUEUE_H */
