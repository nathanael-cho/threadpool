#ifndef CS165_THREAD_POOL
#define CS165_THREAD_POOL

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define THREAD_COUNT 4
#define JOB_MAXIMUM 8

typedef struct Job {
    struct Job* next;
    void (*call)(void*);
    void* arguments;
} Job;

typedef struct JobQueue {
    Job* front;
    Job* back;
    int active_jobs;
} JobQueue;

typedef struct ThreadList {
    pthread_t* threads;
    int length;
} ThreadList;

typedef struct ThreadPool {
    ThreadList* thread_list;
    JobQueue* job_queue;
    pthread_mutex_t lock;
    pthread_cond_t signal;
    pthread_cond_t is_finished_signal;
} ThreadPool;

ThreadPool* create_threadpool(void);
void put_job(ThreadPool* pool, Job* job);
void destroy_threadpool(ThreadPool* pool);
void wait_for_jobs(ThreadPool* pool);

#endif
