#include <fwk/task/task.h>
#include <fwk/task/queue.h>
#include <fwk/task/event.h>
#include <fwk/basic/basictrace.h>
#include <fwk/memmgmt/memmgmt.h>
#include <fwk/timer/stw_mgmt.h>

#include <stdio.h>
#include <string.h>

#include <sched.h>
#include <unistd.h>
#include <errno.h>

#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include <fwk/cli/cli.h>
int gCliIdx = 0;
int cmd_test(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    int i;
    printf("svr log: %s (idx %i) called by \"%s\", %i arguments:", __func__, gCliIdx, command, argc);
    for (i = 0; i < argc; i++)
        printf(" %s,", argv[i]);
    printf("\n");
    cli_print(cli, "cli=%p, %s : %s increment to %i \n", (void*)cli, command, __func__, gCliIdx++);
    return CLI_OK;
}
int cmd_test_2(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    int i;
    printf("svr log: %s (idx %i) called by \"%s\", %i arguments:", __func__, gCliIdx, command, argc);
    for (i = 0; i < argc; i++)
        printf(" %s,", argv[i]);
    printf("\n");
    cli_print(cli, "cli=%p, %s : %s increment to %i \n", (void*)cli, command, __func__, gCliIdx++);
    return CLI_OK;
}

struct cli_user_cmd gCliUserCmd[] = {
    	{"test", cmd_test, "My 1st test command"},
    	{"test2", cmd_test_2, "customer 2nd user cmd"},
    	{NULL, NULL, NULL},
};

void* cliDemo(void* args)
{
    (void)(args);
    efwk_cli_start(1024*1024);
    return NULL;
}
void* mainLoop(void* args)
{
    (void)args;
    sleep(10);
    return NULL;
}
int initCliSvr(void)
{
  fwk_taskID_t tid;
  fwk_taskAttr_t tAttr = {"cliSvr", cliDemo, mainLoop, NULL, SCHED_FIFO, 50, {0, 20*1024, 0, 0}, 1, 0, -1};
  int rc = fwk_createTask(&tAttr, &tid);
  printf("cliServer tid: %p\n", tid);
  return rc;
}

extern unsigned long int fwk_rawTid(fwk_taskID_t tid);

int randomN(int N)
{
  struct timeval tpstart;
  gettimeofday(&tpstart, NULL);
  srand(tpstart.tv_usec);

  float num = N*rand()/(RAND_MAX);
  return (int)(N + num);
}

fwk_queueID_t gQid = NULL;

void* gMid = NULL;
void* gSid = NULL;
int gTaskGui = 1;
int gCounter = 0;

void* consumer(__attribute__((unused)) void* args)
{
  int rc = fwk_takeSemaphore(gSid, -1);
  if (rc) {
  	printf("takeSemaphore failed: rc %i, errno %i\n", rc, errno);
  }
  printf("%s: gCount=%i\n", __func__, --gCounter);
  sleep(randomN(2));
  return NULL;
}

void* producer(__attribute__((unused)) void* args)
{
//bearer,  fabricant,  begetter, generator
  int rc = fwk_giveSemaphore(gSid);
  if (rc) {
  	printf("giveSemaphore failed: rc %i, errno %i\n", rc, errno);
  }
  printf("%s: gCount=%i\n", __func__, ++gCounter);
  sleep(randomN(3));
  return NULL;
}

void* master(__attribute__((unused)) void* args)
{
  int rc = fwk_lockMutex(gMid, -1);
  if (rc) {
  	printf("lockMutex failed: rc %i, errno %i\n", rc, errno);
  }
  printf("%s: gCount=%i\n", __func__, ++gCounter);
  sleep(randomN(1));
  rc = fwk_unlockMutex(gMid);
  if (rc) {
  	printf("unlockMutex failed: rc %i, errno %i\n", rc, errno);
  }
  sleep(randomN(5));
  return NULL;
}

void* slave(__attribute__((unused)) void* args)
{
  int rc = fwk_lockMutex(gMid, -1);
  if (rc) {
  	printf("lockMutex failed: rc %i, errno %i\n", rc, errno);
  }
  printf("%s: gCount=%i\n", __func__, ++gCounter);
  sleep(randomN(1));
  rc = fwk_unlockMutex(gMid);
  if (rc) {
  	printf("unlockMutex failed: rc %i, errno %i\n", rc, errno);
  }
  sleep(randomN(5));
  return NULL;
}

