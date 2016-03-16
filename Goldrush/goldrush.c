/*  goldrush.c
 *  The Gold Rush Example
 *  CSC 360 Tutorial, 2015 Fall
 */     
#include "goldrush.h"

#define MAX_SLAVES 100
#define MAX_DIGTIME 10
#define MAX_DIGNUM 50

/********global variables (critical section)*****************************************/
//two mutex
pthread_mutex_t mutex_modify_critical_section = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_make_request = PTHREAD_MUTEX_INITIALIZER;
//one convar
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

int slave_in_treasury = 0;//the id of the slave currently in the treasury
int treasury_avail = 1;//if the treasury available
int num_waiting = 0;//number of slaves that are waiting outside treasury
int total_coin = 0;//the total number of gold coins you have got
struct slave* waiting_slaves[MAX_SLAVES];//waiting slaves array


/********dig_thread*************************************************************
*       This function is called with each thread creation
*       INPUT: a pointer to a slave struct
*       OUTPUT: detailed information regarding when the slave digs, submits and terminates
********************************************************************************/
void* dig_thread(struct slave* slave){
    //print info. when the slave begins to work
    pthread_mutex_lock(&mutex_modify_critical_section);
    printf("Slave no.%2d starts working...\n", slave->ID);
    pthread_mutex_unlock(&mutex_modify_critical_section);
    
    //dig until find some gold coins
    int dig_time, coin_num;
    dig_time = rand() % MAX_DIGTIME + 1;
    sleep(dig_time);
    coin_num = rand() % MAX_DIGNUM + 1;
    
    //print info. after the slave finds some coins
    pthread_mutex_lock(&mutex_modify_critical_section);
    printf("Hurrah! Slave no.%2d found %2d gold coins!\n", slave->ID, coin_num);
    pthread_mutex_unlock(&mutex_modify_critical_section);
    
    //attempt to enter the treasury
    request_treasury(slave);
    
    //request is successful! enter the treasury
    pthread_mutex_lock(&mutex_modify_critical_section);
    slave_in_treasury = slave->ID;//mark the slave in treasury
    printf("Slave no.%2d enters the treasury and will stay for %2d seconds.\n", slave->ID, slave->submit_time);
    pthread_mutex_unlock(&mutex_modify_critical_section);
    
    //submits coins and print submission info.
    sleep(slave->submit_time); //consume some time to submit coins
    
    //add coins to the total number
    pthread_mutex_lock(&mutex_modify_critical_section);
    total_coin += coin_num;
    printf("Slave no.%2d finished submission. Your total coins number reaches %2d!\n", slave->ID, total_coin);
    pthread_mutex_unlock(&mutex_modify_critical_section);

    //leave the treasury
    release_treasury();
    return 0;
}

/****************************************Begin Monitor********************************************/

/********request_treasury*****************************************
 *      This function will allow only one slave at a time
 *      to enter the treasury
 *      INPUT: a pointer to a slave struct
 *      OUTPUT: one slave will be able to enter the treasury
 *      while others will wait outside
 *****************************************************************/ 
void request_treasury(struct slave* slave){
    //lock mutex_make_request
    pthread_mutex_lock(&mutex_make_request);

    //request to enter the treasure, on success, return;    
    if(treasury_avail == 1 && num_waiting == 0){
        //set current slave
        slave_in_treasury = slave->ID;
        //set the treasury unavailable
        treasury_avail--;
        pthread_mutex_unlock(&mutex_make_request);
        return;
    }
    
    //otherwise, add the slave to the waiting list
    pthread_mutex_lock(&mutex_modify_critical_section);
    waiting_slaves[num_waiting] = slave;
    num_waiting++;
    pthread_mutex_unlock(&mutex_modify_critical_section);
    
    //wait, if (1) the slave is not at the head of the waiting list or (2) the treasury is unavailable, wait
    while((slave->ID != waiting_slaves[0]->ID) || (treasury_avail != 1)){
        //print wait info.
        printf("Slave no.%2d is waiting for the leaving of slave no.%2d.\n", slave->ID, slave_in_treasury);
        //wait on the conditional variable, release mutex_make_request
        pthread_cond_wait(&cond, &mutex_make_request);
    }
    
    //get out of the waiting list, keep the order (FIFS) of left slaves 
    pthread_mutex_lock(&mutex_modify_critical_section);
    int i;
    for (i = 0; i < num_waiting; i++)
        waiting_slaves[i] = waiting_slaves[i+1];
    num_waiting--;
    treasury_avail--;//the treasury becomes unavailable
    pthread_mutex_unlock(&mutex_modify_critical_section);
    
    //release mutex_make_request
    pthread_mutex_unlock(&mutex_make_request);
}

/********release_treasury*****************************************
 *      This function will make the treasury available
 *      again and wake up the waiting slaves
 *      INPUT: nothing
 *      OUTPUT: treasury_avail is increased and waiting threads
 *      woken up
 *****************************************************************/ 
void release_treasury(){
    //treasury is available again
    pthread_mutex_lock(&mutex_modify_critical_section);
    treasury_avail++;
    pthread_mutex_unlock(&mutex_modify_critical_section);
    //wake up all the waiting threads
    pthread_cond_broadcast(&cond);
}

/*******************************************End of Monitor********************************************/


int main(int argc, char* argv[]){
    int input_num = atoi(argv[1]);//get user input
    //initialize threads and slaves arrays
    pthread_t thread_array[input_num];
    struct slave* slave_array[input_num];
    
    printf("\n###### GOLD RUSH. GO! ######\n\n");
    printf("Start to catch %d slaves...\n", input_num);
    sleep(2);//consume some time to catch slaves
    
    int j;
    for(j = 0; j < input_num; j++){
        //populate the slave
        struct slave *populate = malloc(sizeof(struct slave));
        populate->ID = j+1;
        populate->submit_time = rand()%MAX_DIGTIME + 1;
        slave_array[j] = populate;
        //create a thread for the slave that executes dig_thread
        if(pthread_create(&thread_array[j], 0, (void*)dig_thread, (void*)slave_array[j])){
            printf("There was an error creating the thread\n");
            return -1;
        }else
            printf("Slave no.%2d gets ready!\n", slave_array[j]->ID);
    }
    //wait for each thread to terminate before exiting the program
    for(j = 0; j < input_num; j++){
        if(pthread_join(thread_array[j], NULL)){
            printf("There was an error in pthread_join.\n");
            return -1;
        }
    }
    printf("\n###### GOLD RUSH. END. YOUR TOTAL COINS: %d. ######\n", total_coin);
    return 0;
}