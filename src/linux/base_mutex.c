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

struct timespec fwk_msToTs(int timeout, clockid_t clk_id)
{
	struct timespec ts;
	clock_gettime(clk_id, &ts);
	ts.tv_sec += (timeout / 1000);
	ts.tv_nsec += (timeout % 1000) * 1000000000;
	if (ts.tv_nsec >= 1000000000) {
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000;
	}
	return ts;
}

struct timespec fwk_msDeltaTs(int timeout, clockid_t clk_id, struct timespec* baseTs)
{
	struct timespec ts;
	clock_gettime(clk_id, &ts);
	ts.tv_sec += (timeout / 1000);
	ts.tv_nsec += (timeout % 1000) * 1000000000;
	if (baseTs) {
		ts.tv_sec -= baseTs->tv_sec;
		ts.tv_nsec -= baseTs->tv_nsec;
	}
	if (ts.tv_nsec >= 1000000000) {
		ts.tv_sec++;
		ts.tv_nsec -= 1000000000;
	} else if (ts.tv_nsec < 0) {
		ts.tv_sec--;
		ts.tv_nsec += 1000000000;
	}
	ts.tv_sec = (ts.tv_sec < 0) ? 0 : ts.tv_sec;
	return ts;
}

int fwk_msDeltaRemain(int timeout, clockid_t clk_id, struct timespec* baseTs)
{
	int remain = timeout;
	if (timeout > 0) {
		struct timespec ts;
		clock_gettime(clk_id, &ts);
		remain = (ts.tv_sec - baseTs->tv_sec) * 1000;
		remain += (ts.tv_nsec - baseTs->tv_nsec) / 1000000000;
		remain = timeout - remain;
		remain = (remain > 0) ? remain : 0;
	}
	return remain;
}

int fwk_getDefaultMutexPrioCeiling(void)
{
	int rc = 0;
	int priority = 0;

	pthread_mutexattr_t mutexAttr;
	rc += pthread_mutexattr_init(&mutexAttr);
	rc += pthread_mutexattr_getprioceiling(&mutexAttr, &priority);

	if (rc) {
		fwk_basictrace_print(FWK_BASICTRACE_MODULE_FWK, FWK_BASICTRACE_LEVEL_ERR,
			"%s failed, rc %i, errno %i\n", __func__, rc, errno);
	}
	return priority;
}

#define FWK_MUTEX_MAX_LIMIT 200
typedef struct {
	fwk_mutexAttr_t attr;
	pthread_mutex_t mutex;
	int used;
}fwk_mutexList_t;
static fwk_mutexList_t gFwkMutexList[FWK_MUTEX_MAX_LIMIT];
static int gCurMutexCount = 0;

