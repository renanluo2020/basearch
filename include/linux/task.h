

#ifndef FWK_TASK_TASK_H_
#define FWK_TASK_TASK_H_

#include <pthread.h>
#include <fwk/task/mutex.h>
//#include <fwk/task/event.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define  BASE_TASK_NAME_LEN           16

#define  OSIX_NAME_LEN           4

#define OSIX_SCHED_RR             (1 << 16)
#define OSIX_SCHED_FIFO          (1 << 17)
#define OSIX_SCHED_OTHER       (1 << 18)

typedef enum {
    TASK_SCHED_RR            = 1,
    TASK_SCHED_FIFO         = 2,
    TASK_SCHED_OTHER      = 3             
} task_policy_t;

#define  OSIX_TSK                0
#define  OSIX_SEM                1
#define  OSIX_QUE                2
#define  OSIX_TRUE               1
#define  OSIX_FALSE              0
#define  OSIX_RSC_INV         NULL


/* system ID */
typedef uint8_t fwk_sysID_t;
/* task ID */
typedef void* fwk_taskID_t;
/* task handler function prototype */
typedef void *(*fwk_taskFunc_proto_t)(void *);
/* task name maximum size */
#define FWK_TASK_NAME_MAX_LEN 16

/*
 * Task resource reservation
 */
typedef struct
{
	/* stack size */
	size_t stackSize;
	/* mem pool size */
	fwk_addr_t memPoolSize;
	/* default event queue */
	uint16_t queueSize; /* max size for each event in the queue */
	uint16_t queueDepth; /* max number of events */
} fwk_taskRes_t;

typedef struct {
	char name[FWK_TASK_NAME_MAX_LEN];
	fwk_taskFunc_proto_t initFunc;
	fwk_taskFunc_proto_t func;
	void * args;
	uint8_t policy;
	uint8_t priority;
	fwk_taskRes_t resource;
	bool_t independent; //detach or fork
	int taskType; //0-Preemptive, 1-Normal
	int loopTimes;
}fwk_taskAttr_t;

typedef struct {
	fwk_taskAttr_t attr;
	pthread_t tid; //os thread id
	void* mid; //mutex id
	void* cid; // condition id
	void* qid; //queue id
	void* pid; //mem pool id
	int taskPause;
	int used;
}fwk_taskList_t;
#define FWK_TASK_MAX_LIMIT 100

extern void fwk_showTask(fwk_taskID_t tid);
extern int fwk_createTask(fwk_taskAttr_t* pAttr, fwk_taskID_t* pTid);
extern void* fwk_taskMemPool(fwk_taskID_t tid);
extern int fwk_terminateTask(void);

/*
 * Create preemptive task
 * These tasks that can preempt according to priority.
 * Input parameters:
 * 	tid: designated unique task ID if > 0; if input *tid == 0, then let system to allocate a TID
 * 	name: task name, unique within a system, as friendly name for debug
 * 	func: task function
 * 	args: arguments to pass to the start function
 * 	resource: task resource need
 * 	priority: task priority, higher priority > lower priority
 * 	policy: scheduling policy
 * 	independent: decides if manages resource independently from its parent task where it's created (in Linux, this means process v.s. thread).
 * Output:
 * 	tid: holds the allocated task ID if input *tid == 0.
 * Return:
 * 	0 if succeed
 * 	or error code as defined with fwk_taskErrCode_t
 */
extern int fwk_createPreemptiveTask(const char* const name, fwk_taskID_t * tid,
		fwk_taskFunc_proto_t func, void * args, uint8_t priority,
		uint8_t policy, fwk_taskRes_t resource, bool_t independent);

/*
 * Create normal task.
 * Normal task cannot preempt others and can be executed only if others cooperatively give up CPU, e.g. those tasks of CFS class in Linux.
 * In task scheduling, normal tasks shall always have lower priority than all real time tasks.
 * In a platform that supports only real-time tasks, this maybe implemented with all normal tasks at a low priority level.
 * In Linux platform, a priority can still be assigned, however, it may impact only the CPU time allocation.
 */
extern int fwk_createNormalTask(const char* const name, fwk_taskID_t * tid,
		fwk_taskFunc_proto_t func, void * args, uint8_t priority,
		fwk_taskRes_t resource, bool_t independent);
extern int fwk_createDisposableTask(const char* const name, fwk_taskID_t * tid,
		fwk_taskFunc_proto_t func, void * args, uint8_t priority,
		fwk_taskRes_t resource, bool_t independent);

/*
 * delete a task
 * Input:
 * 	tid: task ID of the task to delete
 * Return:
 * 	0 if succeed
 * 	or error code as defined with fwk_taskErrCode_t
 */
extern int fwk_deleteTask(fwk_taskID_t tid);

/*
 * voluntarily yield CPU
 */
extern int fwk_yieldTask(void);

/*
 * suspend & resume a specific task
 */
extern int fwk_suspendTask(fwk_taskID_t tid);

extern int fwk_resumeTask(fwk_taskID_t tid);

/*
 * Get ID of the task that calling this API
 */
extern fwk_taskID_t fwk_myTaskId(void);

/*
 * task ID, name identification mapping
 */
extern fwk_taskID_t fwk_taskId(const char* const name);

extern const char* fwk_taskName(fwk_taskID_t tid);

#ifdef __cplusplus
} /* extern C */
#endif

#endif /* FWK_TASK_TASK_H_ */
