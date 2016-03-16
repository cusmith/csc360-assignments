#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>

//structure of slave info.
typedef struct customer{
	int customer_no;
    double arrival_time;
    double service_time;
    int priority;
    int line_no;
}customer;

void* customer_thread(struct customer* customer);
void request_service(struct customer* customer);
void swap_to_front(struct customer* customer);
void wait_for_arrival_batch(struct customer* customer);
void sort_waiting_customers();
void sort_all_waiting_customers();
void sort_inactive_waiting_customers();
void release_service();
int relative_time_int();
int compare_customer_priority();
float relative_time();