int fwk_createMutex(fwk_mutexAttr_t* pMAttr, void** pMid)
{
	int i, rc = 0, mcbIdx;
	pthread_mutexattr_t mutexAttr;
	fwk_mutexAttr_t fma;
	fwk_mutexList_t* pFML = NULL;

	for (i = 0; i < FWK_MUTEX_MAX_LIMIT; ++i) {
		if (gFwkMutexList[i].used == 0) {
			pFML = &gFwkMutexList[i];
			mcbIdx = i;
			break;
		}
	}
	if (!pFML) { //gCurMutexCount > FWK_MUTEX_MAX_LIMIT
		fwk_basictrace_print(FWK_BASICTRACE_MODULE_FWK, FWK_BASICTRACE_LEVEL_ERR,
			"No mutex block to alloc, try to enlarge FWK_MUTEX_MAX_LIMIT.\n");
		return FWK_E_FULL;
	}
	fwk_memmgmt_set(pFML, 0, sizeof(*pFML));

	if (!pMAttr) {
		sprintf(fma.name, "Mutex%i", mcbIdx);
		fma.pshared = PTHREAD_PROCESS_PRIVATE;
		fma.type = PTHREAD_MUTEX_DEFAULT;
		fma.protocol = PTHREAD_PRIO_NONE; //prevent priority invert: PROTECT
		fma.prioceiling = fwk_getDefaultMutexPrioCeiling();
		fma.robust = PTHREAD_MUTEX_STALLED;
		fma.robust_np = PTHREAD_MUTEX_STALLED_NP;
		pMAttr = &fma;
	}
	rc += pthread_mutexattr_init(&mutexAttr);
	CHECK(rc);
	rc += pthread_mutexattr_setpshared(&mutexAttr, pMAttr->pshared);
	CHECK(rc);
	rc += pthread_mutexattr_settype(&mutexAttr, pMAttr->type);
	CHECK(rc);
	rc += pthread_mutexattr_setprotocol(&mutexAttr, pMAttr->protocol);
	CHECK(rc);
	rc += pthread_mutexattr_setprioceiling(&mutexAttr, pMAttr->prioceiling);
	CHECK(rc);
	rc += pthread_mutexattr_setrobust(&mutexAttr, pMAttr->robust);
	CHECK(rc);
	rc += pthread_mutexattr_setrobust_np(&mutexAttr, pMAttr->robust_np);
	CHECK(rc);
	if (rc) {
		fwk_basictrace_print(FWK_BASICTRACE_MODULE_FWK, FWK_BASICTRACE_LEVEL_WRN,
			"pthread_mutexattr_set failed, rc %i, errno %i\n", rc, errno);
	}

	rc = pthread_mutex_init (&(pFML->mutex), &mutexAttr);
	CHECK(rc);
	if (rc) {
		fwk_basictrace_print(FWK_BASICTRACE_MODULE_FWK, FWK_BASICTRACE_LEVEL_ERR,
			"pthread_mutex_init failed, rc %i, errno %i\n", rc, errno);
		rc = pthread_mutex_destroy(&(pFML->mutex));
		CHECK(rc);
		return FWK_E_INTERNAL;
	}
	rc = pthread_mutexattr_destroy(&mutexAttr);
	CHECK(rc);
	fwk_memmgmt_cpy(&pFML->attr, pMAttr, sizeof(*pMAttr));
	pFML->used = 1;
	if (pMid) *pMid = pFML;
	++gCurMutexCount;
	return rc;
}

int fwk_deleteMutex(void* mid)
{
	int rc = FWK_E_PARAM;
	if (!mid) return rc;
	fwk_mutexList_t* p = (fwk_mutexList_t*)mid;
	rc = pthread_mutex_lock(&(p->mutex));
	if (!rc) {
		rc = pthread_mutex_unlock(&(p->mutex));
		CHECK(rc);
	}
	if (!rc || rc == EINVAL) { //EINVAL: mutex owner thread has exit
		rc = pthread_mutex_destroy(&(p->mutex));
		CHECK(rc);
		if (rc) {
			fwk_basictrace_print(FWK_BASICTRACE_MODULE_FWK, FWK_BASICTRACE_LEVEL_ERR,
				"pthread_mutex_destroy failed[%s], rc %i, errno %i\n", p->attr.name, rc, errno);
			return FWK_E_INTERNAL;
		}
		p->used = 0;
		--gCurMutexCount;
	}
	return rc;
}

int fwk_lockMutex(void* mid, int timeout)
{
	int rc = FWK_E_INTERNAL;
	fwk_mutexList_t* p = (fwk_mutexList_t*)mid;
	if (!p) return FWK_E_PARAM;
	if (timeout < 0) {
		rc = pthread_mutex_lock(&(p->mutex));
		CHECK(rc);
	} else if (timeout == 0) {
		rc = pthread_mutex_trylock(&(p->mutex));
		CHECK(rc);
	} else {
		struct timespec ts = fwk_msToTs(timeout, CLOCK_MONOTONIC);
		rc = pthread_mutex_timedlock(&(p->mutex), &ts);
		CHECK(rc);
	}
	return rc;
}

int fwk_unlockMutex(void* mid)
{
	fwk_mutexList_t* p = (fwk_mutexList_t*)mid;
	if (!p) return FWK_E_PARAM;
	int rc = pthread_mutex_unlock(&(p->mutex));
	return rc;
}

void* fwk_mutexID(char* name)
{
	int i;
	for (i = 0; i < FWK_MUTEX_MAX_LIMIT; ++i) {
		if (!gFwkMutexList[i].used)  continue;
		if (!strcmp(gFwkMutexList[i].attr.name, name)) {
			return &gFwkMutexList[i];
		}
	}
	return NULL;
}

