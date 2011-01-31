#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <numa.h>
#include "spm_mt.h"
#include "mt_lib.h"
#include "spmv_loops_mt_numa.h"
#include "tsc.h"
#include "prfcnt.h"
#include "vector.h"

static VECTOR_TYPE *y = NULL;
static pthread_barrier_t barrier;
static unsigned long loops_nr = 0;
static float secs = 0.0;

static void *do_spmv_thread_main(void *arg) {
    spm_mt_thread_t *spm_mt_thread = (spm_mt_thread_t *) arg;
	SPMV_NAME(_fn_t) *spmv_mt_fn = spm_mt_thread->spmv_fn;
	setaffinity_oncpu(spm_mt_thread->cpu);

	int i;
    tsc_t tsc;
    tsc_init(&tsc);
    tsc_start(&tsc);
	for (i = 0; i < loops_nr; i++) {
		pthread_barrier_wait(&barrier);
		spmv_mt_fn(spm_mt_thread->spm, spm_mt_thread->data, y);
		pthread_barrier_wait(&barrier);
	}
    tsc_pause(&tsc);
    secs = tsc_getsecs(&tsc);
    tsc_shut(&tsc);
    return (void *) 0;
}

static void *do_spmv_thread(void *arg)
{
    spm_mt_thread_t *spm_mt_thread = (spm_mt_thread_t *) arg;
	SPMV_NAME(_fn_t) *spmv_mt_fn = spm_mt_thread->spmv_fn;
	setaffinity_oncpu(spm_mt_thread->cpu);

	int i;
	for (i = 0; i < loops_nr; i++) {
		pthread_barrier_wait(&barrier);
		spmv_mt_fn(spm_mt_thread->spm, spm_mt_thread->data, y);
		pthread_barrier_wait(&barrier);
	}

    return (void *) 0;
}

float SPMV_NAME(_bench_mt_loop_numa)(spm_mt_t *spm_mt,
                                     unsigned long loops,
                                     unsigned long rows_nr,
                                     unsigned long cols_nr,
                                     SPMV_NAME(_fn_t) *fn)
{
    int err, i;
    pthread_t *tids;
    VECTOR_TYPE *x;

	loops_nr = loops;
	err = pthread_barrier_init(&barrier, NULL, spm_mt->nr_threads);
	if (err){
		perror("pthread_barrier_init");
		exit(1);
	}

	tids = malloc(sizeof(pthread_t)*spm_mt->nr_threads);
	if ( !tids ){
		perror("malloc");
		exit(1);
	}

    size_t *parts = malloc(sizeof(*parts)*spm_mt->nr_threads);
    int *nodes = malloc(sizeof(*nodes)*spm_mt->nr_threads);
    if (!parts || !nodes) {
		perror("malloc");
		exit(1);
    }

    /* Mask for indicating on which nodes we allocated x */
    unsigned long   alloc_nodemask = 0;
    for (i = 0; i < spm_mt->nr_threads; i++) {
        spm_mt_thread_t *spm = &(spm_mt->spm_threads[i]);
        if (!(alloc_nodemask & (1 << spm->node))) {
            /* x is not allocated on this node */
            printf("alloc x on node: %d\n", spm->node);
            x = VECTOR_NAME(_create_onnode)(cols_nr, spm->node);
            VECTOR_NAME(_init_rand_range)(x, (ELEM_TYPE) -1000,
                                          (ELEM_TYPE) 1000);
            alloc_nodemask |= 1 << spm->node;
        }

        /* part_info is the number of rows assigned to each thread */
        parts[i] = (uint64_t) spm->part_info;
        nodes[i] = spm->node;
        spm->data = x;
        if (fn)
            spm->spmv_fn = fn;
    }

    
    /* Allocate an interleaved y */
    y = VECTOR_NAME(_create_interleaved)(rows_nr, parts, spm_mt->nr_threads,
                                         nodes);
    VECTOR_NAME(_init)(y, 0);

    for (i = 1; i < spm_mt->nr_threads; i++)
        pthread_create(tids + i, NULL, do_spmv_thread, spm_mt->spm_threads + i);

    do_spmv_thread_main(spm_mt->spm_threads);

	for (i = 1; i < spm_mt->nr_threads; i++){
		pthread_join(tids[i], NULL);
	}

    /* Destroy vectors */
    for (i = 0; i < spm_mt->nr_threads; i++) {
        spm_mt_thread_t *spm = &(spm_mt->spm_threads[i]);
        if (alloc_nodemask & (1 << spm->node)) {
            VECTOR_NAME(_destroy)(spm->data);
            alloc_nodemask &= ~(1 << spm->node);
        }
    }

    VECTOR_NAME(_destroy)(y);

    free(parts);
    free(nodes);
    free(tids);
    pthread_barrier_destroy(&barrier);
    return secs;
}

