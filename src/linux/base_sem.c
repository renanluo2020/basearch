#include <fwk/task/mutex.h>
#include <fwk/basic/basictrace.h>
#include <fwk/memmgmt/memmgmt.h>

#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <string.h>


UINT32 base_sem_init ()
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


UINT32 base_sem_info_find (CHAR au1Name[], UINT32 u4RscType, UINT4 *pu4RscId)
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

UINT32 base_sem_info_add (CHAR au1Name[], UINT32 u4RscType, UINT4 u4RscId)
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
                    gaOsixSem[u4Idx].SemId = u4RscId;

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
VOID base_sem_info_del (UINT32 u4RscType, UINT4 u4RscId)
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


/************************************************************************
 *  Function Name   : OsixCreateSem
 *  Description     : This creates a sema4 of a given name.
 *  Input           : au1SemName  - Name of sema4.
 *                    u4InitialCount - Initial value of sema4.
 *                    u4Flags        - Unused.
 *  Output          : pSemId - Pointer to memory which contains SEM-ID
 *  Returns         : OSIX_SUCCESS/OSIX_FAILURE.
 ************************************************************************/
UINT32 base_sem_create (const UINT1 au1SemName[4], UINT4 u4InitialCount,
               UINT4 u4Flags, tOsixSemId * pSemId)
{
   UINT1               au1Name[OSIX_NAME_LEN + 4], u1Index;
   
   /*如果 pshared 的值为 0，那么信号量将被进程内的线程共享*/
    INT32 pshared = 0;
    
    MEMSET ((UINT1 *) au1Name, '\0', (OSIX_NAME_LEN + 4));
    for (u1Index = 0;
         ((u1Index < OSIX_NAME_LEN) && (au1SemName[u1Index] != '\0'));
         u1Index++)
    {
        au1Name[u1Index] = au1SemName[u1Index];
    }

    u4Flags = 0;                /* unused; */

    if (base_sem_info_find(au1Name, OSIX_SEM, (UINT4 *) pSemId) == OSIX_SUCCESS)
    {
        /* Semaphore by this name already exists. */
        return (OSIX_FAILURE);
    }

    /* For sem, the pThreads version of OsixRscAdd does not use */
    /* the last argument. So anything can be passed; we pass 0. */
    SEM_INF("OsixSemCrt (%s)\r\n", au1Name);

    if (sem_init ((sem_t *) * pSemId, pshared, u4InitialCount))
    {
        base_sem_info_del(OSIX_SEM, (UINT4) *pSemId);
        return (OSIX_FAILURE);
    }
    
    if (base_sem_info_add (au1Name, OSIX_SEM, 0) == OSIX_FAILURE)
    {
        return (OSIX_FAILURE);
    }

    /* Implementation assumes that mutex is created when u4InitialCount */
    /* is 1, otherwise sem is for task sync. OsixSemCrt creates binary  */
    /* sem in blocked state. So, if u4InitialCount is 1, give the sem.  */
    if (u4InitialCount == 1)
    {
      //  OsixSemGive (*pSemId);
    }
    return (OSIX_SUCCESS);

}


/************************************************************************
 *  Function Name   : OsixDeleteSem
 *  Description     : This deletes sema4 of a given name.
 *  Input           : u4Node   - Unused.
 *                    au1SemName - Name of sema4 to be deleted.
 *  Returns         : OSIX_SUCCESS/OSIX_FAILURE.
 ************************************************************************/
UINT4 base_sem_delete (UINT4 u4Node, const UINT1 au1SemName[4])
{
    tOsixSemId          SemId;
    UINT1               au1Name[OSIX_NAME_LEN + 4], u1Index;

    MEMSET ((UINT1 *) au1Name, '\0', (OSIX_NAME_LEN + 4));
    for (u1Index = 0;
         ((u1Index < OSIX_NAME_LEN) && (au1SemName[u1Index] != '\0'));
         u1Index++)
    {
        au1Name[u1Index] = au1SemName[u1Index];
    }

    u4Node = 0;                    /* unused; */
    if (base_sem_info_find(au1Name, OSIX_SEM, (UINT4 *) &SemId) == OSIX_SUCCESS)
    {
        SEM_INF("OsixSemDel (0x%lx)\r\n",
                   (UINT4) SemId);
        base_sem_info_del(OSIX_SEM, (UINT4) SemId);
        
        sem_destroy (SemId);

        return (OSIX_SUCCESS);
    }
    return (OSIX_FAILURE);
}

/************************************************************************
 *  Function Name   : OsixTakeSem
 *  Description     : This is called by appln.s to take a sema4.
 *  Input           : u4Node     - Unused.
 *                    au1SemName - Name of sema4.
 *                    u4Flags    - Unused
 *                    u4Timeout  - Unused
 *  Returns         : OSIX_SUCCESS/OSIX_FAILURE.
 ************************************************************************/
