/*
 * Experiment to measure cache line effects on throughput.
 * Author: Joel Fernandes <agnel.joel@gmail.com>
 */

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <sched.h>
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>
#include <inttypes.h>

#define NUM_BUFF 20
#define CACHE_LINE_SIZE 64
#define MAX_BUFF 20
#define ALRM 2

char __attribute__((aligned(4096))) buff[NUM_BUFF][CACHE_LINE_SIZE];
int __attribute__((aligned(4096))) stop;

#define GPLOT 0

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error_en(en, msg) \
	do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

void stop_experiment(int signal)
{
	if(signal == SIGALRM)
		stop = 1;
}

void measure_count(int count) {
	int i;

	/* Timestamp */
	uint32_t total_time;
	uint32_t total_count = 0;
	uint32_t *counter;
	struct timeval start, end;

	memset(buff, 0, NUM_BUFF * CACHE_LINE_SIZE);

	gettimeofday(&start, NULL);

	while(!stop) {
		for(i = 0; i < count; i++) {
			counter = (uint32_t *)buff[i];
			(*counter)++;
		}		
	}

	stop = 0;

	gettimeofday(&end, NULL);

	total_time = (end.tv_sec - start.tv_sec);

	for(i = 0; i < count; i++) {
		counter = (uint32_t *)buff[i];
		total_count += *counter;
	}
	
	if (GPLOT)
		printf("%d " PRIu32 "\n", count, total_count/total_time);
	else
		printf("Number of buffers: %3d, " \
			"Total time: %" PRIu32 \
			" ops: %" PRIu32 \
			" speed (ops/sec) = \t %" PRIu32 "\n",
			count,
			total_time,
			total_count,
			total_count/total_time);
}

int main()
{
	int count = 1;

	struct itimerval iv = 
	{
		.it_value = { .tv_sec = ALRM },
		.it_interval = { .tv_sec = ALRM }
	};

	struct sigaction sigalrm = {
		.sa_handler = stop_experiment,
		.sa_flags = SA_RESTART,
	};
	sigaction(SIGALRM, &sigalrm, NULL);

	while(count <= MAX_BUFF) {
		setitimer(ITIMER_REAL, &iv, NULL);
		measure_count(count++);
	}
}
