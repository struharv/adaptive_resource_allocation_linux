#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <linux/kernel.h>
#include <sys/syscall.h>

#include "params.h"

#define NSEC_PER_SEC 1000000000ULL
#define JOBS 200
#define LOOPS 2*30000000
#define LOOPS1 2*50000000
#define PERIOD_IN_NANOS 900000000
#define CHANGE_EVERY_TASK 70

#define handle_error_en(en, msg) \
	do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define __NR_finish 434
#define __NR_start 435
#define __NR_init 436
#define REQUIRED_RT 300000000
#define DISABLE_SYSCALL 0

struct periodic_task {
	unsigned long current_job_id;
	struct timespec period;
	struct timespec first_activation;
	struct timespec current_activation;
	int terminated;
};


void timespec_add(struct timespec * ts, struct timespec * period)
{
        ts->tv_nsec += period->tv_nsec;
        while (ts->tv_nsec >= NSEC_PER_SEC) {
                ts->tv_nsec -= NSEC_PER_SEC;
                ts->tv_sec++;
        }
}

long finish_syscall(void)
{
	if (DISABLE_SYSCALL) {
		printf("disable syscall\n");
		return;
	}
    return syscall(__NR_finish);
}
long start_syscall(void)
{
	if (DISABLE_SYSCALL) {
		printf("disable syscall\n");
		return;
	}
    return syscall(__NR_start);
}

long init_syscall(long value)
{
	if (DISABLE_SYSCALL) {
		printf("disable syscall\n");
		return;
	}
    return syscall(__NR_init, value);
}

void sleep_until_next_activation(struct periodic_task *tsk) {
	int err;
	do {
		
		err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &tsk->current_activation, NULL);
		if (err != 0) {
			printf("waking up error!\n");
		} 
	} while (err != 0 && errno == EINTR);
	assert(err == 0);
}


unsigned long timenow() {
	struct timeval timecheck;
	gettimeofday(&timecheck, NULL);
	return timecheck.tv_sec * 1000000 + (long)timecheck.tv_usec;
}

unsigned long do_work(long loops) {
	unsigned long start = timenow();
	for(int i = 0; i < loops; i++) {}
	return timenow() - start;
}

void print_monotime() {
	struct timespec monotime;
   	clock_gettime(CLOCK_MONOTONIC, &monotime);		
	printf("%ld %ld\n", monotime.tv_sec, monotime.tv_nsec);
}

void process_one_activation(struct periodic_task *tsk, long loops) {
	
	start_syscall();
	printf(":dowork:");		
	print_monotime();

	unsigned long duration = do_work(loops);
	
	printf("Job #%lu\n", tsk->current_job_id);
	printf("Elapsed time:%ld\n", duration);

	finish_syscall();
}



void process_activations(void) {
	int err;
	struct periodic_task tsk;
	struct timespec now_time;

	tsk.current_job_id = 1;
	tsk.terminated = 0;

	tsk.period.tv_sec = 0;
	tsk.period.tv_nsec = PERIOD_IN_NANOS;

	err = clock_gettime(CLOCK_MONOTONIC, &tsk.first_activation);
	assert(err == 0);
	tsk.current_activation = tsk.first_activation;

	init_syscall(REQUIRED_RT);
	int cnt = 0;
	long loops = LOOPS;
	while (!tsk.terminated) {
		printf("------\n");		
		printf(":before_sleep:");		
		print_monotime();
		
		sleep_until_next_activation(&tsk);
		printf(":after_sleep:");		
		print_monotime();
		
		if (tsk.current_job_id % CHANGE_EVERY_TASK == 0) {
			if (loops == LOOPS) {
				loops = LOOPS1;
			} else {
				loops = LOOPS;
			}
		}
		process_one_activation(&tsk, loops);
		tsk.current_job_id++;
		
		printf(":after_job:");		
		print_monotime();

		clock_gettime(CLOCK_MONOTONIC, &now_time);		
		unsigned long long now = now_time.tv_sec * NSEC_PER_SEC  + now_time.tv_nsec;	
		do {
			timespec_add(&tsk.current_activation, &tsk.period);
			unsigned long long added = tsk.current_activation.tv_sec * NSEC_PER_SEC + tsk.current_activation.tv_nsec ;
			printf("adding %llu %lluT\n", added, now);
			if (added > now) {
				break;			
			}
			//printf("adding more!\n");

		} while (1);	

		printf(":sleep_activation:%ld %ld\n", tsk.current_activation.tv_sec, tsk.current_activation.tv_nsec);
		if (cnt++ >= JOBS)
			return;
	}
}

static void display_sched_attr(int policy, struct sched_param *param) {
	printf("policy=%s, priority=%d\n",
		(policy == SCHED_FIFO)  ? "SCHED_FIFO" :
		(policy == SCHED_RR)    ? "SCHED_RR" :
		(policy == SCHED_OTHER) ? "SCHED_OTHER" :
		"???",

	param->sched_priority);
}


static void display_thread_sched_attr(char *msg) {
	int policy, s;
	struct sched_param param;

	s = pthread_getschedparam(pthread_self(), &policy, &param);
	if (s != 0)
		handle_error_en(s, "pthread_getschedparam");
	
	printf("%s\n", msg);
	display_sched_attr(policy, &param);
}	

int main(int argc, char** argv) {
	struct sched_param param;
	param.sched_priority = 10;
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
	display_thread_sched_attr("");

	process_activations();
}

