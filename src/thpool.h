#ifndef _THPOOL_
#define _THPOOL_

#include <pthread.h>
#include <semaphore.h>

typedef struct thpool_job {
	void*  (*function)(void* arg);
	void*                     arg;
	struct thpool_job*     next;
	struct thpool_job*     prev;
} thpool_job;

typedef struct thpool_jobqueue{
	thpool_job *head;
	thpool_job *tail;
	int           jobsN;
	sem_t        *queueSem;
}thpool_jobqueue;

typedef struct thpool{
	pthread_t*       threads;
	int              threadsN;
	thpool_jobqueue* jobqueue;
}thpool;

typedef struct thread_data{                            
	pthread_mutex_t *mutex_p;
	thpool        *tp_p;
}thread_data;

/**
 * @brief  Initialize threadpool
 * 
 * Allocates memory for the threadpool, jobqueue, semaphore and fixes 
 * pointers in jobqueue.
 * 
 * @param  number of threads to be used
 * @return threadpool struct on success,
 *         NULL on error
 */
thpool* thpool_init(int threadsN);


/**
 * @brief What each thread is doing
 * 
 * In principle this is an endless loop. The only time this loop gets interuppted is once
 * thpool_destroy() is invoked.
 * 
 * @param threadpool to use
 * @return nothing
 */
void thpool_thread_do(thpool* tp_p);


/**
 * @brief Add work to the job queue
 * 
 * Takes an action and its argument and adds it to the threadpool's job queue.
 * If you want to add to work a function with more than one arguments then
 * a way to implement this is by passing a pointer to a structure.
 * 
 * ATTENTION: You have to cast both the function and argument to not get warnings.
 * 
 * @param  threadpool to where the work will be added to
 * @param  function to add as work
 * @param  argument to the above function
 * @return int
 */
int thpool_add_work(thpool* tp_p, void *(*function_p)(void*), void* arg_p);


/**
 * @brief Destroy the threadpool
 * 
 * This will 'kill' the threadpool and free up memory. If threads are active when this
 * is called, they will finish what they are doing and then they will get destroyied.
 * 
 * @param threadpool a pointer to the threadpool structure you want to destroy
 */
void thpool_destroy(thpool* tp_p);

/**
 * @brief Initialize queue
 * @param  pointer to threadpool
 * @return 0 on success,
 *        -1 on memory allocation error
 */
int thpool_jobqueue_init(thpool* tp_p);

/**
 * @brief Add job to queue
 * 
 * A new job will be added to the queue. The new job MUST be allocated
 * before passed to this function or else other functions like thpool_jobqueue_empty()
 * will be broken.
 * 
 * @param pointer to threadpool
 * @param pointer to the new job(MUST BE ALLOCATED)
 * @return nothing 
 */
void thpool_jobqueue_add(thpool* tp_p, thpool_job* newjob_p);

/**
 * @brief Remove last job from queue. 
 * 
 * This does not free allocated memory so be sure to have peeked() \n
 * before invoking this as else there will result lost memory pointers.
 * 
 * @param  pointer to threadpool
 * @return 0 on success,
 *         -1 if queue is empty
 */
int thpool_jobqueue_removelast(thpool* tp_p);


/** 
 * @brief Get last job in queue (tail)
 * 
 * Gets the last job that is inside the queue. This will work even if the queue
 * is empty.
 * 
 * @param  pointer to threadpool structure
 * @return job a pointer to the last job in queue,
 *         a pointer to NULL if the queue is empty
 */
thpool_job* thpool_jobqueue_peek(thpool* tp_p);

/**
 * @brief Remove and deallocate all jobs in queue
 * 
 * This function will deallocate all jobs in the queue and set the
 * jobqueue to its initialization values, thus tail and head pointing
 * to NULL and amount of jobs equal to 0.
 * 
 * @param pointer to threadpool structure
 * */
void thpool_jobqueue_empty(thpool* tp_p);

#endif
