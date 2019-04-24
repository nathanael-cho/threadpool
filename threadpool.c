#include "threadpool.h"

////////////////////////////
// GLOBALS + DECLARATIONS //
////////////////////////////

// Assumes a single threadpool at any given time
static volatile int wrap_up_work;

// Assumes caller has mutex lock
Job* get_next_job(JobQueue* queue);

////////////////////
// INITIALIZATION //
////////////////////

void* thread_work(void* void_pool) {
    pthread_detach(pthread_self());

    ThreadPool* pool = (ThreadPool*) void_pool;

    JobQueue* queue = pool->job_queue;
    pthread_mutex_lock(&(pool->lock));
    while (!wrap_up_work || queue->active_jobs) {
        Job* job = NULL;

        while (!(job = get_next_job(queue))) {
            if (wrap_up_work) {
                pool->thread_list->length--;
                pthread_cond_signal(&(pool->is_finished_signal));
                pthread_mutex_unlock(&(pool->lock));
                return NULL;
            }
            pthread_cond_wait(&(pool->signal), &(pool->lock));
        }

        pthread_mutex_unlock(&(pool->lock));

        job->call(job->arguments);

        free(job);

        pthread_mutex_lock(&(pool->lock));
        queue->active_jobs--;
        if (!queue->active_jobs) {
            pthread_cond_signal(&(pool->is_finished_signal));
        }
    }

    pool->thread_list->length--;
    pthread_cond_signal(&(pool->is_finished_signal));
    pthread_mutex_unlock(&(pool->lock));

    return NULL;
}

int create_thread(pthread_t* thread, ThreadPool* pool) {
    return pthread_create(thread, NULL, thread_work, pool);
}

ThreadPool* create_threadpool(void) {
    wrap_up_work = 0;

    ThreadPool* pool = calloc(1, sizeof(ThreadPool));
    if (pthread_mutex_init(&(pool->lock), NULL)) {
        free(pool);
        return NULL;
    }
    if (pthread_cond_init(&(pool->signal), NULL)) {
        pthread_mutex_destroy(&(pool->lock));
        free(pool);
        return NULL;
    }
    if (pthread_cond_init(&(pool->is_finished_signal), NULL)) {
        pthread_cond_destroy(&(pool->signal));
        pthread_mutex_destroy(&(pool->lock));
        free(pool);
        return NULL;
    }

    JobQueue* queue = pool->job_queue = calloc(1, sizeof(JobQueue));
    queue->front = NULL;
    queue->back = NULL;
    queue->active_jobs = 0;

    ThreadList* thread_list = pool->thread_list = calloc(1, sizeof(ThreadList));
    thread_list->threads = calloc(THREAD_COUNT, sizeof(pthread_t));
    thread_list->length = 0;
    for (int i = 0; i < THREAD_COUNT; i++) {
        if (create_thread(thread_list->threads + i, pool)) {
            destroy_threadpool(pool);
            return NULL;
        }
        thread_list->length++;
    }

    return pool;
}

//////////
// JOBS //
//////////

Job* get_next_job(JobQueue* queue) {
    if (!queue->front) {
        return NULL;
    }

    Job* retrieved_job = queue->front;
    queue->front = retrieved_job->next;
    if (queue->back == retrieved_job) {
        queue->back = retrieved_job->next;
    }
    return retrieved_job;
}

void put_job(ThreadPool* pool, Job* job) {
    if (!job || wrap_up_work) {
        return;
    }

    pthread_mutex_lock(&(pool->lock));

    JobQueue* queue = pool->job_queue;

    if (queue->active_jobs > JOB_MAXIMUM) {
        pthread_cond_broadcast(&(pool->signal));
        pthread_cond_wait(&(pool->is_finished_signal), &(pool->lock));
    }

    if (!queue->front) {
        queue->front = job;
    } else {
        queue->back->next = job;
    }
    queue->back = job;
    queue->active_jobs++;

    pthread_cond_signal(&(pool->signal));
    pthread_mutex_unlock(&(pool->lock));
}

/////////////////
// DESTRUCTION //
/////////////////

void destroy_thread_list(ThreadList* thread_list) {
    if (!thread_list) {
        return;
    }

    free(thread_list->threads);
    free(thread_list);
}

void destroy_job_queue(JobQueue* queue) {
    if (!queue) {
        return;
    }

    free(queue);
}

void destroy_threadpool(ThreadPool* pool) {
    if (!pool) {
        return;
    }

    pthread_mutex_lock(&(pool->lock));
    wrap_up_work = 1;
    pthread_mutex_unlock(&(pool->lock));
    pthread_cond_broadcast(&(pool->signal));

    pthread_mutex_lock(&(pool->lock));
    while (pool->job_queue->active_jobs || pool->thread_list->length) {
        pthread_cond_wait(&(pool->is_finished_signal), &(pool->lock));
    }
    pthread_mutex_unlock(&(pool->lock));

    destroy_thread_list(pool->thread_list);
    destroy_job_queue(pool->job_queue);
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->signal));
    pthread_cond_destroy(&(pool->is_finished_signal));
    free(pool);
}

///////////////////
// WAIT FOR JOBS //
///////////////////

void wait_for_jobs(ThreadPool* pool) {
    pthread_mutex_lock(&(pool->lock));
    while((pool->job_queue)->active_jobs) {
        pthread_cond_wait(&(pool->is_finished_signal), &(pool->lock));
    }
    pthread_mutex_unlock(&(pool->lock));
}
