
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <sys/syscall.h>


#include <errno.h>
#include <string.h>
#include <limits.h>

#include <fwk/task/task.h>
#include <fwk/basic/basictrace.h>
#include <fwk/memmgmt/memmgmt.h>
#include <fwk/task/mutex.h>
#include <fwk/task/queue.h>
#include <fwk/task/event.h>


static fwk_taskList_t gFwkTaskList[FWK_TASK_MAX_LIMIT];
static int gCurTaskCount = 0;

static clock_t      gStartTicks;

typedef tPthreadQ  *tPthreadQId;

tOsixTsk            gaOsixTsk[OSIX_MAX_TSKS + 1];
/* 0th element is invalid and not used */

tOsixSem            gaOsixSem[OSIX_MAX_SEMS + 1];
/* 0th element is invalid and not used */

tOsixQue            gaOsixQue[OSIX_MAX_QUES + 1];
/* 0th element is invalid and not used */

pthread_mutex_t     gOsixMutex;


UINT32 base_task_init ()
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

UINT32 base_task_info_find (CHAR au1Name[], UINT32 u4RscType, UINT4 *pu4RscId)
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

UINT32 base_task_info_add (CHAR au1Name[], UINT32 u4RscType, UINT4 u4RscId)
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
VOID base_task_info_del (UINT32 u4RscType, UINT4 u4RscId)
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

/************************************************************************/
/*  Function Name   : OsixTaskWrapper                                   */
/*  Description     : Intermediate function between OsixTskCrt and      */
/*                  : application entry point function, which serves    */
/*                  : to prevent the application from kicking off even  */
/*                  : before CreateTask has completed. The synch is     */
/*                  : accomplished through the use of a per task mutex. */
/*                  : The mutex is 'taken' just before pthread_create   */
/*                  : and is given up just before close of CreateTask.  */
/*  Input(s)        : pArg - task arguments passed here.                */
/*  Output(s)       : None                                              */
/*  Returns         : None                                              */
/************************************************************************/

static VOID *base_task_wrapper (void *pArg)
{
    void                (*TaskPtr) (INT4);
    INT4                i4Arg;
    tOsixTsk           *pTsk = (tOsixTsk *) pArg;
    INT4  i4Prio;
    static INT4 i4Policy;
    static struct sched_param param;
    pid_t tid;

    /* Waits till OsixCreateTask releases the lock */
    pthread_mutex_lock (&(pTsk->TskMutex));

    /* OsixCreateTask is complete, now releases the lock */
    pthread_mutex_unlock (&(pTsk->TskMutex));

    TaskPtr = pTsk->pTskStrtAddr;
    i4Arg = pTsk->u4Arg;


    pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

#if 0
    pthread_getschedparam(pthread_self(), &i4Policy, &param);
    if (i4Policy == SCHED_OTHER)
    {
        i4Prio = pTsk->u4Prio;
        tid = syscall(SYS_gettid);
        /* TODO: Do not set the priority in MXUsh31913. Uncomment it if needed in future. */
/*        setpriority(PRIO_PROCESS, tid, OSIX_TO_POSIX_NICE_PRIO(i4Prio)); */
        /* use -11 temporary here, we think the BONUS is 10 */
        setpriority(PRIO_PROCESS, tid, -11);
    }
#endif

    /* Call the actual application Entry Point function. */
    (*TaskPtr) (i4Arg);

    return ((void *) NULL);
}