void* msgA(__attribute__((unused)) void* args)
{
  static int sn = 0, rc = 0;
  int i;
  char msg[FWK_QUEUE_MSG_DEF_LEN*2];
  uint16_t len = 0;
  int timeout = -1;

  for (i = 0; i < randomN(3); ++i) {
    sprintf(msg, "%s:%i", __func__, ++sn);
    len = strlen(msg);
    rc = fwk_sendToQueue(gQid, msg, len, timeout);
    printf("%s sent with %i\n", msg, rc);
    sleep(randomN(2));
  }

  len = sizeof(msg);
  for (i = 0; i < randomN(2); ++i) {
    msg[0] = 0;
    rc = fwk_receiveFromQueue(gQid, msg, len, timeout);
    printf("%s received by %s with len %i\n", msg, __func__, rc);
    sleep(randomN(1));
  }
  return &rc;
}

void* msgB(__attribute__((unused)) void* args)
{
  static int sn = 0, rc = 0;
  int i;
  char msg[FWK_QUEUE_MSG_DEF_LEN*2];
  uint16_t len = 0;
  int timeout = -1;

  for (i = 0; i < randomN(3); ++i) {
    sprintf(msg, "%s:%i", __func__, ++sn);
    len = strlen(msg);
    rc = fwk_sendToQueue(gQid, msg, len, timeout);
    printf("%s sent with %i\n", msg, rc);
    sleep(randomN(2));
  }

  len = sizeof(msg);
  for (i = 0; i < randomN(2); ++i) {
    msg[0] = 0;
    rc = fwk_receiveFromQueue(gQid, msg, len, timeout);
    printf("%s received by %s with len %i\n", msg, __func__, rc);
    sleep(randomN(1));
  }
  return &rc;
}

void* evtA(__attribute__((unused)) void* args)
{
  static int sn = 0, rc = 0;
  int i;
  int timeout = -1;
  fwk_taskID_t dest;

  int buf[FWK_QUEUE_MSG_DEF_LEN];
  fwk_event_t* event = (fwk_event_t*)buf;
  char* msg = (char*)buf + FWK_QUEUE_MSG_DEF_LEN - sizeof(fwk_event_t);

  for (i = 0; i < randomN(2); ++i) {
    sprintf(msg, "%s:%i", __func__, ++sn);
    fwk_eventType_t eventType = i%2;
    dest = fwk_taskId("evtB");
    if (dest) {
      rc = fwk_task_sendEvent(dest, eventType, msg, strlen(msg), timeout);
      printf("%s, %i sent with %i\n", msg, eventType, rc);
    }
    sleep(randomN(3));
  }

  for (i = 0; i < randomN(3); ++i) {
    rc = fwk_task_receiveEvent(event, timeout);
    printf("event[%s, %i, 0x%x, %i, %p] received by %s with rc %i, timeout %i\n",
      (char*)event->data, event->eventType, event->datalen, event->sourceSys, event->sourceTask, __func__, rc, timeout);
    //if (!rc || event.data) fwk_memmgmt_free(event.data);
    //event.data = NULL;
    sleep(randomN(1));
  }
  return &rc;
}

void* evtB(__attribute__((unused)) void* args)
{
  static int sn = 0, rc = 0;
  int i;
  int timeout = 0;//-1;
  fwk_taskID_t dest;
#if 1
  int buf[FWK_QUEUE_MSG_DEF_LEN];
  fwk_event_t* event = (fwk_event_t*)buf;
  char* msg = (char*)buf + FWK_QUEUE_MSG_DEF_LEN - sizeof(fwk_event_t);
#else
  fwk_event_t event;
  char msg[FWK_QUEUE_MSG_DEF_LEN - sizeof(fwk_event_t)];
#endif
  for (i = 0; i < randomN(3); ++i) {
    sprintf(msg, "%s:%i", __func__, ++sn);
    fwk_eventType_t eventType = i%5;
    dest = fwk_taskId("evtA");
    if (dest) {
      rc = fwk_task_sendEvent(dest, eventType, msg, strlen(msg), timeout);
      printf("%s, %i sent with %i\n", msg, eventType, rc);
    }
    sleep(randomN(4));
  }

  for (i = 0; i < randomN(4); ++i) {
    rc = fwk_task_receiveEvent(event, timeout);
    printf("event[%s, %i, 0x%x, %i, %p] received by %s with rc %i, timeout %i\n",
      (char*)event->data, event->eventType, event->datalen, event->sourceSys, event->sourceTask, __func__, rc, timeout);
    //if (!rc || event.data) fwk_memmgmt_free(event.data);
    //event.data = NULL;
    sleep(randomN(2));
  }
  return &rc;
}

