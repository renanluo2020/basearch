#include <os/queue.h>
#include <os/mem.h>
#include <trace.h>

#include <errno.h>
#include <string.h>

typedef struct {
	uint16_t msgLen;
	void* msgBuf;
}fwk_msgNode_t;
typedef struct {
	fwk_queueAttr_t attr;
	void* mid; //also for used flag
	int aloneMutex;
	int head;
	int tail;
	void* msgQ;
	fwk_msgNode_t* varQ;
}fwk_queueList_t;
#define FWK_QUEUE_MAX_LIMIT 100
static fwk_queueList_t gFwkQueueList[FWK_QUEUE_MAX_LIMIT];
static int gCurQueueCount = 0;


UINT32 base_queue_init ()
{
    UINT32               u4Idx;

    if (pthread_mutex_init (&gOsixMutex, NULL))
    {
        return (OSIX_FAILURE);
    }

    /* Initialize all arrays. */
    for (u4Idx = 0; u4Idx <= OSIX_MAX_TSKS; u4Idx++)
    {
        gaOsixTsk[u4Idx].u2Free = OSIX_TRUE;
        gaOsixTsk[u4Idx].u4Events = 0;
        MEMSET (gaOsixTsk[u4Idx].name, '\0', (OSIX_NAME_LEN + 4));
    }
    for (u4Idx = 0; u4Idx <= OSIX_MAX_SEMS; u4Idx++)
    {
        gaOsixSem[u4Idx].u2Free = OSIX_TRUE;
        gaOsixSem[u4Idx].u2Filler = 0;
        MEMSET (gaOsixSem[u4Idx].name, '\0', (OSIX_NAME_LEN + 4));
    }
    for (u4Idx = 0; u4Idx <= OSIX_MAX_QUES; u4Idx++)
    {
        gaOsixQue[u4Idx].u2Free = OSIX_TRUE;
        gaOsixQue[u4Idx].u2Filler = 0;
        MEMSET (gaOsixQue[u4Idx].name, '\0', (OSIX_NAME_LEN + 4));
    }

    gStartTicks = times (NULL);
    return (OSIX_SUCCESS);
}


UINT32 base_queue_info_find (CHAR au1Name[], UINT32 u4RscType, UINT4 *pu4RscId)
{
    UINT32               u4Idx;

    if (STRCMP (au1Name, "") == 0)
    {
        return (OSIX_FAILURE);
    }
    if (pthread_mutex_lock (&gOsixMutex) != 0)
    {
        return (OSIX_FAILURE);
    }

    switch (u4RscType)
    {
        case OSIX_TSK:
            /* scan global task array to find the task */
            for (u4Idx = 1; u4Idx <= OSIX_MAX_TSKS; u4Idx++)
            {
                if (MEMCMP
                    (au1Name, gaOsixTsk[u4Idx].au1Name,
                     (OSIX_NAME_LEN + 4)) == 0)
                {
                    /***
                     * For the case of tasks, applications know only our array
                     * index.  This helps us to simulate events.             
                     ***/
                    *pu4RscId = u4Idx;
                    pthread_mutex_unlock (&gOsixMutex);
                    return (OSIX_SUCCESS);
                }
            }
            break;

        case OSIX_SEM:
            /* scan global semaphore array to find the semaphore */
            for (u4Idx = 1; u4Idx <= OSIX_MAX_SEMS; u4Idx++)
            {
                if (MEMCMP
                    (au1Name, gaOsixSem[u4Idx].au1Name,
                     (OSIX_NAME_LEN + 4)) == 0)
                {
                    /* pThread version of OsixRscFind returns pointer to semId */
                    *pu4RscId = (UINT32) &(gaOsixSem[u4Idx].SemId);
                    pthread_mutex_unlock (&gOsixMutex);
                    return (OSIX_SUCCESS);
                }
            }
            break;

        case OSIX_QUE:
            /* scan global queue array to find the queue */
            for (u4Idx = 1; u4Idx <= OSIX_MAX_QUES; u4Idx++)
            {
                if (MEMCMP
                    (au1Name, gaOsixQue[u4Idx].au1Name,
                     (OSIX_NAME_LEN + 4)) == 0)
                {
                    *pu4RscId = gaOsixQue[u4Idx].u4RscId;
                    pthread_mutex_unlock (&gOsixMutex);
                    return (OSIX_SUCCESS);
                }
            }
            break;

        default:
            break;
    }
    pthread_mutex_unlock (&gOsixMutex);
    return (OSIX_FAILURE);
}