void* fwk_rawMid(void* mid)
{
	fwk_mutexList_t* p = (fwk_mutexList_t*)mid;
	if (!p) return NULL;
	return &(p->mutex);
}

void fwk_showMutex(void* mid)
{
	int i;
	fwk_mutexList_t* p = (fwk_mutexList_t*)mid;
	if (p) {
		//fwk_basictrace_print(FWK_BASICTRACE_MODULE_FWK, FWK_BASICTRACE_LEVEL_INF,
		printf(
			"mutex attr:[name=%s, pshared=%i, type=%i, protocol=%i, prioceiling=%i, robust=%i, robust_np=%i]\n",
			p->attr.name,
			p->attr.pshared,
			p->attr.type,
			p->attr.protocol,
			p->attr.prioceiling,
			p->attr.robust,
			p->attr.robust_np);
		printf("mutex state:[lock=%i, count=%i, owner=%i, kind=%i, nusers=%i, spins=%i",
			p->mutex.__data.__lock, p->mutex.__data.__count, p->mutex.__data.__owner,
			p->mutex.__data.__kind, p->mutex.__data.__nusers, p->mutex.__data.__spins);
#if __WORDSIZE == 64
		__pthread_list_t* next = &(p->mutex.__data.__list);
#elif __WORDSIZE == 32
		__pthread_slist_t* next = &(p->mutex.__data.__list);
#else
#error "Unsupported architecture..."
#endif
		while (next) {
			printf(", ->%p", (void*)next);
			next = next->__next;
		}
		printf(", size[");
		for (i = 0; i < __SIZEOF_PTHREAD_MUTEX_T; ++i) {
			if (p->mutex.__size[i]) {
				printf(", %i=%i", i, p->mutex.__size[i]);
			}
		}
		printf("]]\n");
	} else {
		for (i = 0; i < FWK_MUTEX_MAX_LIMIT; ++i) {
			if (gFwkMutexList[i].used) {
				printf("%i: ", i);
				fwk_showMutex(&gFwkMutexList[i]);
			}
		}
	}
}

#define FWK_COND_MAX_LIMIT 100
typedef struct {
	fwk_condAttr_t attr;
	pthread_cond_t cond;
	void* mid;
	int used;
}fwk_condList_t;
static fwk_condList_t gFwkCondList[FWK_COND_MAX_LIMIT];
static int gCurCondCount = 0;

int fwk_createCond(fwk_condAttr_t* pCAttr, void** pCid)
{
	int i, rc = 0, ccbIdx;
	pthread_condattr_t condAttr;
	fwk_condAttr_t fca;
	fwk_condList_t* pFCL = NULL;

	for (i = 0; i < FWK_COND_MAX_LIMIT; ++i) {
		if (gFwkCondList[i].used == 0) {
			pFCL = &gFwkCondList[i];
			ccbIdx = i;
			break;
		}
	}
	if (!pFCL) { //gCurCondCount > FWK_COND_MAX_LIMIT
		fwk_basictrace_print(FWK_BASICTRACE_MODULE_FWK, FWK_BASICTRACE_LEVEL_ERR,
			"No cond block to alloc, try to enlarge FWK_COND_MAX_LIMIT.\n");
		return FWK_E_FULL;
	}
	fwk_memmgmt_set(pFCL, 0, sizeof(*pFCL));

	if (!pCAttr) {
		sprintf(fca.name, "Cond%i", ccbIdx);
		fca.pshared = PTHREAD_PROCESS_PRIVATE;
		fca.clock = CLOCK_REALTIME;
		pCAttr = &fca;
	}
	rc += pthread_condattr_init(&condAttr);
	CHECK(rc);
	rc += pthread_condattr_setpshared(&condAttr, pCAttr->pshared);
	CHECK(rc);
	rc += pthread_condattr_setclock(&condAttr, pCAttr->clock);
	CHECK(rc);
	if (rc) {
		fwk_basictrace_print(FWK_BASICTRACE_MODULE_FWK, FWK_BASICTRACE_LEVEL_WRN,
			"%s pthread_condattr_set failed, rc %i, errno %i\n", __func__, rc, errno);
	}

	rc = pthread_cond_init (&(pFCL->cond), &condAttr);
	CHECK(rc);
	if (rc) {
		fwk_basictrace_print(FWK_BASICTRACE_MODULE_FWK, FWK_BASICTRACE_LEVEL_ERR,
			"%s pthread_cond_init failed, rc %i, errno %i\n", __func__, rc, errno);
		rc = fwk_deleteMutex(pFCL->mid);
		CHECK(rc);
		//rc = pthread_cond_destroy(&(pFCL->cond));
		return FWK_E_INTERNAL;
	}
	rc = pthread_condattr_destroy(&condAttr);
	CHECK(rc);
	fwk_memmgmt_cpy(&pFCL->attr, pCAttr, sizeof(*pCAttr));
	pFCL->used = 1;
	if (pCid) *pCid = pFCL;
	++gCurCondCount;
	return rc;
}