void* evt1(__attribute__((unused)) void* args)
{
  static int sn = 0, rc = 0;
  int i;
  int timeout = 1000;
  char msg[FWK_QUEUE_MSG_DEF_LEN];
  void* eid1 = fwk_findEvent("evt1");
  void* eid2 = fwk_findEvent("evt2");

  for (i = 0; i < randomN(3); ++i) {
    sprintf(msg, "%s:%i", __func__, ++sn);
    if (eid2) {
      rc = fwk_sendEvent(eid2, msg, strlen(msg), timeout);
      printf("%s, sent with %i\n", msg, rc);
    }
    sleep(randomN(4));
  }

  for (i = 0; i < randomN(4); ++i) {
    if (eid1) {
      rc = fwk_waitEvent(eid1, msg, sizeof(msg), timeout);
      printf("[%s] received by %s with rc %i, timeout %i\n", msg, __func__, rc, timeout);
    }
    sleep(randomN(2));
  }
  return &rc;
}

void* evt2(__attribute__((unused)) void* args)
{
  static int sn = 0, rc = 0;
  int i;
  int timeout = -1;
  char msg[FWK_QUEUE_MSG_DEF_LEN];
  void* eid1 = fwk_findEvent("evt1");
  void* eid2 = fwk_findEvent("evt2");

  for (i = 0; i < randomN(3); ++i) {
    sprintf(msg, "%s:%i", __func__, ++sn);
    if (eid1) {
      rc = fwk_sendEvent(eid1, msg, strlen(msg), timeout);
      printf("%s, sent with %i\n", msg, rc);
    }
    sleep(randomN(4));
  }

  for (i = 0; i < randomN(4); ++i) {
    if (eid2) {
      rc = fwk_waitEvent(eid2, msg, sizeof(msg), timeout);
      printf("[%s] received by %s with rc %i, timeout %i\n", msg, __func__, rc, timeout);
    }
    sleep(randomN(2));
  }
  return &rc;
}

void* arbitor(__attribute__((unused)) void* args)
{
  return NULL;
}

void* background(__attribute__((unused)) void* args)
{
  return NULL;
}

void* initMutex(__attribute__((unused)) void* args)
{
  int rc = 0;
  rc = fwk_createMutex(NULL, &gMid);
  if (rc) {
  	printf("create mutex failed: rc %i, errno %i\n", rc, errno);
  }
  return NULL;
}

void* initSema(__attribute__((unused)) void* args)
{
  int rc = 0;
  fwk_sema4Attr_t sema4Attr = {"procSema4", -1, 0};
  rc = fwk_createSemaphore(&sema4Attr, &gSid);
  //rc = fwk_createSemaphore(NULL, &gSid);
  if (rc) {
  	printf("create semaphore failed: rc %i, errno %i\n", rc, errno);
  }
  return NULL;
}

void* initQueue(__attribute__((unused)) void* args)
{
  uint16_t depth = FWK_QUEUE_MSG_DEF_NUM * 100;
  uint16_t size = FWK_QUEUE_MSG_DEF_LEN;
  static int rc = 0;
  rc = fwk_createFixsizeQueue(&gQid, NULL, depth, size);
  return &rc;
}

void* preFunc(__attribute__((unused)) void* args)
{
  static int rc = 0;
  initMutex(NULL);
  initSema(NULL);
  rc += *(int*)initQueue(NULL);
  //rc += initCliSvr();
  cliDemo(NULL);
  return &rc;
}

void* postFunc(__attribute__((unused)) void* args)
{
  int rc = 0, i, retry = 2; //200

  rc = fwk_clearQueue(gQid);
  if (rc) {
  	printf("release queue failed: rc %i, errno %i\n", rc, errno);
  }
#if 0
  rc = fwk_task_clearAllEvent();
  if (rc) {
  	printf("release event failed: rc %i, errno %i\n", rc, errno);
  }
  rc = fwk_clearTask();
  if (rc) {
  	printf("clear tasks failed: rc %i, errno %i\n", rc, errno);
  }
#endif
  for (i = 0; i < retry; ++i) {
	  rc = fwk_deleteMutex(gMid);
	  if (rc) {
	  	printf("release mutex failed: rc %i, errno %i, retry %i\n", rc, errno, i);
	  } else {
		  break;
	  }
	  usleep(10*1000);
  }

  for (i = 0; i < retry; ++i) {
	  rc = fwk_deleteSemaphore(gSid);
	  if (rc) {
	  	printf("release semaphore failed: rc %i, errno %i, retry %i\n", rc, errno, i);
	  } else {
		  break;
	  }
	  usleep(10*1000);
  }
  return NULL;
}