UINT32 base_queue_info_add (CHAR au1Name[], UINT32 u4RscType, UINT4 u4RscId)
{
    UINT32               u4Idx;

    if (pthread_mutex_lock (&gOsixMutex) != 0)
    {
        return (OSIX_FAILURE);
    }

    switch (u4RscType)
    {
        case OSIX_TSK:
            /* scan global task array to find a free slot */
            for (u4Idx = 1; u4Idx <= OSIX_MAX_TSKS; u4Idx++)
            {
                if ((gaOsixTsk[u4Idx].u2Free) == OSIX_TRUE)
                {
                    gaOsixTsk[u4Idx].u2Free = OSIX_FALSE;
                    gaOsixTsk[u4Idx].u4Events = 0;
                    MEMCPY (gaOsixTsk[u4Idx].au1Name, au1Name,
                            (OSIX_NAME_LEN + 4));
                    pthread_mutex_unlock (&gOsixMutex);
                    return (OSIX_SUCCESS);
                }
            }
            break;

        case OSIX_SEM:
            /* scan global semaphore array to find a free slot */
            for (u4Idx = 1; u4Idx <= OSIX_MAX_SEMS; u4Idx++)
            {
                if ((gaOsixSem[u4Idx].u2Free) == OSIX_TRUE)
                {
                    gaOsixSem[u4Idx].u2Free = OSIX_FALSE;
                    gaOsixSem[u4Idx].u2Filler = 0;
                    MEMCPY (gaOsixSem[u4Idx].au1Name, au1Name,
                            (OSIX_NAME_LEN + 4));
                    pthread_mutex_unlock (&gOsixMutex);
                    return (OSIX_SUCCESS);
                }
            }
            break;

        case OSIX_QUE:
            /* scan global queue array to find a free slot */
            for (u4Idx = 1; u4Idx <= OSIX_MAX_QUES; u4Idx++)
            {
                if ((gaOsixQue[u4Idx].u2Free) == OSIX_TRUE)
                {
                    gaOsixQue[u4Idx].u4RscId = u4RscId;
                    gaOsixQue[u4Idx].u2Free = OSIX_FALSE;
                    gaOsixQue[u4Idx].u2Filler = 0;
                    MEMCPY (gaOsixQue[u4Idx].au1Name, au1Name,
                            (OSIX_NAME_LEN + 4));
                    pthread_mutex_unlock (&gOsixMutex);
                    return (OSIX_SUCCESS);
                }
            }
            break;

        default:
            break;
    }

    pthread_mutex_unlock (&gOsixMutex);
    return (OSIX_FAILURE);
}

/************************************************************************/
/*  Function Name   : OsixRscDel                                        */
/*  Description     : Free an allocated resouce                         */
/*  Input(s)        : u4RscType - Type of resource (Task/Queue/Sema4)   */
/*                  : u4RscId -   Resource-Id returned by OS            */
/*  Output(s)       : None                                              */
/*  Returns         : None                                              */
/************************************************************************/
VOID base_queue_info_del (UINT32 u4RscType, UINT4 u4RscId)
{
    UINT32               u4Idx;

    if (pthread_mutex_lock (&gOsixMutex) != 0)
    {
        return;
    }

    switch (u4RscType)
    {
        case OSIX_TSK:
            gaOsixTsk[u4RscId].u2Free = OSIX_TRUE;
            MEMSET (gaOsixTsk[u4RscId].name, '\0', (OSIX_NAME_LEN + 4));
            break;

        case OSIX_SEM:
            /* scan global semaphore array to find the semaphore */
            for (u4Idx = 1; u4Idx <= OSIX_MAX_SEMS; u4Idx++)
            {
                if ((&(gaOsixSem[u4Idx].SemId)) == (sem_t *) u4RscId)
                {
                    gaOsixSem[u4Idx].u2Free = OSIX_TRUE;
                    MEMSET (gaOsixSem[u4Idx].name, '\0',
                            (OSIX_NAME_LEN + 4));
                    break;
                }
            }
            break;

        case OSIX_QUE:
            /* scan global queue array to find the queue */
            for (u4Idx = 1; u4Idx <= OSIX_MAX_QUES; u4Idx++)
            {
                if ((gaOsixQue[u4Idx].u4RscId) == u4RscId)
                {
                    gaOsixQue[u4Idx].u2Free = OSIX_TRUE;
                    MEMSET (gaOsixQue[u4Idx].name, '\0',
                            (OSIX_NAME_LEN + 4));
                    break;
                }
            }
            break;

        default:
            break;
    }
    pthread_mutex_unlock (&gOsixMutex);
}


