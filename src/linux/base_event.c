//
//    Copyright (C) 2017 LGPL
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Lesser General Public
//    License as published by the Free Software Foundation; either
//    version 2.1 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Lesser General Public License for more details.
//
//    You should have received a copy of the GNU Lesser General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
//    USA

#include <fwk/task/event.h>
#include <fwk/task/queue.h>
#include <fwk/memmgmt/memmgmt.h>
#include <fwk/basic/basictrace.h>
#include <errno.h>
#include <string.h>




UINT32 base_event_init ()
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
        MEMSET (gaOsixTsk[u4Idx].name_tmp, '\0', (OSIX_NAME_LEN + 4));
    }
    for (u4Idx = 0; u4Idx <= OSIX_MAX_SEMS; u4Idx++)
    {
        gaOsixSem[u4Idx].u2Free = OSIX_TRUE;
        gaOsixSem[u4Idx].u2Filler = 0;
        MEMSET (gaOsixSem[u4Idx].name_tmp, '\0', (OSIX_NAME_LEN + 4));
    }
    for (u4Idx = 0; u4Idx <= OSIX_MAX_QUES; u4Idx++)
    {
        gaOsixQue[u4Idx].u2Free = OSIX_TRUE;
        gaOsixQue[u4Idx].u2Filler = 0;
        MEMSET (gaOsixQue[u4Idx].name_tmp, '\0', (OSIX_NAME_LEN + 4));
    }

    gStartTicks = times (NULL);
    return (OSIX_SUCCESS);
}


UINT32 base_event_info_find (CHAR au1Name[], UINT32 u4RscType, UINT4 *pu4RscId)
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

UINT32 base_event_info_add (CHAR au1Name[], UINT32 u4RscType, UINT4 u4RscId)
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
VOID base_event_info_del (UINT32 u4RscType, UINT4 u4RscId)
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
            MEMSET (gaOsixTsk[u4RscId].name_tmp, '\0', (OSIX_NAME_LEN + 4));
            break;

        case OSIX_SEM:
            /* scan global semaphore array to find the semaphore */
            for (u4Idx = 1; u4Idx <= OSIX_MAX_SEMS; u4Idx++)
            {
                if ((&(gaOsixSem[u4Idx].SemId)) == (sem_t *) u4RscId)
                {
                    gaOsixSem[u4Idx].u2Free = OSIX_TRUE;
                    MEMSET (gaOsixSem[u4Idx].name_tmp, '\0',
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
                    MEMSET (gaOsixQue[u4Idx].name_tmp, '\0',
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

/************************************************************************
 *  Function Name   : OsixSendEvent
 *  Description     : This sends an event to a task.
 *  Input           : u4Node - Unused
 *                    au1TskName - Name of task to which to send event.
 *                    u4Events - A bit mask of the event to be sent.
 *  Returns         : None.
 ************************************************************************/
UINT32 base_event_send (const UINT1 au1TskName[], UINT4 u4Events)
{
    tOsixTaskId         TskId;
    UINT1               au1Name[OSIX_NAME_LEN + 4], u1Index;

    MEMSET ((UINT1 *) au1Name, '\0', (OSIX_NAME_LEN + 4));
    for (u1Index = 0;
         ((u1Index < OSIX_NAME_LEN) && (au1TskName[u1Index] != '\0'));
         u1Index++)
    {
        au1Name[u1Index] = au1TskName[u1Index];
    }


    if (base_task_info_find(au1Name, OSIX_TSK, &TskId) == OSIX_SUCCESS)
    {
        UINT4               u4Idx = (UINT4) TskId;

        EVENT_INF("OsixEvtSend (%ld, 0x%lx)\r\n", (UINT4) TskId, u4Events);
        pthread_mutex_lock (&gaOsixTsk[u4Idx].EvtMutex);
        gaOsixTsk[u4Idx].u4Events |= u4Events;
        pthread_cond_signal (&gaOsixTsk[u4Idx].EvtCond);
        pthread_mutex_unlock (&gaOsixTsk[u4Idx].EvtMutex);

        return (OSIX_SUCCESS);
    }
    return OSIX_FAILURE;
}

/************************************************************************
 *  Function Name   : OsixReceiveEvent
 *  Description     : This is called by an application to receive
 *                    an event.
 *  Input           : u4Node - Unused
 *                    u4Flags - whether blocking (OSIX_WAIT) or
 *                                  non-blocking (OSIX_NO_WAIT)
 *                    u4Timeout - Unused (Not supported.)
 *  Output          : pu4RcvdEvts - Pointer to memory location which
 *                                  contains received events upon return.
 *  Returns         : OSIX_SUCCESS/OSIX_FAILURE.
 ************************************************************************/
UINT4
base_event_receive (UINT4 u4Events, UINT4 u4Flags, UINT4 *pu4RcvdEvts)
{
    tOsixTaskId         TskId;

    if ((TskId = base_task_cur_get ()) == 0)
    {
        return (OSIX_FAILURE);
    }
    
    UINT4               u4Idx = (UINT4) TskId;

    EVENT_INF("OsixEvtRecv (%ld, 0x%lx, %ld, 0x%lx)\r\n", (UINT4) TskId,
               u4Events, u4Flg, pu4RcvEvents);

    *pu4RcvEvents = 0;

    pthread_mutex_lock (&gaOsixTsk[u4Idx].EvtMutex);

    if ((u4Flg == OSIX_NO_WAIT) &&
        (((gaOsixTsk[u4Idx].u4Events) & u4Events) == 0))
    {
        pthread_mutex_unlock (&gaOsixTsk[u4Idx].EvtMutex);
        return (OSIX_FAILURE);
    }

    while (1)
    {
        if (((gaOsixTsk[u4Idx].u4Events) & u4Events) != 0)
        {
            /* A required event has happened */
            *pu4RcvEvents = (gaOsixTsk[u4Idx].u4Events) & u4Events;
            gaOsixTsk[u4Idx].u4Events &= ~(*pu4RcvEvents);
            pthread_mutex_unlock (&gaOsixTsk[u4Idx].EvtMutex);
            return (OSIX_SUCCESS);
        }

        pthread_cond_wait (&gaOsixTsk[u4Idx].EvtCond,
                           &gaOsixTsk[u4Idx].EvtMutex);
    }
}

void base_event_show(void* eid)
{
	int i;
	fwk_eventList_t* pEid = (fwk_eventList_t*)eid;
	if (pEid) {
		fwk_showQueue(pEid->qid);
		fwk_showCond(pEid->cid);
		fwk_showMutex(pEid->mid);
	} else {
		for (i = 0; i < FWK_EVENT_MAX_LIMIT; ++i) {
			if (gFwkEventList[i].qid) {
				printf("Event[%i]: ", i);
				fwk_showEvent(&gFwkEventList[i]);
			}
		}
	}
}