void* timer(__attribute__((unused)) void* args)
{
  static int i = 0;
  fwk_taskID_t tid = fwk_myTaskId();
  unsigned long int pid = fwk_rawTid(tid);
  if (gTaskGui) printf("%s[%ld]: %i\n", __func__, pid, ++i);
  sleep(2);
  return NULL;
}

int gTimeATick = 0;
int gTimeBTick = 0;
void a_repeating_timer (__attribute__((unused)) stw_tmr_t *tmr, void *parm) 
{
    int cnt = *(int*)parm + 1;
    *(int*)parm = cnt;
    if (cnt % 10 == 0) {
      printf("TimerA cb: %s timer a count=%i\n", __func__, cnt);
      //stw_timer_stats();
    } 
} 

// TODO: Please original author check if the original implementation works if only one paramter declared (When
// called back, invalid pointer shoudle be de-refernced.
void b_timer (__attribute__((unused)) stw_tmr_t *tmr, void *parm)
{
    printf("TimerB cb: %s timer b count=%i\n", __func__, ++*(int*)parm);
}

void c_timer (__attribute__((unused)) stw_tmr_t *tmr, void *parm)
{
    printf("TimerC cb: %s timer b count=%i\n", __func__, ++*(int*)parm);
    int rc = tmr_delete_timerNode("TimerC");
    printf("%s delete tmr %i\n", __func__, rc);
}

void* timerClient(__attribute__((unused)) void* args)
{
  int buf[FWK_QUEUE_MSG_DEF_LEN];
  fwk_event_t* event = (fwk_event_t*)buf;
  int timeout = FOREVER;
  int len = fwk_task_receiveEvent(event, timeout);
  printf("event[%s, %i, 0x%x, %i, %p] received by %s with len %i, timeout %i\n",
    (char*)event->data, event->eventType, event->datalen, event->sourceSys, event->sourceTask, __func__, len, timeout);
  gTimeBTick += 1;
  if (len < 0) sleep(5);
  return NULL;
}

int initTimerDemo(void)
{
  int rc = 0;
  fwk_taskID_t tid;
  fwk_taskAttr_t tAttr = {"timerM", NULL, tmr_main_task, NULL, SCHED_FIFO, 50, {0, 20*1024, FWK_QUEUE_MSG_DEF_LEN, 8}, 1, 0, -1};
  rc = fwk_createTask(&tAttr, &tid);
  printf("timer_main_task tid: %p\n", tid);

  fwk_taskAttr_t tcAttr = {"tmClient", NULL, timerClient, NULL, SCHED_FIFO, 50, {0, 20*1024, FWK_QUEUE_MSG_DEF_LEN, 8}, 1, 0, -1};
  rc = fwk_createTask(&tcAttr, &tid);
  printf("timer_client tid: %p\n", tid);

  tmr_timer_init();
  //rc = stw_timer_create(STW_NUMBER_BUCKETS,STW_RESOLUTION, "Demo Timer Wheel");

  uint32_t delay = NTERVAL_PER_SECOND/10;//10 times in 1 second
  uint32_t periodic_delay = NTERVAL_PER_SECOND/10;//10 times in 1 second
  //add timer node with name TimerA & TimerB
  rc = tmr_add_timerNode("TimerA",delay,periodic_delay,NULL, a_repeating_timer, &gTimeATick);
  delay = 100 * SYS_TIME_TICKS_IN_A_SEC;// 1 times in 1 second
  periodic_delay = 100 * SYS_TIME_TICKS_IN_A_SEC; // 1 times in 1 second 
  rc = tmr_add_timerNode("TimerB",delay,periodic_delay, tid, b_timer, &gTimeBTick);
  rc = tmr_add_timerNode("TimerC",delay,periodic_delay, NULL, c_timer, &gTimeBTick);

  //rc = tmr_start_timer("TimerA");
  //rc = tmr_start_timer("TimerB");
  rc = tmr_start_timer("TimerC");
  return rc;
}