void SPMV_NAME(_check_mt_loop_numa)(void *spm_serial,
                                    spm_mt_t *spm_mt,
                                    SPMV_NAME(_fn_t) *fn,
                                    unsigned long loops,
                                    unsigned long rows_nr,
                                    unsigned long cols_nr,
                                    SPMV_NAME(_fn_t) *mt_fn)
{
    int err, i;
    pthread_t *tids;
    VECTOR_TYPE *x, *y2;

	loops_nr = loops;
	err = pthread_barrier_init(&barrier, NULL, spm_mt->nr_threads + 1);
	if (err){
		perror("pthread_barrier_init");
		exit(1);
	}

	tids = malloc(sizeof(pthread_t)*spm_mt->nr_threads);
	if ( !tids ){
		perror("malloc");
		exit(1);
	}

    size_t *parts = malloc(sizeof(*parts)*spm_mt->nr_threads);
    int *nodes = malloc(sizeof(*nodes)*spm_mt->nr_threads);
    if (!parts || !nodes) {
		perror("malloc");
		exit(1);
    }

    /* Mask for indicating on which nodes we allocated x */
    unsigned long   alloc_nodemask = 0;
    for (i = 0; i < spm_mt->nr_threads; i++) {
        spm_mt_thread_t *spm = &(spm_mt->spm_threads[i]);
        if (!(alloc_nodemask & (1 << spm->node))) {
            /* x is not allocated on this node */
            x = VECTOR_NAME(_create_onnode)(cols_nr, spm->node);
            VECTOR_NAME(_init_rand_range)(x, (ELEM_TYPE) -1000,
                                          (ELEM_TYPE) 1000);
            alloc_nodemask |= 1 << spm->node;
        }

        /* part_info is the number of rows assigned to each thread */
        parts[i] = (uint64_t) spm->part_info;
        nodes[i] = spm->node;
        spm->data = x;
        if (mt_fn)
            spm->spmv_fn = mt_fn;
    }

    
    /* Allocate an interleaved y */
    y = VECTOR_NAME(_create_interleaved)(rows_nr, parts, spm_mt->nr_threads,
                                         nodes);
    y2 = VECTOR_NAME(_create)(rows_nr);
    VECTOR_NAME(_init)(y, 0);
    VECTOR_NAME(_init)(y2, 0);

    for (i = 0; i < spm_mt->nr_threads; i++)
        pthread_create(tids + i, NULL, do_spmv_thread, spm_mt->spm_threads + i);

    for (i = 0; i < loops_nr; i++) {
        pthread_barrier_wait(&barrier);
        pthread_barrier_wait(&barrier);
        /* x points to the last x vector created */
        fn(spm_serial, x, y2);
		if (VECTOR_NAME(_compare)(y2, y) < 0) {
			exit(1);
		}
    }

    /* Destroy vectors */
    for (i = 0; i < spm_mt->nr_threads; i++) {
        spm_mt_thread_t *spm = &(spm_mt->spm_threads[i]);
        if (alloc_nodemask & (1 << spm->node)) {
            VECTOR_NAME(_destroy)(spm->data);
            alloc_nodemask &= ~(1 << spm->node);
        }
    }

    VECTOR_NAME(_destroy)(y);
    VECTOR_NAME(_destroy)(y2);

	for (i = 0; i < spm_mt->nr_threads; i++){
		pthread_join(tids[i], NULL);
	}

    pthread_barrier_destroy(&barrier);
    free(parts);
    free(nodes);
    free(tids);
}
