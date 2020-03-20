#ifndef FWK_TASK_MUTEX_H_
#define FWK_TASK_MUTEX_H_

#include <fwk/basic/types.h>
#include <fwk/basic/error.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

struct timespec fwk_msToTs(int timeout, clockid_t clk_id);
struct timespec fwk_msDeltaTs(int timeout, clockid_t clk_id, struct timespec* baseTs);
int fwk_msDeltaRemain(int timeout, clockid_t clk_id, struct timespec* baseTs);

#if defined(DEBUG) || !(defined(RELEASE))
#define CHECK(rc) if (rc) printf("Error %s-%i: rc=%i\n", __func__, __LINE__, rc)
#else
#define CHECK(rc)
#endif

#define FWK_MUTEX_NAME_LEN 8
typedef struct {
	char name[FWK_MUTEX_NAME_LEN];
	int pshared; //PTHREAD_PROCESS_PRIVATE(0) | PTHREAD_PROCESS_SHARED(1)
	int type; //PTHREAD_MUTEX_DEFAULT(0) | PTHREAD_MUTEX_NORMAL(0) | PTHREAD_MUTEX_ERRORCHECK(1) | PTHREAD_MUTEX_RECURSIVE(2) | PTHREAD_MUTEX_ERRORCHECK_NP
	int protocol; //PTHREAD_PRIO_NONE(0) | PTHREAD_PRIO_INHERIT(1) | PTHREAD_PRIO_PROTECT(2)
	int prioceiling; //priority ceiling, in case of protocol inherit or protect
	int robust; //PTHREAD_MUTEX_STALLED(0) | PTHREAD_MUTEX_ROBUST(1)
	int robust_np; //PTHREAD_MUTEX_STALLED_NP(0) | PTHREAD_MUTEX_ROBUST_NP(1)
}fwk_mutexAttr_t;
/*
Create mutex
Input:
	pMAttr: mutex attribute pointer, NULL means using default setting
Output:
	pMid: mutex ID pointer, NULL means ignore. Find it with fwk_mutexID(name)
Return:
	0 if succeed
	error code defined in fwk_errCode_t if failed
*/
int fwk_createMutex(fwk_mutexAttr_t* pMAttr, void** pMid);
int fwk_deleteMutex(void* mid);

/*
Lock mutex
Input:
	mid: mutex ID, acquire from fwk_createMutex or fwk_mutexID
	timeout: positive means waiting millisecond, 0 means no wait, negative means wait for ever
Return:
	0 if succeed
	error code defined in fwk_errCode_t if failed
*/
int fwk_lockMutex(void* mid, int timeout);
int fwk_unlockMutex(void* mid);
void* fwk_mutexID(char* name);
void fwk_showMutex(void* mid);

#define FWK_COND_NAME_LEN 8
typedef struct {
	char name[FWK_COND_NAME_LEN];
	int pshared; //PTHREAD_PROCESS_PRIVATE(0) | PTHREAD_PROCESS_SHARED(1)
	int clock; //CLOCK_REALTIME(0) | CLOCK_MONOTONIC(1) | CLOCK_PROCESS_CPUTIME_ID(2) | CLOCK_THREAD_CPUTIME_ID(3)
}fwk_condAttr_t;
/*
Create condition
Input:
	pCAttr: condition attribute pointer, NULL means using default setting
Output:
	pCid: condition ID pointer, NULL means ignore. Find it with fwk_condID(name)
Return:
	0 if succeed
	error code defined in fwk_errCode_t if failed
*/
int fwk_createCond(fwk_condAttr_t* pCAttr, void** pCid);
int fwk_deleteCond(void* cid);

/*
Create condition
Input:
	cid: condition ID, acquire from fwk_createCond or fwk_condID
	mid: mutex ID, used for condition wait critical region, acquire from fwk_createMutex or fwk_mutexID
	timeout: positive means waiting millisecond, 0 means no wait, negative means wait for ever
Return:
	0 if succeed
	error code defined in fwk_errCode_t if failed
*/
int fwk_waitCond(void* cid, void* mid, int timeout);
int fwk_wakeupCond(void* cid);
int fwk_getCondClock(void* cid);
void* fwk_condID(char* name);
void fwk_showCond(void* cid);

#define FWK_SEMA4_NAME_LEN 16
typedef struct {
	char name[FWK_SEMA4_NAME_LEN];
	int pshared; //negative - processes; zero - thread internal, default; positive - threads
	unsigned int value; //initial counter, default 0
}fwk_sema4Attr_t;
/*
Create semaphore
Input:
	pSAttr: semaphore attribute pointer, NULL means using default setting
Output:
	pSid: semaphore ID pointer, NULL means ignore. Find it with fwk_semaphoreID(name)
Return:
	0 if succeed
	error code defined in fwk_errCode_t if failed
*/
int fwk_createSemaphore(fwk_sema4Attr_t* pSAttr, void** pSid);
int fwk_deleteSemaphore(void* sid);
int fwk_takeSemaphore(void* sid, int timeout);
int fwk_giveSemaphore(void* sid);
void* fwk_semaphoreID(char* name);
void fwk_showSemaphore(void* sid);

#ifdef __cplusplus
}
#endif

#endif