int cmdMain(int argc, char** argv)
{
  char buf[FWK_QUEUE_MSG_DEF_LEN];
  const char* name = NULL;
  fwk_taskID_t tid = 0;
  int timeout = 0;
  int rc = 0;
  int i;

  if (argc < 1) return -1;
  char* cmd = argv[0];
  const char* cmdList[] = {
    "quit: quit this tool",
    "echo: display input in turn",
    "help: show this info list",
    "taskCreate: create timer thread with assigned parameters",
    "taskDelete: delete thread, arg1 specify taskName",
    "taskPause: pause thread, arg1 specify taskName",
    "taskResume: resume thread, arg1 specify taskName",
    "taskName: show thread name, arg1 specify taskName",
    "taskShow: show thread list",
    "mutexDemo: create mutex tasks: master vs. slave",
    "mutexShow: show mutex list",
    "mutexLock: lock mutex",
    "mutexUnlock: unlock mutex",
    "semaDemo: create semaphore tasks: consumer vs. producer",
    "semaShow: show semaphore list",
    "queueDemo: create queue sender and receiver: msgA, msgB",
    "msgSend: send msg to queue of msgA and msgB",
    "msgRecv: receive msg from queue of msgA and msgB",
    "queueShow: show queue list",
    "eventDemo: create event tasks: evtA, evtB",
    "eventSend: send event to evtA and evtB",
    "evtDemo: create independent event: evt1, evt2",
    "evtSend: send event to evt1 or evt2",
    "evtRecv: receive event from evt1 or evt2",
    "evtShow: show event list",
    "timerDemo: initialize timer wheel and start timer",
    "timerStart: start timer ",
    "timerStop: stop timer",
    "",
  };

  if (!strcmp(cmd, "q") || !strcmp(cmd, "quit") || !strcmp(cmd, "exit")) {
    rc = 886;

  } else if (!strcmp(cmd, "echo")) {
    for (i = 1; i < argc; ++i) {
      printf("%s ", argv[i]);
    }
    printf("\n");

  } else if (!strcmp(cmd, "help")) {
    for (i = 0; (size_t)i < sizeof(cmdList)/sizeof(cmdList[0]); ++i) {
      printf("%s\n", cmdList[i]);
    }

  } else if (!strcmp(cmd, "taskCreate")) {
    uint8_t priority = 1;
    uint8_t policy = SCHED_FIFO;
    fwk_taskRes_t resource;
    resource.stackSize = 1024*20;
    bool_t independent = 1;
    rc = fwk_createPreemptiveTask("timer", &tid, timer, NULL, priority, policy, resource, independent);
    printf("pid:%ld\n", fwk_rawTid(tid));
    printf("tid: %p\n", tid);

  } else if (!strcmp(cmd, "taskDelete")) {
    if (argc > 1) {
      name = argv[1];
    } else {
      name = "timer";
    }
    tid = fwk_taskId(name);
    printf("pid:%ld\n", fwk_rawTid(tid));
    rc = fwk_deleteTask(tid);
    printf("delete %s tid: %p\n", name, tid);

  } else if (!strcmp(cmd, "taskPause")) {
    if (argc > 1) {
      name = argv[1];
    } else {
      name = "timer";
    }
    tid = fwk_taskId(name);
    rc = fwk_suspendTask(tid);
    printf("pid:%ld\n", fwk_rawTid(tid));
    printf("tid: %p\n", tid);

  } else if (!strcmp(cmd, "taskResume")) {
    if (argc > 1) {
      name = argv[1];
    } else {
      name = "timer";
    }
    tid = fwk_taskId(name);
    rc = fwk_resumeTask(tid);
    printf("pid:%ld\n", fwk_rawTid(tid));
    printf("tid: %p\n", tid);

  } else if (!strcmp(cmd, "taskName")) {
    if (argc > 1) {
      name = argv[1];
    } else {
      name = "timer";
    }
    tid = fwk_taskId(name);
    const char* tName = fwk_taskName(tid);
    printf("tid: %p, pid:%ld, taskName:%s\n", tid, fwk_rawTid(tid), tName);

  } else if (!strcmp(cmd, "taskShow")) {
    fwk_showTask(NULL);

  } else if (!strcmp(cmd, "mutexDemo")) {
    fwk_taskAttr_t MAttr = {"master", NULL, master, NULL, SCHED_FIFO, 80, {0, 20*1024, 0, 10}, 1, 0, -1};
    rc = fwk_createTask(&MAttr, &tid);
    printf("master tid: %p\n", tid);

    fwk_taskAttr_t SAttr = {"slave", NULL, slave, NULL, SCHED_FIFO, 20, {0, 20*1024, 0, 10}, 1, 0, -1};
    rc = fwk_createTask(&SAttr, &tid);
    printf("slave tid: %p\n", tid);

  } else if (!strcmp(cmd, "mutexShow")) {
    fwk_showMutex(NULL);
  } else if (!strcmp(cmd, "mutexLock")) {
    if (argc > 1) {
      timeout = strtol(argv[1], NULL, 0);
    }
    rc = fwk_lockMutex(gMid, timeout);
  } else if (!strcmp(cmd, "mutexUnlock")) {
    rc = fwk_unlockMutex(gMid);

  } else if(!strcmp(cmd, "semaDemo")) {
    fwk_taskAttr_t CAttr = {"consumer", NULL, consumer, NULL, SCHED_FIFO, 50, {0, 20*1024, 0, 10}, 1, 0, -1};
    rc = fwk_createTask(&CAttr, &tid);
    printf("consumer tid: %p\n", tid);

    fwk_taskAttr_t PAttr = {"producer", NULL, producer, NULL, SCHED_FIFO, 10, {0, 20*1024, 0, 10}, 1, 0, -1};
    rc = fwk_createTask(&PAttr, &tid);
    printf("producer tid: %p\n", tid);

  } else if (!strcmp(cmd, "semaShow")) {
    fwk_showSemaphore(NULL);

  } else if (!strcmp(cmd, "queueDemo")) {
    fwk_taskAttr_t AAttr = {"msgA", NULL, msgA, NULL, SCHED_FIFO, 80, {0, 20*1024, 0, 10}, 1, 0, -1};
    rc = fwk_createTask(&AAttr, &tid);
    printf("msgA tid: %p\n", tid);

    fwk_taskAttr_t BAttr = {"msgB", NULL, msgB, NULL, SCHED_FIFO, 20, {0, 20*1024, 0, 10}, 1, 0, -1};
    rc = fwk_createTask(&BAttr, &tid);
    printf("msgB tid: %p\n", tid);

  } else if (!strcmp(cmd, "msgSend")) {
    if (argc > 1) {
      strcpy(buf, argv[1]);
    } else {
      strcpy(buf, "Empty");
    }
    timeout = (argc > 2) ? strtol(argv[2], NULL, 0) : -1;
    rc = fwk_sendToQueue(gQid, buf, strlen(buf), timeout);

  } else if (!strcmp(cmd, "msgRecv")) {
    timeout = (argc > 1) ? strtol(argv[1], NULL, 0) : -1;
    rc = fwk_receiveFromQueue(gQid, buf, sizeof(buf), timeout);
    printf("Received len=%i, msg[%s]\n", rc, buf);

  } else if (!strcmp(cmd, "queueShow")) {
    fwk_showQueue(NULL);

  } else if(!strcmp(cmd, "eventDemo")) {
    fwk_taskAttr_t AAttr = {"evtA", NULL, evtA, NULL, SCHED_FIFO, 50, {0, 20*1024, FWK_QUEUE_MSG_DEF_LEN, 8}, 1, 0, -1};
    rc = fwk_createTask(&AAttr, &tid);
    //rc = fwk_task_createEvent(tid);
    printf("evtA tid: %p\n", tid);

    fwk_taskAttr_t BAttr = {"evtB", NULL, evtB, NULL, SCHED_FIFO, 10, {0, 20*1024, FWK_QUEUE_MSG_DEF_LEN, 8}, 1, 0, -1};
    rc = fwk_createTask(&BAttr, &tid);
    //rc = fwk_task_createEvent(tid);
    printf("evtB tid: %p\n", tid);

  } else if(!strcmp(cmd, "eventSend")) {
    if (argc > 1) {
      strcpy(buf, argv[1]);
    } else {
      strcpy(buf, "evtA");
    }
    fwk_taskID_t dest = fwk_taskId(buf);
    fwk_eventType_t eventType = (fwk_eventType_t)(__LINE__);
    rc = fwk_task_sendEvent(dest, eventType, "Hello", 8, -1);

  } else if(!strcmp(cmd, "evtDemo")) {
    fwk_taskAttr_t AAttr = {"evt1", NULL, evt1, NULL, SCHED_FIFO, 50, {0, 0, 0, 0}, 1, 0, -1};
    rc = fwk_createTask(&AAttr, &tid);
    printf("evt1 tid: %p, rc %i\n", tid, rc);
    rc = fwk_createEvent(20, 8, NULL, "evt1");

    fwk_taskAttr_t BAttr = {"evt2", NULL, evt2, NULL, SCHED_FIFO, 10, {0, 0, 0, 0}, 1, 0, -1};
    rc = fwk_createTask(&BAttr, &tid);
    printf("evt2 tid: %p, rc %i\n", tid, rc);
    rc = fwk_createEvent(20, 8, NULL, "evt2");

  } else if(!strcmp(cmd, "evtSend")) {
    if (argc > 1) {
      strcpy(buf, argv[1]);
    } else {
      strcpy(buf, "evt1");
    }
    void* eid = fwk_findEvent(buf);
    rc = fwk_sendEvent(eid, "Hello", 8, -1);

  } else if(!strcmp(cmd, "evtRecv")) {
    if (argc > 1) {
      strcpy(buf, argv[1]);
    } else {
      strcpy(buf, "evt1");
    }
    void* eid = fwk_findEvent(buf);
    rc = fwk_waitEvent(eid, buf, sizeof(buf), -1);

  } else if (!strcmp(cmd, "evtShow")) {
    fwk_showEvent(NULL);

  } else if(!strcmp(cmd, "timerDemo")) {
    rc = initTimerDemo();
  } else if(!strcmp(cmd, "timerStart")) {
    if (argc > 1) {
      rc = tmr_start_timer(argv[1]);
    } else {
      printf("Please append timer name.\n");
    }
  } else if(!strcmp(cmd, "timerStop")) {
    if (argc > 1) {
      rc = tmr_stop_timer(argv[1]);
    } else {
      printf("Please append timer name.\n");
    }
  } else if(!strcmp(cmd, "timerShow")) {
    if (argc > 1) {
      rc = tmr_show_timerNode(argv[1]);
    } else {
      printf("Please append timer name.\n");
    }
  } else if(!strcmp(cmd, "timerDelete")) {
    if (argc > 1) {
      rc = tmr_delete_timerNode(argv[1]);
      //rc = stw_timer_destroy();
    } else {
      printf("Please append timer name.\n");
    }

  } else {
    printf("Unsupported command %s\n", cmd);
    rc = -2;
  }
  return rc;
}