UINT32 base_task_create (const CHAR *name, UINT32 policy, UINT32 priority, UINT32 stacksize, 
                VOID (*func) (VOID *),VOID *funcargs, base_task_id * taskid)
{
    UINT32               u4Arg;
    CHAR               tname[BASE_TASK_NAME_LEN], u1Index;
    INT4 i4Policy;

    struct sched_param  SchedParam;
    pthread_attr_t      Attr;
    tOsixTsk           *pTsk = 0;
    UINT32               u4Idx;
    INT4                i4OsPrio;

    TASK_INF("base_task_create name:%s, policy:%d, priority:%d, stacksize:%d\r\n", 
        name, policy, priority, stacksize);

    /*can not use memcpy, it will cause memory overflow*/
    MEM_SET ((CHAR *) tname, '\0', BASE_TASK_NAME_LEN);
    for (int i = 0; ((i < BASE_TASK_NAME_LEN) && (name[i] != '\0')); i++)
    {
        tname[i] = name[i];
    }   
/*
	int len = strlen(name);
	if (len ! = 0) 
	{
		len = (len < BASE_TASK_NAME_LEN) ? len : (BASE_TASK_NAME_LEN - 1);
		strncpy(name_tmp, name, len);
	}
*/
    if (base_task_info_find (tname, OSIX_TSK, u4Idx) == OSIX_SUCCESS)
    {
        return BASE_E_EXISTS;    /* Task by this name already exists */
    }
    
    if (base_task_info_add (tname, OSIX_TSK, 0) != BASE_E_NONE)
    {
        return BASE_E_FAIL;
    }

    if (policy = TASK_SCHED_OTHER)
    {
        i4Policy = SCHED_OTHER;
    }
    else if (policy = TASK_SCHED_RR)
    {
        i4Policy = SCHED_RR;
    }
    else if (policy = TASK_SCHED_FIFO)
    {
        i4Policy = SCHED_FIFO;
    }
    else
    {
        i4Policy = SCHED_RR;/*default*/
    }


    SchedParam.sched_priority = priority;
    pthread_attr_init (&Attr);
    
    pthread_attr_setstacksize (&Attr, stacksize);
    pthread_attr_setinheritsched (&Attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy (&Attr, i4Policy);
    pthread_attr_setschedparam (&Attr, &SchedParam);

    base_task_info_find (au1Name, OSIX_TSK, &u4Idx);
    pTsk = &gaOsixTsk[u4Idx];
    *taskid = (tOsixTaskId) u4Idx;

    pTsk->u4Prio = (UINT32) priority;
    pTsk->taskstacksize = stacksize;

    /* CondVars and associated mutexes,
     * (1) for task creation synch.
     * (2) for Events mechanism
     */
    pthread_cond_init (&(pTsk->EvtCond), NULL);
    pthread_mutex_init (&(pTsk->TskMutex), NULL);
    pthread_mutex_init (&(pTsk->EvtMutex), NULL);

    /* Store the TaskStartAddr and Args in TCB for further reference */
    pTsk->pTskStrtAddr = func;
    pTsk->u4Arg = funcargs;

    /* We need the threadId to be stored in pTsk before pthreads gives
     * control to the entry point. So we use a "stub" function base_task_wrapper
     * and use condvars to ensure an orderly creation.
     */
    pthread_mutex_lock (&(pTsk->TskMutex));
    if (pthread_create (&(pTsk->ThrId), &Attr, base_task_wrapper, (void *) pTsk))
    {
        pthread_mutex_unlock (&(pTsk->TskMutex));

        pthread_mutex_destroy (&(pTsk->TskMutex));
        pthread_mutex_destroy (&(pTsk->EvtMutex));
        pthread_cond_destroy (&(pTsk->EvtCond));

        base_task_info_del (OSIX_TSK, u4Idx);
        return (BASE_E_FAIL);
    }
    pthread_mutex_unlock (&(pTsk->TskMutex));

    return (BASE_E_NONE);    
}

/************************************************************************
 *  Function Name   : OsixDeleteTask
 *  Description     : This deletes a task of a specified name.
 *  Input           : u4Node     - Unused.
 *                    au1TskName - Name of task to be deleted.
 *  Returns         : OSIX_SUCCESS/OSIX_FAILURE.
 ************************************************************************/
UINT4
base_task_delete (const UINT1 au1TskName[4])
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
        TASK_INF("OsixTskDel (0x%x)\r\n", (UINT4) TskId);
        pthread_mutex_destroy (&(gaOsixTsk[(UINT4) TskId].TskMutex));
        pthread_mutex_destroy (&(gaOsixTsk[(UINT4) TskId].EvtMutex));
        pthread_cond_destroy (&(gaOsixTsk[(UINT4) TskId].EvtCond));
        
        base_task_info_del(OSIX_TSK, (UINT4) TskId);
        
        pthread_detach (gaOsixTsk[(UINT4) TskId].ThrId);
        pthread_cancel (gaOsixTsk[(UINT4) TskId].ThrId);

        return (OSIX_SUCCESS);
    }
    return (OSIX_FAILURE);
}

