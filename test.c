#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h> 
#include <time.h> 

#define TIMER_INTERVAL 1
#define TIMER_COUNT 5

pthread_key_t timer_key; // 线程特定数据键

void timer_handler(int sig) {
    int *count = pthread_getspecific(timer_key); // 获取定时器信息
    printf("Timer %ld count: %d\n", pthread_self(), ++(*count));
}

void *thread_func(void *arg) { 
    int count = 0;
    struct sigevent sev; 
    timer_t timer;

    // 创建定时器
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGALRM;
    sev.sigev_value.sival_ptr = &timer;
    if (timer_create(CLOCK_REALTIME, &sev, &timer) == -1) {
        perror("timer_create");
        exit(EXIT_FAILURE);
    }

    // 设置定时器
    struct itimerspec its;
    its.it_interval.tv_sec = TIMER_INTERVAL;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = TIMER_INTERVAL;
    its.it_value.tv_nsec = 0;
    if (timer_settime(timer, 0, &its, NULL) == -1) {
        perror("timer_settime");
        exit(EXIT_FAILURE);
    }

    // 分配线程特定数据
    pthread_setspecific(timer_key, &count);

    // 等待定时器结束
    while (count < TIMER_COUNT) {
        sleep(1);
    }

    // 销毁定时器
    timer_delete(timer);

    pthread_exit(NULL);
}

int main() {
    pthread_t thread1, thread2;
    
    // 创建线程特定数据键
    if (pthread_key_create(&timer_key, NULL) != 0) {
        perror("pthread_key_create");
        exit(EXIT_FAILURE);
    }

    // 创建线程1
    if (pthread_create(&thread1, NULL, thread_func, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // 创建线程2
    if (pthread_create(&thread2, NULL, thread_func, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    // 等待线程结束
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    // 销毁线程特定数据键
    pthread_key_delete(timer_key);

    return 0;
}
