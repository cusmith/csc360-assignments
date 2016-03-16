/*  goldrush.h
 *  The Gold Rush Example
 *  CSC 360 Tutorial, 2015 Fall
 */ 
 
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<pthread.h>
#include<unistd.h>
#include<sys/time.h>

//structure of slave info.
typedef struct slave{
    int ID;
    int submit_time;
}slave;

void* dig_thread(struct slave*);
void request_treasury(struct slave*);
void release_treasury();