/************************************************************************/
/*  Function Name   : PTHREAD_Create_MsgQ                               */
/*  Description     : Creates a queue using a linear block of memory.   */
/*  Input(s)        : u4MaxMsgs  - Max messages that can be held.       */
/*                  : u4MaxMsgLen- Max length of a messages             */
/*  Output(s)       : None                                              */
/*  Returns         : Queue-Id, NULL if creation fails                  */
/************************************************************************/
tPthreadQId
PTHREAD_Create_MsgQ (UINT4 u4MaxMsgs, UINT4 u4MsgLen)
{
    pthread_mutex_t    *qSem;
    tPthreadQ          *pPthreadQ;

    /* Allocate memory for holding messages. Create a semaphore for      */
    /* protection between multiple simultaneous calls to write or read   */
    /* Initialize the read and write pointers to the Q start location    */
    /* Initia the pointer marking the end of the queue's memory location */
    pPthreadQ =
        (tPthreadQ *) MEM_MALLOC (((u4MaxMsgs + 1) * u4MsgLen) +
                              sizeof (tPthreadQ));
    if (pPthreadQ == NULL)
    {
        return (NULL);
    }
    pPthreadQ->pQBase = (UINT1 *) ((UINT1 *) pPthreadQ + sizeof (tPthreadQ));

    qSem = (pthread_mutex_t *) MEM_MALLOC (sizeof (pthread_mutex_t));
    if (NULL == qSem) 
    {
        free(pPthreadQ);
        return NULL;
    }
    pthread_mutex_init (qSem, 0);
    pPthreadQ->QueMutex = qSem;

    pthread_cond_init (&(pPthreadQ->QueCond), NULL);

    pPthreadQ->pQEnd = (pPthreadQ->pQBase) + ((u4MaxMsgs + 1) * u4MsgLen);
    pPthreadQ->pQRead = pPthreadQ->pQBase;
    pPthreadQ->pQWrite = pPthreadQ->pQBase;
    pPthreadQ->u4MsgLen = u4MsgLen;

    pPthreadQ->u4OverFlows = 0;

    return (pPthreadQ);
}

/************************************************************************/
/*  Function Name   : PTHREAD_Delete_MsgQ                               */
/*  Description     : Deletes a Q.                                      */
/*  Input(s)        : QueId     - The QId returned.                     */
/*  Output(s)       : None                                              */
/*  Returns         : None                                              */
/************************************************************************/
VOID
PTHREAD_Delete_MsgQ (tPthreadQId QId)
{
    tPthreadQ          *pPthreadQ = (tPthreadQ *) QId;
    /* Wait for semaphore to ensure that when the queue is deleted */
    /* no one is reading from or writing into it. Then delete the  */
    /* semaphore, free the queue memory and initialize queue start */
    if (pthread_mutex_lock (pPthreadQ->QueMutex))
    {
        return;
    }
    pthread_cond_destroy (&(pPthreadQ->QueCond));
    pthread_mutex_destroy (pPthreadQ->QueMutex);
    free (pPthreadQ->QueMutex);
    free ((VOID *) pPthreadQ);
}

