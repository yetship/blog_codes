//
// Created by yetship on 2017/7/14.
//
#include <cstring>
#include <ctime>
#include <iostream>
#include <array>
#include <forward_list>
using namespace std;

#include <sys/time.h>
#include <sys/signal.h>

#define WHEEL_SIZE 8
#define TIMER_NUM 5
#define SECONDS 1000000000

typedef int timer_id;
typedef int timer_expiry(timer_id id, void *user_data, int len);

int alarmCnt = 0;
int currTimerId = 0;
timer_id id[TIMER_NUM];

struct timer {
    timer* next;
    timer_id id;			/**< timer id		*/

    int interval;			/**< timer interval(second)*/
    int rotation; 			/**< 0 -> interval 	*/

    timer_expiry *cb;		/**< call if expiry 	*/
    void *user_data;		/**< callback arg	*/
    int len;			/**< user_data length	*/

    timer(int interval, timer_expiry *timerCb, void* userData, int dataLen):
            interval(interval), rotation(0), cb(timerCb), id(currTimerId++), next(NULL) {
        if(userData != NULL || dataLen != 0) {
            this->user_data = malloc(dataLen);
            memcpy(this->user_data, userData, dataLen);
            this->len = dataLen;
        } else {
            this->user_data = NULL;
        }
    }
};

int currSlot = 0;
array<timer*, WHEEL_SIZE> timeWheel;


time_t addTimer(int interval, timer_expiry *cb, void *user_data, int len) {
    int targetSlot = (interval + WHEEL_SIZE - currSlot) % WHEEL_SIZE;

    timer *t = new timer(interval, cb, user_data, len);
    t->rotation = (interval + currSlot) / WHEEL_SIZE;

    timer* next = timeWheel[targetSlot];
    if (next) {
        while(next->next) {
            next = next->next;
        }
        next->next = t;
    } else {
        timeWheel[targetSlot] = t;
    }

    return t->id;
}

static char *fmtTime(char *tstr)
{
    time_t t;

    t = time(NULL);
    strcpy(tstr, ctime(&t));
    tstr[strlen(tstr)-1] = '\0';

    return tstr;
}

int timerCb(timer_id id, void *arg, int len)
{
    char tstr[200];
    alarmCnt ++;
    printf("hello [%s]/id %d: timer '%s' cb is here.\n", fmtTime(tstr), id, (char*)arg);
    return 0;
}


static void sigFunc(int signo)
{
    timer *t = timeWheel[currSlot], *prev = NULL;

    while (t) {
        if (t->rotation > 0) {
            t->rotation --;
            prev = t;
        } else {
            t->cb(t->id, t->user_data, t->len);

            if (prev == NULL) {
                timeWheel[currSlot] = t->next;
                addTimer(t->interval, t->cb, t->user_data, t->len);
                delete t;
            } else {
                prev->next = t->next;
            }
        }
        t = t->next;
    }
    currSlot = (currSlot + 1) % WHEEL_SIZE;
}


int main() {
    void (*old_sigfunc)(int);
    if ((old_sigfunc = signal(SIGALRM, sigFunc)) == SIG_ERR) {
        printf("signal register error");
        return -1;
    }

    /* Setting our interval timer for driver our mutil-timer and store old timer value */
    struct itimerval value, ovalue;
    value.it_value.tv_sec = 1;
    value.it_value.tv_usec = 0;
    value.it_interval.tv_sec = 1;
    value.it_interval.tv_usec = 0;
    int ret = setitimer(ITIMER_REAL, &value, &ovalue);
    if (ret != 0) {
        perror("set timer error");
        return -1;
    }

    for (int i = 0; i < WHEEL_SIZE; i++) {
        timeWheel[i] = NULL;
    }

    printf("register all\n");
    id[0] = addTimer(2, timerCb, (void *) "a", sizeof("a"));
    id[1] = addTimer(3, timerCb, (void *) "b", sizeof("b"));
    id[2] = addTimer(5, timerCb, (void *) "c", sizeof("c"));
    id[3] = addTimer(7, timerCb, (void *) "d", sizeof("d"));
    id[4] = addTimer(9, timerCb, (void *) "e", sizeof("e"));

    // set timer
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec= 0.1 * SECONDS;
    printf("timer waiting\n");
    while (alarmCnt < 45) {
        nanosleep(&time, NULL);
    }
}