int fwk_deleteCond(void* cid)
{
	int rc = FWK_E_PARAM;
	if (!cid) return rc;
	fwk_condList_t* p = (fwk_condList_t*)cid;
	rc = pthread_cond_destroy(&(p->cond));
	p->used = 0;
	--gCurCondCount;
	return rc;
}

int fwk_waitCond(void* cid, void* mid, int timeout)
{
	int rc = FWK_E_INTERNAL;
	fwk_condList_t* p = (fwk_condList_t*)cid;
	if (!p) return FWK_E_PARAM;
	pthread_mutex_t* rawMid = (pthread_mutex_t*)fwk_rawMid(mid);
	if (timeout >= 0) {
		struct timespec ts = fwk_msToTs(timeout, p->attr.clock);
		rc = pthread_cond_timedwait(&(p->cond), rawMid, &ts);
		CHECK(rc);
	} else {
		rc = pthread_cond_wait(&(p->cond), rawMid);
		CHECK(rc);
	}
	return rc;
}

int fwk_wakeupCond(void* cid)
{
	fwk_condList_t* p = (fwk_condList_t*)cid;
	if (!p) return FWK_E_PARAM;
	int rc = pthread_cond_signal(&(p->cond));
	//int rc = pthread_cond_broadcast(&(p->cond));
	CHECK(rc);
	return rc;
}

int fwk_getCondClock(void* cid)
{
	fwk_condList_t* p = (fwk_condList_t*)cid;
	if (!p) return FWK_E_PARAM;
	return p->attr.clock;
}

void* fwk_condID(char* name)
{
	int i;
	for (i = 0; i < FWK_COND_MAX_LIMIT; ++i) {
		if (!gFwkCondList[i].used)  continue;
		if (!strcmp(gFwkCondList[i].attr.name, name)) {
			return &gFwkCondList[i];
		}
	}
	return NULL;
}

void fwk_showCond(void* cid)
{
	int i;
	fwk_condList_t* p = (fwk_condList_t*)cid;
	if (p) {
		//fwk_basictrace_print(FWK_BASICTRACE_MODULE_FWK, FWK_BASICTRACE_LEVEL_ERR,
		printf(
			"cond attr[name=%s, pshared=%i, clock=%i]\n",
			p->attr.name,
			p->attr.pshared,
			p->attr.clock
			);
		printf("cond state: [lock=%i, futex=%i, SN(total %lld, wakeup %lld, woken %lld), mutex %p, nwaiters=%i, bc-SN %i",
			p->cond.__data.__lock, p->cond.__data.__futex,
			p->cond.__data.__total_seq, p->cond.__data.__wakeup_seq, p->cond.__data.__woken_seq,
			p->cond.__data.__mutex, p->cond.__data.__nwaiters, p->cond.__data.__broadcast_seq);
		printf(", size[");
		for (i = 0; i < __SIZEOF_PTHREAD_COND_T; ++i) {
			if (p->cond.__size[i]) printf(", %i=%i", i, p->cond.__size[i]);
		}
		printf("]]\n");

	} else {
		for (i = 0; i < FWK_COND_MAX_LIMIT; ++i) {
			if (gFwkCondList[i].used) {
				printf("Cond[%i]: ", i);
				fwk_showCond(&gFwkCondList[i]);
			}
		}
	}
}