/************************************************************************/
/*  Function Name   : PTHREAD_Send_MsgQ                                 */
/*  Description     : Sends a message to a Q.                           */
/*  Input(s)        : QueId -    The Q Id.                              */
/*                  : pu1Msg -   Pointer to message to be sent.         */
/*  Output(s)       : None                                              */
/*  Returns         : 0 on SUCCESS and (-1) on FAILURE                  */
/************************************************************************/
INT4
PTHREAD_Send_MsgQ (tPthreadQId QId, UINT1 *pMsg)
{
    tPthreadQ          *pPthreadQ = (tPthreadQ *) QId;
    UINT1              *pWrite, *pRead, *pBase, *pEnd;
    UINT4               u4MsgLen;

    /* Ensure mutual exclusion. Wait and take the mutual exclusion        */
    /* semaphore. A write is possible if the queue is not full. Queue is  */
    /* recognized as full if by writing one more message, write and read  */
    /* pointers become equal. Actually, this means that the queue holds   */
    /* only u4MaxMsgs-1 messages to be safe. When checking the pointers   */
    /* or when advancing the write pointer after the write operation,     */
    /* take care of the wrap-around since this is a circular queue. When  */
    /* the message is written, advance the write pointer by u4MsgLen.     */
    if (pthread_mutex_lock (pPthreadQ->QueMutex))
    {
        return (-1);
    }

    pWrite = pPthreadQ->pQWrite;
    pRead = pPthreadQ->pQRead;
    pBase = pPthreadQ->pQBase;
    pEnd = pPthreadQ->pQEnd;
    u4MsgLen = pPthreadQ->u4MsgLen;

    if (((pWrite + u4MsgLen) == pEnd) && (pRead == pBase))
    {
        pthread_mutex_unlock (pPthreadQ->QueMutex);
        pPthreadQ->u4OverFlows++;
        return (-1);
    }
    if ((pWrite + u4MsgLen) == pRead)
    {
        pthread_mutex_unlock (pPthreadQ->QueMutex);
        pPthreadQ->u4OverFlows++;
        return (-1);
    }
    memcpy (pWrite, pMsg, u4MsgLen);
    (pPthreadQ->pQWrite) += u4MsgLen;

    if ((pPthreadQ->pQWrite) == pEnd)
    {
        (pPthreadQ->pQWrite) = pBase;
    }

    /* unblock anyone waiting to read    */
    pthread_cond_signal (&pPthreadQ->QueCond);

    /* allow others to read/write/delete */
    pthread_mutex_unlock (pPthreadQ->QueMutex);

    return (0);
}

/************************************************************************/
/*  Function Name   : PTHREAD_Receive_MsgQ                              */
/*  Description     : Receives a message from a Q.                      */
/*  Input(s)        : QueId -     The Q Id.                             */
/*                  : i4Timeout - Time to wait in case of WAIT.         */
/*  Output(s)       : pu1Msg -    Pointer to message to be sent.        */
/*  Returns         : 0 on SUCCESS and (-1) on FAILURE                  */
/************************************************************************/
INT4
PTHREAD_Receive_MsgQ (tPthreadQId QId, UINT1 *pMsg, INT4 i4Timeout)
{
    tPthreadQ          *pPthreadQ = (tPthreadQ *) QId;
    
        //获取时间
        struct timespec outtime;
        clock_gettime(CLOCK_MONOTONIC, &outtime);
        //ms为毫秒，换算成秒
        outtime.tv_sec += i4Timeout/1000;
        
        //在outtime的基础上，增加ms毫秒
        //outtime.tv_nsec为纳秒，1微秒=1000纳秒
        //tv_nsec此值再加上剩余的毫秒数 ms%1000，有可能超过1秒。需要特殊处理
        UINT64  us = outtime.tv_nsec/1000 + 1000 * (i4Timeout % 1000); //微秒
        //us的值有可能超过1秒，
        outtime.tv_sec += us / 1000000; 

        us = us % 1000000;
        outtime.tv_nsec = us * 1000;//换算成纳秒
        
    /* Only FM task/thread reads from the queue. Multiple other tasks may */
    /* write. Deletion of the queue is also done only by the FM task. So  */
    /* a semaphore for mutual exclusion is needed for writing but not for */
    /* reading. Only a blocking semaphore to wait for a message is needed */
    /* if the queue is empty.                                             */

    /* Check if queue exists. If yes, wait and take the mutual exclusion  */
    /* semaphore. A read is possible if the queue is not empty. Queue is  */
    /* recognized as empty if write and read pointers are equal i.e. all  */
    /* the written messages have already been read.                       */
    if (pthread_mutex_lock (pPthreadQ->QueMutex))
    {
        return (-1);
    }

    /* NO_WAIT case: if i4Timeout is 0, it is NO_WAIT. */
    if (!i4Timeout && (pPthreadQ->pQWrite) == (pPthreadQ->pQRead))
    {
        pthread_mutex_unlock (pPthreadQ->QueMutex);
        return -1;
    }
    while ((pPthreadQ->pQWrite) == (pPthreadQ->pQRead))
    {
        /* Queue is empty. Block on the semaphore. When a write is done */
        /* the writer will release the semaphore and unblock this wait. */
        /* Before blocking, allow someone else to write into queue.     */
        pthread_cond_wait (&pPthreadQ->QueCond, pPthreadQ->QueMutex);
        /*pthread_cond_wait (&pPthreadQ->QueCond, pPthreadQ->QueMutex, &outtime);*/
    }

    /* There is at least 1 message in the queue and we have locked the */
    /* mutual exclusion semaphore so nobody else changes the state.    */
    memcpy (pMsg, pPthreadQ->pQRead, pPthreadQ->u4MsgLen);
    (pPthreadQ->pQRead) += (pPthreadQ->u4MsgLen);
    if ((pPthreadQ->pQRead) == (pPthreadQ->pQEnd))
    {
        (pPthreadQ->pQRead) = (pPthreadQ->pQBase);
    }
    pthread_mutex_unlock (pPthreadQ->QueMutex);
    return (0);
}

