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

#define NUMCPUS 2
// Has to be a minimum of 8 due to size of uint64_t
#define MAXSEP 256

#define GPLOT 1

#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

#define handle_error_en(en, msg) \
	do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

struct thread_info {    /* Used as argument to thread_start() */
	pthread_t thread_id;        /* ID returned by pthread_create() */
	int       thread_num;       /* Application-defined thread # */
	char     *argv_string;      /* From command-line argument */
	int       sep;                /* Separation between per-thread count */
};

uint8_t __attribute__((aligned(4096))) counters[NUMCPUS*MAXSEP];
int __attribute__((aligned(4096))) stop;

void *threadloop(void *arg)
{
	struct thread_info *tinfo = (struct thread_info *) arg;
	uint64_t *counter;

	counter = (uint64_t *)(&(counters[(tinfo->thread_num - 1) * tinfo->sep]));
	counter[0] = 0;

	while(1) {
		if(stop)
			break;
		counter[0]++;
	}
}

void stop_experiment(int signal)
{
	if(signal == SIGALRM)
		stop = 1;
}

void measure_count(int sep) {
	int s;
	/* Timestamp */
	unsigned long int total_time;
	uint64_t total_count = 0;
	uint64_t *counter;
	struct timeval start, end;

	gettimeofday(&start, NULL);

	/* Thread creation */
	int num_threads = NUMCPUS;
	struct thread_info *tinfo;
	pthread_attr_t attr;

	s = pthread_attr_init(&attr);
	if (s != 0)
		handle_error_en(s, "pthread_attr_init");

	tinfo = calloc(num_threads, sizeof(struct thread_info));
	if (tinfo == NULL)
		handle_error("calloc");

	for (int tnum = 0; tnum < num_threads; tnum++) {
		tinfo[tnum].thread_num = tnum + 1;
		tinfo[tnum].sep = sep;

		s = pthread_create(&tinfo[tnum].thread_id, &attr,
				&threadloop, &tinfo[tnum]);
		if (s != 0)
			handle_error_en(s, "pthread_create");
	}

	while(!stop) sleep(2);

        for (int tnum = 0; tnum < num_threads; tnum++) {
		pthread_join(tinfo[tnum].thread_id, NULL);
	}

	stop = 0;

	gettimeofday(&end, NULL);
	total_time = (end.tv_sec - start.tv_sec);

	for(int i = 0; i < NUMCPUS; i++) {
		counter = (uint64_t *)(&(counters[i*sep]));
		total_count += *counter;
	}
	
	if (GPLOT)
		printf("%d %lu\n", sep, total_count/total_time);
	else
		printf("Separation: %3d, Total time: %3lu, ops: %lu, speed (ops/sec) = \t %lu\n",
		       sep, total_time, total_count, total_count/total_time);
}

int main()
{
	int sep = sizeof(uint64_t);
	struct itimerval iv = 
	{
		.it_value = { .tv_sec = 5 },
		.it_interval = { .tv_sec = 5 }
	};

	struct sigaction sigalrm = {
		.sa_handler = stop_experiment,
		.sa_flags = SA_RESTART,
	};
	sigaction(SIGALRM, &sigalrm, NULL);

	while(sep <= MAXSEP) {
		setitimer(ITIMER_REAL, &iv, NULL);
		measure_count(sep);
		sep = sep << 1;
	}
}