#define FWK_SEMA4_MAX_LIMIT 20
typedef struct {
	fwk_sema4Attr_t attr;
	sem_t sema;
	sem_t* pSema;
	int used;
}fwk_sema4List_t;
static fwk_sema4List_t gFwkSema4List[FWK_SEMA4_MAX_LIMIT];
static int gCurSema4Count = 0;

int fwk_createSemaphore(fwk_sema4Attr_t* pSAttr, void** pSid)
{
	int i, rc = 0, scbIdx;
	fwk_sema4Attr_t fsa;
	fwk_sema4List_t* pFSL = NULL;

	for (i = 0; i < FWK_SEMA4_MAX_LIMIT; ++i) {
		if (gFwkSema4List[i].used == 0) {
			pFSL = &gFwkSema4List[i];
			scbIdx = i;
			break;
		}
	}
	if (!pFSL) {
		fwk_basictrace_print(FWK_BASICTRACE_MODULE_FWK, FWK_BASICTRACE_LEVEL_ERR,
			"No mutex block to alloc, try to enlarge FWK_SEMA4_MAX_LIMIT.\n");
		return FWK_E_FULL;
	}
	fwk_memmgmt_set(pFSL, 0, sizeof(*pFSL));

	if (!pSAttr) {
		sprintf(fsa.name, "Sema%i", scbIdx);
		fsa.pshared = 0;
		fsa.value = 0;
		pSAttr = &fsa;
	}
	if (pFSL->attr.pshared < 0) {
		pFSL->pSema = sem_open(pFSL->attr.name, O_CREAT|O_EXCL, 0644, pFSL->attr.value);
	} else {
		pFSL->pSema = &(pFSL->sema);
		rc = sem_init(pFSL->pSema, pFSL->attr.pshared, pFSL->attr.value);
	}
	if (rc || !pFSL->pSema) {
		fwk_basictrace_print(FWK_BASICTRACE_MODULE_FWK, FWK_BASICTRACE_LEVEL_ERR,
			"%s failed, rc %i, errno %i\n", __func__, rc, errno);
		//no resource need release
		return FWK_E_INTERNAL;
	}
	fwk_memmgmt_cpy(&pFSL->attr, pSAttr, sizeof(*pSAttr));
	pFSL->used = 1;
	if (pSid) *pSid = pFSL;
	++gCurSema4Count;
	return rc;
}

int fwk_deleteSemaphore(void* sid)
{
	int rc = FWK_E_INTERNAL;
	if (!sid) return FWK_E_PARAM;
	fwk_sema4List_t* p = (fwk_sema4List_t*)sid;
	if (p->attr.pshared < 0) {
		rc = sem_close(p->pSema);
		rc = sem_unlink(p->attr.name);
	} else {
		rc = sem_destroy(p->pSema);
	}
	p->used = 0;
	--gCurSema4Count;
	return rc;
}

int fwk_takeSemaphore(void* sid, int timeout)
{
	int rc = FWK_E_INTERNAL;
	fwk_sema4List_t* p = (fwk_sema4List_t*)sid;
	if (!p) return FWK_E_PARAM;
	if (timeout < 0) {
		rc = sem_wait(p->pSema);
	} else if (timeout == 0) {
		rc = sem_trywait(p->pSema);
	} else {
		int i;
		for (i = 0; i < timeout/10; ++i) {
			rc = sem_trywait(p->pSema);
			if (!rc) break;
			usleep(10*1000);
		}
	}
	return rc;
}

int fwk_giveSemaphore(void* sid)
{
	fwk_sema4List_t* p = (fwk_sema4List_t*)sid;
	if (!p) return FWK_E_PARAM;
	int rc = sem_post(p->pSema);
	return rc;
}

void* fwk_semaphoreID(char* name)
{
	int i;
	for (i = 0; i < gCurSema4Count; ++i) {
		if (!strcmp(gFwkSema4List[i].attr.name, name)) {
			return &gFwkSema4List[i];
		}
	}
	return NULL;
}

void fwk_showSemaphore(void* sid)
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