/************************************************************************/
/*  Function Name   : PTHREAD_MsgQ_NumMsgs                              */
/*  Description     : Returns No. of messages currently in a Q.         */
/*  Input(s)        : QueId -     The Q Id.                             */
/*  Output(s)       : None                                              */
/*  Returns         : pu4NumberOfMsgs - Contains count upon return.     */
/************************************************************************/
UINT4
PTHREAD_MsgQ_NumMsgs (tPthreadQId QId)
{
    tPthreadQ           PthreadQ = *((tPthreadQ *) QId);
    UINT4               u4Msgs;

    if ((PthreadQ.pQWrite) < (PthreadQ.pQRead))
    {
        u4Msgs =
            (PthreadQ.pQWrite) - (PthreadQ.pQBase) + (PthreadQ.pQEnd) -
            (PthreadQ.pQRead);
        return (u4Msgs / (PthreadQ.u4MsgLen));
    }
    else
    {
        return (((PthreadQ.pQWrite) - (PthreadQ.pQRead)) / (PthreadQ.u4MsgLen));
    }
}
/************************************************************************/
/* Routines for managing message queues                                 */
/************************************************************************/
/************************************************************************/
/*  Function Name   : OsixQueCrt                                        */
/*  Description     : Creates a OSIX Q.                                 */
/*  Input(s)        : au1name[ ] - The Name of the Queue.               */
/*                  : u4MaxMsgs  - Max messages that can be held.       */
/*                  : u4MaxMsgLen- Max length of a messages             */
/*  Output(s)       : pQueId     - The QId returned.                    */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT32 base_queue_create (UINT1 au1Name[], UINT4 u4MaxMsgs, UINT4 u4MaxMsgLen, tOsixQId * pQueId)
{
    EVENT_INF("OsixQueCrt (%s, %ld, %ld)\r\n", au1Name, u4MaxMsgLen, u4MaxMsgs);
    
    *pQueId = (tOsixQId) PTHREAD_Create_MsgQ (u4MaxMsgs, u4MaxMsgLen);
    if (*pQueId == (tOsixQId) NULL)
    {
        return (OSIX_FAILURE);
    }
    if (base_queue_info_find(au1Name, OSIX_QUE, *pQueId) == OSIX_FAILURE)
    {
        PTHREAD_Delete_MsgQ ((tPthreadQId) (*pQueId));
        return (OSIX_FAILURE);
    }
    return (OSIX_SUCCESS);
}

/************************************************************************/
/*  Function Name   : OsixQueDel                                        */
/*  Description     : Deletes a Q.                                      */
/*  Input(s)        : QueId     - The QId returned.                     */
/*  Output(s)       : None                                              */
/*  Returns         : OSIX_SUCCESS                                      */
/************************************************************************/
void base_queue_delete (tOsixQId QueId)
{
    QUEUE_INF("OsixQueDel (%ld)\r\n", QueId);
    
    base_queue_info_del (OSIX_QUE, (UINT4) QueId);
    
    PTHREAD_Delete_MsgQ ((tPthreadQId) QueId);
    return;
}

