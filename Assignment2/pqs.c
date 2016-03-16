#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200112L

#include "pqs.h"

#define MAX_CUSTOMERS 100

/********global variables (critical section)*****************************************/
// two mutex
pthread_mutex_t mutex_modify_critical_section = PTHREAD_MUTEX_INITIALIZER; // Critical section
pthread_mutex_t mutex_make_request = PTHREAD_MUTEX_INITIALIZER; // Make request
// three convar
pthread_cond_t cond_available = PTHREAD_COND_INITIALIZER; // Service available
pthread_cond_t cond_override = PTHREAD_COND_INITIALIZER; // Customer override
pthread_cond_t cond_batch = PTHREAD_COND_INITIALIZER; // Batch ready

int customer_count = 0; // Total customers for simulation
int num_waiting = 0; // Customers that have arrived but have not been served
int arrival_batch = 0; // Counter for batching same time arrival customers
int service_busy = 0; // Flag for if a customer is actively being served

struct timeval start, end; // Variables for the simulation clock

struct customer* waiting_customers[MAX_CUSTOMERS]; // Array of customers that have arrived but are yet to be served
struct customer* customer_array[MAX_CUSTOMERS]; // Array of all customers in the simulation

void* customer_thread(struct customer* customer) {
    struct timespec timeToWait;
    struct timeval now;
    int result;

    // Wait for arrival time
    usleep(customer->arrival_time*1000000);
    printf("customer %2d arrives: arrival time (%.2f), service time (%.1f), priority (%2d). \n", customer->customer_no, customer->arrival_time, customer->service_time, customer->priority);

    // Add customer to queue
    pthread_mutex_lock(&mutex_modify_critical_section);
    waiting_customers[num_waiting] = customer;
    num_waiting++;
    pthread_mutex_unlock(&mutex_modify_critical_section);

    while (1) {
        // Wait for service
        request_service(customer);
        service_busy++;
        pthread_mutex_lock(&mutex_modify_critical_section);
        printf("The clerk starts serving customer %2d at time %.2f. \n", customer->customer_no, relative_time());
        pthread_mutex_unlock(&mutex_modify_critical_section);

        // Set time to wait
        gettimeofday(&now, NULL);
        timeToWait.tv_sec = now.tv_sec;
        timeToWait.tv_nsec = now.tv_usec * 1000;
        timeToWait.tv_sec += (customer->service_time);

        // Receive service
        pthread_mutex_lock(&mutex_modify_critical_section);
        result = pthread_cond_timedwait(&cond_override, &mutex_modify_critical_section, &timeToWait);
        pthread_mutex_unlock(&mutex_modify_critical_section);

        // If service was not interrupted, break the while loop and release service
        if (result != 0) {
            pthread_mutex_lock(&mutex_modify_critical_section);
            printf("The clerk finishes the service to customer %2d at time %d. \n", customer->customer_no, relative_time_int());
            pthread_mutex_unlock(&mutex_modify_critical_section);
            break;
        }

    }
    release_service();
    return 0;
}

// What to do when a customer arrives
void request_service(struct customer* customer){
    pthread_mutex_lock(&mutex_make_request);

    // Check if this customer needs to wait for their arrival batch
    wait_for_arrival_batch(customer);
    
    // If this customer has higher priority than the active customer
    if(customer->priority > waiting_customers[0]->priority) {
        pthread_mutex_lock(&mutex_modify_critical_section);
        // Override the active customer
        pthread_cond_broadcast(&cond_override);
        printf("customer %2d interrupts the service of lower-priority customer %2d. \n", customer->customer_no, waiting_customers[0]->customer_no);
        // Swap them into the active customer position
        swap_to_front(customer);
        pthread_mutex_unlock(&mutex_modify_critical_section);
        pthread_mutex_unlock(&mutex_make_request);
        return;
    }

    // While a customer is not next in line
    while(customer->customer_no != waiting_customers[0]->customer_no){
        pthread_mutex_lock(&mutex_modify_critical_section);
        printf("customer %2d waits for the finish of customer %2d. \n", customer->customer_no, waiting_customers[0]->customer_no);
        pthread_mutex_unlock(&mutex_modify_critical_section);
        // Wait until service is available again to re-check
        pthread_cond_wait(&cond_available, &mutex_make_request);
    }
    
    // Customer being serviced, release request mutex
    pthread_mutex_unlock(&mutex_make_request);
    return;
}

// Function to synchronize all customers arriving at the same time
void wait_for_arrival_batch(struct customer* customer) {
    int i, batch_count = 0;
    pthread_mutex_lock(&mutex_modify_critical_section);
    // Count the number of customers that share the same arrival time
    for (i = 0; i < customer_count; i++) {
        if (customer_array[i]->arrival_time == customer->arrival_time) {
            batch_count++;
        }
    }
    // Increment the batch counter
    arrival_batch++;
    // If the batch counter is full, the batch has all arrived
    if (arrival_batch >= batch_count) {
        //Reset batch counter
        arrival_batch = 0;
        pthread_mutex_unlock(&mutex_modify_critical_section);
        // Sort batch, and release them
        sort_waiting_customers();
        pthread_cond_broadcast(&cond_batch);
    }
    else {
        // Wait for full batch to arrive
        pthread_mutex_unlock(&mutex_modify_critical_section);
        pthread_cond_wait(&cond_batch, &mutex_make_request);
    }
    return;
}

