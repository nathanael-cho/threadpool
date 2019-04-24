#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "threadpool.h"

// Some trickery to test the code even with optimization on
void* thread_maximum_improvement(void) {
    sleep(1);

    return NULL;
}

void* thread_minimum_improvement(void) {
    int a = 0;
    for (int i = 0; i < 10000000; i++) {
        if (i % 2) {
            a += i;
        } else {
            a-= i;
        }
    }

    // We know it will be > 0
    if (a < 0) {
        printf("%d\n", a);
    }

    return NULL;
}

void thread_implementation(ThreadPool* pool, void* (*call) ()) {
    for (int i = 0; i < 16; i++) {
        Job* job = malloc(sizeof(Job));
        job->next = NULL;
        job->call = (void*) call;
        job->arguments = NULL;
        put_job(pool, job);
    }

    wait_for_jobs(pool);
}

void naive_implementation(void* (*call) ()) {
    for (int i = 0; i < 16; i++) {
        call();
    }
}

int main(int argc, char** argv) {
    int execute_maximum_flag = 1;
    int execute_minimum_flag = 1;

    if (argc > 1) {
        if (strcmp(argv[1], "-max") == 0) {
            execute_minimum_flag = 0;
        }
        if (strcmp(argv[1], "-min") == 0) {
            execute_maximum_flag = 0;
        }
    }

    ThreadPool* pool = create_threadpool();

    struct timeval stop, start;

    if (execute_maximum_flag) {
        gettimeofday(&start, NULL);
        naive_implementation(thread_maximum_improvement);
        gettimeofday(&stop, NULL);
        double microseconds = (double) (stop.tv_usec - start.tv_usec);
        double whole_seconds = (double) (stop.tv_sec - start.tv_sec);
        double seconds = microseconds / 1000000 + whole_seconds;
        printf("The naive implementation of the light function took %f seconds.\n", seconds);

        gettimeofday(&start, NULL);
        thread_implementation(pool, thread_maximum_improvement);
        gettimeofday(&stop, NULL);
        microseconds = (double) (stop.tv_usec - start.tv_usec);
        whole_seconds = (double) (stop.tv_sec - start.tv_sec);
        seconds = microseconds / 1000000 + whole_seconds;
        printf("The thread implementation of the light function took %f seconds.\n", seconds);
    }

    if (execute_minimum_flag) {
        gettimeofday(&start, NULL);
        naive_implementation(thread_minimum_improvement);
        gettimeofday(&stop, NULL);
        double microseconds = (double) (stop.tv_usec - start.tv_usec);
        double whole_seconds = (double) (stop.tv_sec - start.tv_sec);
        double seconds = microseconds / 1000000 + whole_seconds;
        printf("The naive implementation of the intensive function took %f seconds.\n", seconds);

        gettimeofday(&start, NULL);
        thread_implementation(pool, thread_minimum_improvement);
        gettimeofday(&stop, NULL);
        microseconds = (double) (stop.tv_usec - start.tv_usec);
        whole_seconds = (double) (stop.tv_sec - start.tv_sec);
        seconds = microseconds / 1000000 + whole_seconds;
        printf("The thread implementation of the intensive function took %f seconds.\n", seconds);
    }

    destroy_threadpool(pool);
}