UINT32 base_task_delay (UINT4 u4Duration)
{
    UINT4               u4Sec;
    struct timespec     timeout;

    /* Assumption: OsixSTUPS2Ticks converts u4Duration to micro-seconds
     * We convert that into seconds and nano-seconds for the OS call.
     * If the values configured for OSIX_STUPS and OSIX_TPS change, this
     * code should be changed.
     */

    u4Sec = u4Duration / 1000000;
    timeout.tv_sec = u4Sec;
    timeout.tv_nsec = ((u4Duration - u4Sec * 1000000) * 1000);
    nanosleep (&timeout, NULL);

    return (OSIX_SUCCESS);
}
/*
pthread_yield() causes the calling thread to relinquish the CPU.  
The thread is placed at the end of the run queue for its static 
priority and another thread is scheduled to run.
*/
int base_task_yield()
{
	return pthread_yield();
}

/************************************************************************
 *  Function Name   : OsixGetTaskId
 *  Description     : This returns the TaskId given the task name.
 *  Input           : u4Node - Unused
 *                    au1TskName - Name of task.
 *  Output          : pTskId - Pointer to memory location containing TSK-ID.
 *  Returns         : OSIX_SUCCESS/OSIX_FAILURE.
 ************************************************************************/
UINT32 base_task_get (const UINT1 au1TskName[4], tOsixTaskId * pTskId)
{
    UINT1               au1Name[OSIX_NAME_LEN + 4], u1Index;

    MEMSET ((UINT1 *) au1Name, '\0', (OSIX_NAME_LEN + 4));
    for (u1Index = 0;
         ((u1Index < OSIX_NAME_LEN) && (au1TskName[u1Index] != '\0'));
         u1Index++)
    {
        au1Name[u1Index] = au1TskName[u1Index];
    }

    if (base_task_info_find(au1Name, OSIX_TSK, pTskId) == OSIX_SUCCESS)
    {
        return (OSIX_SUCCESS);
    }
    return (OSIX_FAILURE);
}
/************************************************************************
 *  Function Name   : OsixGetCurTaskId
 *  Description     : This returns the TaskId of caller task.
 *  Input           : None.
 *  Returns         : TSK-ID of caller.
 ************************************************************************/
UINT4 base_task_cur_get (void)
{
    tOsixTaskId         TskId;

    UINT4               u4Count;
    pthread_t           ThrId;

    TASK_INF("OsixTskIdSelf (0x%x)\r\n", (UINT4) pTskId);

    ThrId = pthread_self ();

    for (u4Count = 1; u4Count <= OSIX_MAX_TSKS; u4Count++)
    {
        if ((gaOsixTsk[u4Count].ThrId) == ThrId)
        {
            *pTskId = (tOsixTaskId) u4Count;
            return ((UINT4) TskId);
        }
    }
    return 0;

}


void base_task_show(fwk_taskID_t tid)
{
	int i;
	fwk_taskList_t* pTask = (fwk_taskList_t*)(tid);
	if (pTask) {
		//fwk_basictrace_print(FWK_BASICTRACE_MODULE_FWK, FWK_BASICTRACE_LEVEL_ERR,
		printf(
			"task name=%s, args=%p, policy=%i, priority=%i, "
			"resource[stackSize=%i, memPoolSize=%ld, queueSize=%i, queueDepth=%i], "
			"independent=%i, taskType=%i, loopTimes=%i, taskPause=%i\n",
			pTask->attr.name,
			pTask->attr.args,
			pTask->attr.policy,
			pTask->attr.priority,
			(int)pTask->attr.resource.stackSize,
			pTask->attr.resource.memPoolSize,
			pTask->attr.resource.queueSize,
			pTask->attr.resource.queueDepth,
			pTask->attr.independent,
			pTask->attr.taskType,
			pTask->attr.loopTimes,
			pTask->taskPause);
#if 0 //Display function pointer
		printf("func=%p, initFunc=%p; tid=%lx,\n",
			pTask->attr.func,
			pTask->attr.initFunc,
			(fwk_addr_t)pTask->tid);
#endif
		if (pTask->mid) fwk_showMutex(pTask->mid);
		if (pTask->cid) fwk_showCond(pTask->cid);
		if (pTask->qid) fwk_showQueue(pTask->qid);
	} else {
		for (i = 0; i < FWK_TASK_MAX_LIMIT; ++i) {
			if (gFwkTaskList[i].used != 0) {
				printf("Task[%i]: ", i);
				fwk_showTask(&gFwkTaskList[i]);
			}
		}
	}
}
