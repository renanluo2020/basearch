/*------------------------------------------------------------------
 * stw_mgmt.c -- Single Timer Wheel Timer management
 *------------------------------------------------------------------
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fwk/timer/stw_mgmt.h>
#include <fwk/timer/stw_timer.h>
#ifdef NANO_SLEEP_TIMER
#include <time.h>
#include <sys/times.h>
#else
#include <signal.h>
#include <sys/time.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

static  stw_t      *stw_sys_handle = 0;

/*Array list stored specific timer information*/
static stw_tmr_Node_t tmr_list[STW_MAX_TIMER_NUM];

/*Mutex for opeartions on  tmr_list*/
static pthread_mutex_t tmr_mutex;

/*
 * Name
 *     tmr_unlock_mutex
 *
 *  Description:
 *     Unlock the timer mutex.
 *
 *  Parameters:
 *
 *  Returns:
 *     retMutex
 *
 */
int tmr_unlock_mutex(void)
{
    int retMutex;

    retMutex =  pthread_mutex_unlock(&tmr_mutex);
    if (retMutex) printf("pthread_mutex_unlock ERROR\n");
    return retMutex;
}

/*
 * Name
 *     tmr_lock_mutex
 *
 *  Description:
 *     Lock the timer mutex.
 *
 *  Parameters:
 *
 *  Returns:
 *     retMutex
 *
 */
int tmr_lock_mutex(void)
{
    int retMutex;

    retMutex = pthread_mutex_lock(&tmr_mutex);
    if (retMutex) printf("pthread_mutex_lock ERROR\n");
    return retMutex;
}

/*
 * Name
 *     tmr_init_mutex
 *
 *  Description:
 *     Initial the timer mutex.
 *
 *  Parameters:
 *
 *  Returns:
 *     Nothing
 *
 */
void tmr_init_mutex(void)
{
    int error;
    pthread_mutexattr_t mutex_attr;

    pthread_mutexattr_init(&mutex_attr);
    error = pthread_mutex_init(&tmr_mutex,&mutex_attr);
    if (error) printf("pthread_mutex_init ERROR\n");
    pthread_mutexattr_destroy(&mutex_attr);
}

/*
 * Name
 *     tmr_destroy_mutex
 *
 *  Description:
 *     destroy the timer mutex.
 *
 *  Parameters:
 *
 *  Returns:
 *     Nothing
 *
 */
void tmr_destroy_mutex(void)
{
    int error;

    error = pthread_mutex_destroy(&tmr_mutex);
    if (error) printf("tmr_destroy_mutex ERROR\n");
}

/*
 * Name
 *     tmr_timer_list_init
 *
 *  Description:
 *     Initial the timer list -- tmr_list.
 *
 *  Parameters:
 *
 *  Returns:
 *     Nothing
 *
 */
void tmr_timer_list_init(void){
    //if(tmr_list){
        /* MUTEX */
        tmr_lock_mutex();
        memset(tmr_list,0,sizeof(stw_tmr_Node_t) * STW_MAX_TIMER_NUM);
        /* MUTEX */
        tmr_unlock_mutex();
    //}
}

/*
 * Name
 *     tmr_timer_init
 *
 *  Description:
 *     Initial the whole timer list and timer tick. -- tmr_list, timer tick
 *
 *  Parameters:
 *
 *  Returns:
 *     Nothing
 *
 */
void tmr_timer_init(void){
    /*
     * create and configure nodal KPA timer
     */
    stw_timer_create(STW_NUMBER_BUCKETS, STW_RESOLUTION,  "Demo Timer Wheel", &stw_sys_handle);

    tmr_init_mutex();

    /* MUTEX */
    tmr_lock_mutex();
    memset(tmr_list,0,sizeof(stw_tmr_Node_t) * STW_MAX_TIMER_NUM);
    /* MUTEX */
    tmr_unlock_mutex();

    stw_init_mutex();
}

/*
 * Name
 *     tmr_find_free_tmrNode
 *
 *  Description:
 *     Find free time node from time list and the node's name must not used before.
 *
 *  Parameters:
 *     Initial the specific timer.
 *
 *     *tmrList             pointer to the specific timer element
 *
 *  Returns:
 *    TRUE
 *    FALSE
 *    if tmrNode is not null, it points to the first free time node
 */