int testCase(void)
{
  int rc = 0;
  fwk_taskID_t tid;
  uint8_t priority = 0;
  fwk_taskRes_t resource;
  memset(&resource, 0, sizeof(resource));
  rc = fwk_createNormalTask(NULL, &tid, timer, NULL, priority, resource, 1);
  CHECK(rc);
  return rc;
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char** argv)
{
#define ARGS_CNT        8             /* Max argv entries */
#define ARGS_BUFFER  256         /* bytes total for arguments */
  int rc = 0;
  char cmdLine[ARGS_BUFFER];
  char ch = 0;
  char* cmdArr[ARGS_CNT];

  //testCase();
  preFunc(NULL);
  for (;;) {
    cmdLine[0] = 0;
    printf("cmd>");
    scanf("%[^\n]", cmdLine);
    scanf("%c", &ch);
    if (!strlen(cmdLine)) continue;

    int i = 0;
    int k = 0;   // key index
    int n = 0;   // args number
    int len = 0; // args length

    while (cmdLine[i]) {
      if (cmdLine[i] == '\t' || cmdLine[i] == ' ') {
        if (len) {
          cmdArr[n] = &cmdLine[k];
          cmdLine[i] = 0;  //Terminate the argment
          len = 0;
          ++n;
        }
      } else {
        if (len) {
          ++len;
        } else {
          k = i;
          len = 1;
        }
      }
      ++i;
    }
    if (len) {
      cmdArr[n] = &cmdLine[k];
      ++n;
    }
    rc = cmdMain(n, cmdArr);
    if (rc == 886) break;
    printf("Execute: [");
    for (i = 0; i < n; ++i) {
      printf("%s ", cmdArr[i]);
    }
    printf("], Finished with rc %i\n", rc);
  }
  postFunc(NULL);
  return rc;
}
