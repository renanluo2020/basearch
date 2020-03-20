// Unit test for race condition in stw_timer_tick

#include <fwk/task/task.h>
#include <fwk/task/queue.h>
#include <fwk/timer/stw_mgmt.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#define CNT_TIMER STW_MAX_TIMER_NUM
#define SIZE_PRINT_BUF  4096
int gCounterA[CNT_TIMER];
char gTmrNameA[CNT_TIMER][STW_NAME_LENGTH];

void timerA_cb(__attribute__((unused)) stw_tmr_t *tmr, void *parm) {
    ++*(int *)parm;
}

int ut_timer_rack_condition(void) {
    int i, j, k;
    int rc;
    uint32_t periodic_delay = 1000;
    char buf[SIZE_PRINT_BUF];
    int len;

    tmr_timer_init();
    fwk_taskID_t tid;
    fwk_taskAttr_t tAttr = {"timerM", NULL, tmr_main_task, NULL, SCHED_OTHER, 0, {0, 20*1024, FWK_QUEUE_MSG_DEF_LEN, 8}, 1, 0, -1};
    rc = fwk_createTask(&tAttr, &tid);
    printf("timer_main_task tid: %p\n", tid);
    for(i = 0; i < CNT_TIMER; i++) {
        snprintf(gTmrNameA[i], STW_NAME_LENGTH, "TmrA%03d", i);
        rc = tmr_add_timerNode(gTmrNameA[i], periodic_delay, periodic_delay, NULL, timerA_cb, &gCounterA[i]);
        if (!rc) {
            printf("tmr_add_timerNode failed, i=%02d\n", i);
        }
    }
    for(k = 0; k < 1000; k++) {
        memset(gCounterA, 0, sizeof gCounterA);
        for(i = 0; i < CNT_TIMER; i++) {
            rc = tmr_start_timer(gTmrNameA[i]);
            if (!rc) {
                printf("tmr_start_timer failed, i=%02d\n", i);
            }
        }
        usleep(1000*100*(k%10+1));
        for(i = CNT_TIMER-1; i >= 0; i--) {
            rc = tmr_stop_timer(gTmrNameA[i]);
            if (!rc) {
                printf("tmr_stop_timer failed, i=%02d\n", i);
            }
        }
        len = snprintf(buf, SIZE_PRINT_BUF, "k=%03d, gCounterA", k);
        for(j = 0, i = 1; i < CNT_TIMER; i++) {
            if (gCounterA[i] != gCounterA[j]) {
                if (j == i-1) {
                    len += snprintf(buf+len, SIZE_PRINT_BUF-len, "[%d]=%d, ", i-1, gCounterA[i-1]);
                } else {
                    len += snprintf(buf+len, SIZE_PRINT_BUF-len, "[%d..%d]=%d, ", j, i-1, gCounterA[i-1]);
                }
                j = i;
            }
        }
        if (j == i-1) {
            len += snprintf(buf+len, SIZE_PRINT_BUF-len, "[%d]=%d\n", i-1, gCounterA[i-1]);
        } else {
            len += snprintf(buf+len, SIZE_PRINT_BUF-len, "[%d..%d]=%d\n", j, i-1, gCounterA[i-1]);
        }
        printf("%s", buf);
    }
    return 0;
}

int main(void) {
    /* With above test procedure, with stw_timer_tick() having race condition issue,
    the test always failed due to segment fault in several rounds of test.
    The longest survival k was 338. With stw_timer_tick fixed, no crash anymore
    after five rounds of test
    */
    ut_timer_rack_condition();
    return 0;
}