/************************************************************************/
/*  Function Name   : OsixQueSend                                       */
/*  Description     : Sends a message to a Q.                           */
/*  Input(s)        : QueId -    The Q Id.                              */
/*                  : pu1Msg -   Pointer to message to be sent.         */
/*                  : u4MsgLen - length of the messages                 */
/*  Output(s)       : None                                              */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT32 base_queue_send (tOsixQId QueId, UINT1 *pu1Msg, UINT4 u4MsgLen)
{
    u4MsgLen = 0;

    QUEUE_INF"OsixQueSend (%ld, 0x%x, %ld)\r\n", QueId, pu1Msg, u4MsgLen);
    
    /* Typically native OS calls take message Length as an argument.
     * In the case of PThreads Queues, the value supplied in the call
     * OsixQueCrt is used implicitly as the message length.
     * Hence the u4MsgLen parameter is not used in this function.
     */
    if (PTHREAD_Send_MsgQ ((tPthreadQId) QueId, pu1Msg) != 0)
    {
        return (OSIX_FAILURE);
    }
    return (OSIX_SUCCESS);
}

/************************************************************************/
/*  Function Name   : OsixQueRecv                                       */
/*  Description     : Receives a message from a Q.                      */
/*  Input(s)        : QueId -     The Q Id.                             */
/*                  : u4MsgLen -  length of the messages                */
/*                  : i4Timeout - Time to wait in case of WAIT.         */
/*  Output(s)       : pu1Msg -    Pointer to message to be sent.        */
/*  Returns         : OSIX_SUCCESS/OSIX_FAILURE                         */
/************************************************************************/
UINT32 base_queue_receive (tOsixQId QueId, UINT1 *pu1Msg, UINT4 u4MsgLen, INT4 i4Timeout)
{
    u4MsgLen = 0;

    QUEUE_INF"OsixQueRecv (%ld)\r\n", QueId);
    /* Typically native OS calls take message Length as an argument.
     * In the case of Pthreads Queues, the value supplied in the call
     * OsixQueCrt is used implicitly as the message length.
     * Hence the u4MsgLen parameter is not used in this function.
     */
    if (i4Timeout > 0)
    {
        i4Timeout = ((UINT4) i4Timeout);
    }

    if (PTHREAD_Receive_MsgQ ((tPthreadQId) QueId, pu1Msg, i4Timeout) != 0)
    {
        return (OSIX_FAILURE);
    }
    return (OSIX_SUCCESS);
}



void base_queue_show(fwk_queueID_t qID)
{
	int i, k = 0, idx, end;
	fwk_queueList_t* pQid = (fwk_queueList_t*)qID;
	if (pQid) {
		//fwk_basictrace_print(FWK_BASICTRACE_MODULE_FWK, FWK_BASICTRACE_LEVEL_ERR,
		printf("attr[name=%s, depth=%i, size=%i, maxBufSize=%i], mid[%p], msg[head=%i, tail=%i]:\n",
			pQid->attr.name,
			pQid->attr.depth,
			pQid->attr.size,
			pQid->attr.maxBufSize,
			pQid->mid,
			pQid->head,
			pQid->tail
			);
		if (pQid->head == pQid->tail) {
			printf("Queue is empty.\n");
			return;
		}
		end = (pQid->head < pQid->tail) ? pQid->tail : (pQid->tail + pQid->attr.depth);
		if (pQid->attr.size) {
			for (k = pQid->head; k < end; ++k) {
				idx = k % pQid->attr.depth;
				fwk_msgNode_t* pNode = pQid->varQ + idx;
				printf("\t%i: msg[len %i]=%s\n", idx, pNode->msgLen, (char*)pQid->msgQ + idx * pQid->attr.size);
			}
		} else {
			uint8_t* msgQTail = (uint8_t*)(pQid->msgQ) + pQid->attr.maxBufSize;
			for (k = pQid->head; k < end; ++k) {
				idx = k % pQid->attr.depth;
				fwk_msgNode_t* pNode = pQid->varQ + idx;
				printf("\t%i: len=%i, addr=%p, msg=%s\n", idx, pNode->msgLen, pNode->msgBuf, (char*)pNode->msgBuf);
				if ((uint8_t*)pNode->msgBuf + pNode->msgLen > msgQTail) {
					printf("\t--: %s\n", (char*)pQid->msgQ);
				}
			}
		}
	} else {
		for (i = 0; i < FWK_QUEUE_MAX_LIMIT; ++i) {
			if (gFwkQueueList[i].mid) {
				printf("Queue[%i]: ", i);
				fwk_showQueue(&gFwkQueueList[i]);
			}
		}
	}
}