UINT32 base_sem_take (UINT4 u4Node, const UINT1 au1SemName[4],
             UINT4 u4Flags, UINT4 u4Timeout)
{
    tOsixSemId          SemId;
    UINT1               au1Name[OSIX_NAME_LEN + 4], u1Index;

    MEMSET ((UINT1 *) au1Name, '\0', (OSIX_NAME_LEN + 4));
    for (u1Index = 0;
         ((u1Index < OSIX_NAME_LEN) && (au1SemName[u1Index] != '\0'));
         u1Index++)
    {
        au1Name[u1Index] = au1SemName[u1Index];
    }


    if (base_sem_info_find(au1Name, OSIX_SEM, (UINT4 *) &SemId) == OSIX_SUCCESS)
    {
        SEM_INF( "OsixSemTake (0x%lx)\r\n",
               (UINT4) SemId);

        if (sem_wait (SemId) == 0);
        {
            return (OSIX_SUCCESS);
        }
    }
    return OSIX_FAILURE;
}

/************************************************************************
 *  Function Name   : OsixGiveSem
 *  Description     : This is called by appln.s to give a sema4.
 *  Input           : u4Node     - Unused.
 *                    au1SemName - Name of sema4.
 *  Returns         : OSIX_SUCCESS/OSIX_FAILURE.
 ************************************************************************/
UINT32 base_sem_give (UINT4 u4Node, const UINT1 au1SemName[4])
{
    tOsixSemId          SemId;
    UINT1               au1Name[OSIX_NAME_LEN + 4], u1Index;

    MEMSET ((UINT1 *) au1Name, '\0', (OSIX_NAME_LEN + 4));
    for (u1Index = 0;
         ((u1Index < OSIX_NAME_LEN) && (au1SemName[u1Index] != '\0'));
         u1Index++)
    {
        au1Name[u1Index] = au1SemName[u1Index];
    }

    if (base_sem_info_find (au1Name, OSIX_SEM, (UINT4 *) &SemId) == OSIX_SUCCESS)
    {
        SEM_INF("OsixSemGive (0x%lx)\r\n",
                   (UINT4) SemId);
        if (sem_post (SemId) == 0)
        {
            return (OSIX_SUCCESS);
        }
        SEM_ERR("OsixSemGive (0x%lx) failed\r\n", (UINT4) SemId);

    }
    return OSIX_FAILURE;
}

/************************************************************************
 *  Function Name   : OsixGetSemId
 *  Description     : This returns the SemId given the SemName.
 *  Input           : u4Node     - Unused.
 *                    au1SemName - Name of sema4.
 *  Output          : pId - Pointer to memory location containing SEM-ID.
 *  Returns         : OSIX_SUCCESS/OSIX_FAILURE.
 ************************************************************************/
UINT32 base_sem_get (UINT4 u4Node, const UINT1 au1SemName[4], tOsixSemId * pId)
{
    UINT1               au1Name[OSIX_NAME_LEN + 4], u1Index;

    MEMSET ((UINT1 *) au1Name, '\0', (OSIX_NAME_LEN + 4));
    for (u1Index = 0;
         ((u1Index < OSIX_NAME_LEN) && (au1SemName[u1Index] != '\0'));
         u1Index++)
    {
        au1Name[u1Index] = au1SemName[u1Index];
    }

    if (base_sem_info_find (au1Name, OSIX_SEM, (UINT4 *) pId) == OSIX_SUCCESS)
    {
        return (OSIX_SUCCESS);
    }
    return (OSIX_FAILURE);
}

void base_sem_show(void* sid)
{
	int i;
	fwk_sema4List_t* p = (fwk_sema4List_t*)sid;
	if (p) {
		//fwk_basictrace_print(FWK_BASICTRACE_MODULE_FWK, FWK_BASICTRACE_LEVEL_ERR,
		printf(
			"semaphore attr[name=%s, pshared=%i, value=%u]\n",
			p->attr.name,
			p->attr.pshared,
			p->attr.value
			);
		if (p->pSema) {
		printf("semaphore state: [");
		for (i = 0; i < __SIZEOF_SEM_T; ++i) {
			if (p->pSema->__size[i]) printf(", %i=%i", i, p->pSema->__size[i]);
		}
		printf("]\n");
		}
	} else {
		for (i = 0; i < FWK_SEMA4_MAX_LIMIT; ++i) {
			if (gFwkSema4List[i].used) {
				printf("%i: ", i);
				fwk_showSemaphore(&gFwkSema4List[i]);
			}
		}
	}
}