void release_service() {
    pthread_mutex_lock(&mutex_modify_critical_section);
    int i;
    // Shift all customers up in the array, decrement num_waiting and service busy
    for (i = 0; i < num_waiting; i++)
        waiting_customers[i] = waiting_customers[i+1];
    num_waiting--;
    service_busy--;
    // Sort customers before determining the next active customer
    sort_waiting_customers();
    pthread_mutex_unlock(&mutex_modify_critical_section);

    // Wake up customer threads to check if they are next in line
    pthread_cond_broadcast(&cond_available);
}

// Helper function to know which sort function to call
void sort_waiting_customers() {
    if (service_busy) {
        sort_inactive_waiting_customers();
    }
    else {
        sort_all_waiting_customers();
    }
    return;
}

// Sort all waiting customers (including the one being served)
void sort_all_waiting_customers() {
    int i, swapped, result;
    struct customer *temp = malloc(sizeof(struct customer));

    while (1) {
        swapped = 0;
        for (i = 0; i < num_waiting-1; i++) {
            result = compare_customer_priority(waiting_customers[i], waiting_customers[i+1]);
            if (result == -1) {
                temp = waiting_customers[i];
                waiting_customers[i] = waiting_customers[i+1];
                waiting_customers[i+1] = temp;
                swapped = 1;
            }
        }
        if (swapped == 0) {
            break;
        }
    }
    return;
}

// Sort all waiting customers except the one actively being served
// These sort functions could definitely have been rewritten as 1, but my brain hurts and this gets the job done
void sort_inactive_waiting_customers() {
    int i, swapped, result;
    struct customer *temp = malloc(sizeof(struct customer));

    while (1) {
        swapped = 0;
        for (i = 1; i < num_waiting-2; i++) {
            result = compare_customer_priority(waiting_customers[i], waiting_customers[i+1]);
            if (result == -1) {
                temp = waiting_customers[i];
                waiting_customers[i] = waiting_customers[i+1];
                waiting_customers[i+1] = temp;
                swapped = 1;
            }
        }
        if (swapped == 0) {
            break;
        }
    }
    return;
}

// Helper function to move a customer to the front of the queue (used for overrides)
void swap_to_front(struct customer* customer) {
    int i;
    struct customer *temp = malloc(sizeof(struct customer));
    for (i = 0; i < num_waiting; i++) {
        if (waiting_customers[i]->customer_no == customer->customer_no) {
            temp = waiting_customers[0];
            waiting_customers[0] = waiting_customers[i];
            waiting_customers[i] = temp;
        }
    }
    return;
}

// Comparison function used in the swap methods (+1: correct order, -1: needs to be swapped)
int compare_customer_priority(struct customer* customer1, struct customer* customer2) {
    if (customer1->priority > customer2->priority) {
        return +1;
    }
    else if (customer2->priority > customer1->priority) {
        return -1;
    }
    else if (customer1->arrival_time < customer2->arrival_time) {
        return +1;
    }
    else if (customer2->arrival_time < customer1->arrival_time) {
        return -1;
    }
    else if (customer1->service_time < customer2->service_time) {
        return +1;
    }
    else if (customer2->service_time < customer1->service_time) {
        return -1;
    }
    else if (customer1->line_no > customer2->line_no) {
        return +1;
    }
    else {
        return -1;
    }
}

// Helper function to get simulation timer as a float
float relative_time() {
    float elapsed;
    gettimeofday(&end, NULL);
    elapsed = (end.tv_sec-start.tv_sec)*1000000 + (end.tv_usec-start.tv_usec);
    return elapsed/1000000;
}

// Helper function to get simulation timer as an int
int relative_time_int() {
    int elapsed;
    gettimeofday(&end, NULL);
    elapsed = (end.tv_sec-start.tv_sec) + (end.tv_usec-start.tv_usec)/1000000;
    return elapsed;
}

int main(int argc, char* argv[])
{
    FILE *fp;
    char str[60];
    int j, customer_no, priority;
    double arrival_time, service_time;

    // Record start time for timer readings
    gettimeofday(&start, NULL);

    // Check that input file is given
    if (argc < 2) {
        fprintf(stderr, "No input argument provided\n");
        return -1;
    }

    // Open input file
    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        perror("Invalid input file provided");
        return -1;
    }

    // Check first line of input file for customer count (assuming correct format)
    fgets(str, 60, fp);
    customer_count = atoi(str);

    // If valid customer count is received
    if (customer_count > 0) {
        // Initialize threads and customers
        pthread_t thread_array[customer_count];

        // For each line in input file
        for(j = 0; j < customer_count; j++) {
            fgets(str, 60, fp);
            sscanf(str, "%d:%lf,%lf,%d", &customer_no, &arrival_time, &service_time, &priority);

            // Create customer
            struct customer *new_customer = malloc(sizeof(struct customer));
            new_customer->customer_no = customer_no;
            new_customer->arrival_time = arrival_time/10;
            new_customer->service_time = service_time/10;
            new_customer->priority = priority;
            new_customer->line_no = j;

            customer_array[j] = new_customer;

            // Assign the to a customer_thread to wait until their arrival time
            if(pthread_create(&thread_array[j], 0, (void*)customer_thread, (void*)new_customer)) {
                printf("There was an error creating the thread\n");
                return -1;
            }
            
        }
        // Join threads as they complete
        for(j = 0; j < customer_count; j++){
            if(pthread_join(thread_array[j], NULL)){
                printf("There was an error in pthread_join\n");
                return -1;
            }
        }
    }
    // Close input file
    fclose(fp);
    return 0;
}