bool_t tmr_find_free_tmrNode(const char *timer_name,stw_tmr_Node_t **tmrNode){
    int currTmr;

    *tmrNode = NULL;
    if(!timer_name) return FALSE;

    for(currTmr = 0; currTmr < STW_MAX_TIMER_NUM; currTmr++){
        if(strcmp(timer_name,tmr_list[currTmr].timer_name) == 0){
            return FALSE;
        }
    }

    for(currTmr = 0; currTmr < STW_MAX_TIMER_NUM; currTmr++){
        if(tmr_list[currTmr].used == FALSE){
            *tmrNode = &(tmr_list[currTmr]);
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * Name
 *     tmr_find_tmrNode
 *
 *  Description:
 *     Find the specific timer according time name.
 *
 *  Parameters:
 *     *timer_name      pointer to the name of specific timer
 *
 *     *tmrNode             pointer to the specific timer element
 *
 *  Returns:
 *    TRUE
 *    FALSE
 *    if tmrNode is not null, it points to the first free time node
 */
bool_t tmr_find_tmrNode(const char *timer_name,stw_tmr_Node_t **tmrNode){
    int currTmr;

    *tmrNode = NULL;
    if(!timer_name) return FALSE;

    if(strlen(timer_name) > STW_NAME_LENGTH - 1){
        return FALSE;
    }

    for(currTmr = 0; currTmr < STW_MAX_TIMER_NUM; currTmr++){
        if(tmr_list[currTmr].used == TRUE &&
           strcmp(timer_name,tmr_list[currTmr].timer_name) == 0){
            *tmrNode = &(tmr_list[currTmr]);
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * Name
 *     tmr_add_timerNode
 *
 *  Description:
 *     Add the specific timer to free time node in the time list.
 *
 *  Parameters:
 *
 *     *timer_name      pointer to the name of specific timer
 *
 *     delay            initial delay in milliseconds
 *
 *     periodic_delay   periodic delay in milliseconds
 *
 *     tid   pointer of ThreadInfo to be
 *                      invoked when the timer expires.  This pointer
 *                      must run-to-completion by function send_message.
 *
 *     user_cb          application function call-back to be
 *                      invoked when the timer expires.  This call-back
 *                      must run-to-completion and not block.
 *
 *     parm             persistent parameter passed to the application
 *                      call-back upon expiration
 *
 *  Returns:
 *    TRUE
 *    FALSE
 */
bool_t tmr_add_timerNode(const char *timer_name, uint32_t delay, uint32_t periodic_delay,
                                void* tid, stw_call_back_t user_cb, void *parm){
    //char timer_name[STW_NAME_LENGTH] = {0,};
    stw_tmr_Node_t *tmrNode = NULL;

    if(!timer_name) return FALSE;

    if(strlen(timer_name) > STW_NAME_LENGTH -1)
        return FALSE;

    if(!tid && !user_cb) return FALSE;

    if(tid && !parm) return FALSE;

    /* MUTEX */
    tmr_lock_mutex();
    if(tmr_find_free_tmrNode(timer_name,&tmrNode) == FALSE){
        /* MUTEX */
        tmr_unlock_mutex();
        return FALSE;
    }

    if(!tmrNode){
        /* MUTEX */
        tmr_unlock_mutex();
        return FALSE;
    }

    tmrNode->used = TRUE;
    strncpy(tmrNode->timer_name, timer_name, STW_NAME_LENGTH-1);
    tmrNode->timer_name[STW_NAME_LENGTH-1] = '\0';

    tmrNode->tmr.delay = delay;
    tmrNode->tmr.periodic_delay = periodic_delay;

    tmrNode->tid = tid;
    tmrNode->tmr.func_ptr = user_cb;
    tmrNode->parm = parm;

    stw_timer_prepare(&(tmrNode->tmr));
    /* MUTEX */
    tmr_unlock_mutex();

    return TRUE;
}

/*
 * Name
 *     tmr_delete_timerNode
 *
 *  Description:
 *     Delete the specific timer node in the time list.
 *
 *  Parameters:
 *     *timer_name      pointer to the name of specific timer
 *
 *  Returns:
 *    TRUE
 *    FALSE
 */
bool_t tmr_delete_timerNode(const char *timer_name){
    //char timer_name[STW_NAME_LENGTH] = {0,};
    stw_tmr_Node_t *tmrNode = NULL;

    if(!timer_name)
        return FALSE;

    if(strlen(timer_name) > STW_NAME_LENGTH -1)
        return FALSE;

    /* MUTEX */
    tmr_lock_mutex();
    if(tmr_find_tmrNode(timer_name, &tmrNode) == FALSE){
        /* MUTEX */
        tmr_unlock_mutex();
        return FALSE;
    }

    if(!tmrNode){
        /* MUTEX */
        tmr_unlock_mutex();
        return FALSE;
    }

    memset(tmrNode,0,sizeof(stw_tmr_Node_t));
    /* MUTEX */
    tmr_unlock_mutex();

    return TRUE;
}

/*
 * Name
 *     tmr_show_timerNode
 *
 *  Description:
 *     show the specific timer node in the time list.
 *
 *  Parameters:
 *     *timer_name      pointer to the name of specific timer
 *
 *  Returns:
 *    TRUE
 *    FALSE
 */
bool_t tmr_show_timerNode(const char *timer_name){
    //char timer_name[STW_NAME_LENGTH] = {0,};
    stw_tmr_Node_t *tmrNode = NULL;

    if(!timer_name)
        return FALSE;

    if(strlen(timer_name) > STW_NAME_LENGTH -1)
        return FALSE;

    /* MUTEX */
    tmr_lock_mutex();
    if(tmr_find_tmrNode(timer_name, &tmrNode) == FALSE){
        /* MUTEX */
        tmr_unlock_mutex();
        return FALSE;
    }

    if(!tmrNode){
        /* MUTEX */
        tmr_unlock_mutex();
        return FALSE;
    }

    printf("\n used %d\n", tmrNode->used );
    if(tmrNode->used == FALSE) {
        /* MUTEX */
        tmr_unlock_mutex();
        return TRUE;
    }

    printf("       timer_name=%s\n", tmrNode->timer_name);

    if(tmrNode->tid){
        //fwk_showTask(tmrNode->tid);
    }

    if(tmrNode->parm)
        printf("       parm=%d\n", 1);

    printf("    tmr rotation_count=%u\n", tmrNode->tmr.rotation_count);
    printf("    tmr delay=%u\n", tmrNode->tmr.delay);
    printf("    tmr periodic_delay=%u\n", tmrNode->tmr.periodic_delay);

    /* MUTEX */
    tmr_unlock_mutex();

    return TRUE;
}

/*
 * Name
 *     tmr_start_timer
 *
 *  Description:
 *     Start the specific timer node in the time list.
 *
 *  Parameters:
 *     *timer_name      pointer to the name of specific timer
 *
 *  Returns:
 *    TRUE
 *    FALSE
 */
bool_t tmr_start_timer(const char *timer_name){
    stw_tmr_Node_t *tmrNode = NULL;

    /*
    printf("%s timer=%s \n",
           __func__,
           timer_name);
    */
    if(!timer_name)
        return FALSE;

    if(strlen(timer_name) > STW_NAME_LENGTH -1)
        return FALSE;

    /* MUTEX */
    tmr_lock_mutex();
    if(tmr_find_tmrNode(timer_name, &tmrNode) == FALSE){
        /* MUTEX */
        tmr_unlock_mutex();
        return FALSE;
    }

    if(!tmrNode){
        /* MUTEX */
        tmr_unlock_mutex();
        return FALSE;
    }

    // Added on behalf of weipengc on 2020-02-03
    //if timer has been running, it need to be stop first , then start
    if(stw_timer_running(&tmrNode->tmr) == TRUE){
        if(stw_timer_stop(stw_sys_handle, &tmrNode->tmr) != RC_STW_OK){
            /* MUTEX */
            tmr_unlock_mutex();
            return FALSE;
        }
    }

    stw_timer_prepare(&(tmrNode->tmr));
    if(stw_timer_start(stw_sys_handle,
                             &(tmrNode->tmr),
                             tmrNode->tmr.delay,
                             tmrNode->tmr.periodic_delay,
                             tmrNode->tid,
                             tmrNode->tmr.func_ptr,
                             tmrNode->parm) != RC_STW_OK){
        /* MUTEX */
        tmr_unlock_mutex();
        return FALSE;
    }
    /* MUTEX */
    tmr_unlock_mutex();
    return TRUE;
}

/*
 * Name
 *     tmr_delete_timerNode
 *
 *  Description:
 *     Start the specific timer node in the time list.
 *
 *  Parameters:
 *     *timer_name      pointer to the name of specific timer
 *
 *  Returns:
 *    TRUE
 *    FALSE
 */
bool_t tmr_stop_timer(const char *timer_name){
    stw_tmr_Node_t *tmrNode = NULL;

    if(!timer_name)
        return FALSE;

    if(strlen(timer_name) > STW_NAME_LENGTH -1)
        return FALSE;

    /* MUTEX */
    tmr_lock_mutex();
    if(tmr_find_tmrNode(timer_name, &tmrNode) == FALSE){
        /* MUTEX */
        tmr_unlock_mutex();
        return FALSE;
    }

    if(!tmrNode){
        /* MUTEX */
        tmr_unlock_mutex();
        return FALSE;
    }

    if(stw_timer_stop(stw_sys_handle, &tmrNode->tmr) != RC_STW_OK){
        /* MUTEX */
        tmr_unlock_mutex();
        return FALSE;
    }
    /* MUTEX */
    tmr_unlock_mutex();
    return TRUE;
}

/*
 * This function is envoked as result of the sigalrm
 * to drive the timer wheel.
 */
static void timer_handler (__attribute__((unused)) int signum)
{
    stw_timer_tick(stw_sys_handle);
}

/*
 * Name
 *     tmr_main_task
 *
 *  Description:
 *     Start the specific timer node in the time list.
 *
 *  Parameters:
 *     *timer_name      pointer to the name of specific timer
 *
 *  Returns:
 *    TRUE
 *    FALSE
 */
void * tmr_main_task(__attribute__((unused)) void * ptr){
    printf("tmr_main_task!!!\n\n");
#ifndef NANO_SLEEP_TIMER
    /*
       * interval timer variables
       */
    struct sigaction sa;
    struct itimerval timer;
    uint32_t microseconds;

    /*
     * Install the interval timer_handler as the signal handler
     * for SIGALRM.
     */
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &timer_handler;
    sigaction (SIGALRM, &sa, NULL);

    /*
     * Configure the initial delay for 500 msec
     * and then every 100 msec after
     */
    microseconds = (STW_RESOLUTION * 1000);
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = microseconds;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = microseconds;

    /*
     * Now start the interval timer.
     */
    setitimer (ITIMER_REAL, &timer, NULL);

    //timer main loop
    while (1) {
        pause(); /*keep in loop! do nothing*/
    }

#else // for nano sleep timer
    uint32_t               u4TickInterval;
    uint32_t               u4Ticks;
//    uint32_t               u4Ticks100;
    uint32_t               u4Ticksx10 = 0;
    uint32_t               u4TicksRem = 0;

    struct timespec     req;
    struct tms          st_tms, en_tms;
    //timer_t             st_tms, en_tms;
    clock_t             st_time, en_time;

    /* the interval between 2 ticks in micro-seconds */
    /* this is also the timer granularity in micro-seconds */
    /* all calculations in this routine will be based on micro-seconds */
    uint32_t               gu4Stups = SYS_TIME_TICKS_IN_A_SEC;
    u4TickInterval = 1000000 / (gu4Stups);

    req.tv_sec = u4TickInterval / 1000000;
    req.tv_nsec = (u4TickInterval % 1000000) * 1000;

    for (;;)
    {
        st_time = times (&st_tms);
        nanosleep (&req, NULL);
        en_time = times (&en_tms);

        /* If the retval of times() rolls over, assume one tick elapsed */
        if (en_time < st_time)
        {
            u4Ticks = 1;
        }
        else
        {
            /* Modified for getting correct conversion to 100 ticks */
            u4Ticksx10 = (((en_time - st_time) * gu4Stups * 10)/ sysconf (_SC_CLK_TCK));
            u4Ticks = u4Ticksx10 / 10;
            u4TicksRem +=  u4Ticksx10 % 10;
            if((u4TicksRem / 10) >= 1)
            {
                u4Ticks++;
                u4TicksRem -= 10;
            }
        }

        while (u4Ticks != 0)
        {
            timer_handler(0);
            u4Ticks--;

        }
    }
#endif

    return NULL;
}
