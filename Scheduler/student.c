/*
 * student.c
 * Multithreaded OS Simulation for CS 2200, Project 5
 * Spring 2016
 *
 * This file contains the CPU scheduler for the simulation.
 * Name: Anna McAbee
 * GTID: 902944955
 */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
 #include <string.h>


#include "os-sim.h"


/*
 * current[] is an array of pointers to the currently running processes.
 * There is one array element corresponding to each CPU in the simulation.
 *
 * current[] should be updated by schedule() each time a process is scheduled
 * on a CPU.  Since the current[] array is accessed by multiple threads, you
 * will need to use a mutex to protect it.  current_mutex has been provided
 * for your use.
 */
static pcb_t **current;
static pthread_mutex_t current_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t other_mutex = PTHREAD_MUTEX_INITIALIZER;
static int scheduleType;
//static pthread_cont_t idle_mutex;
static pthread_cond_t idle_cond = PTHREAD_COND_INITIALIZER;
static int timeslice;
static int cpu_count;
struct QUEUE_NODE
{
    pcb_t* data;
    struct QUEUE_NODE* next;
};

typedef struct 
{
    int size;
    struct QUEUE_NODE* head;
    pthread_mutex_t* mutex;

} READY_QUEUE;

READY_QUEUE* ready_queue;

static int timeslice;
void queue_initialize(int cpu_count) {
    ready_queue = (READY_QUEUE*) malloc(sizeof(READY_QUEUE*));
    pthread_mutex_t mut_init = PTHREAD_MUTEX_INITIALIZER;
    //QUEUE_NODE* node = (QUEUE_NODE*) malloc(sizeof(QUEUE_NODE*));
    ready_queue->size = 0;
    ready_queue->head=NULL;
    ready_queue->mutex= &mut_init;
    //printf("Done initializing!\n");
    return;
}

void push(pcb_t* process, READY_QUEUE* ready_queue) {
    //printf("Adding to ready_queue\n");
    pthread_mutex_lock(&other_mutex);
    struct QUEUE_NODE* new_node = (struct QUEUE_NODE*) malloc(sizeof(struct QUEUE_NODE*));
    new_node->data=process;
    //printf("Made a new node: with process %d\n", new_node->data->pid);
    if (ready_queue->head==NULL || ready_queue->size==0) {
       // printf("adding to an empty ready_queue\n");
        ready_queue->head=new_node;
        ready_queue->size++;
    } else {
                //printf("adding to a non empty ready_queue\n");
       //                     printf("old ready_queue size: %d\n", ready_queue->size);

        struct QUEUE_NODE* curr_node = ready_queue->head;
      //  printf("Curr process: %d \n", curr_node->data->pid);
        int i = ready_queue->size;
       // printf("i: %d \n", i);
        if (i == 1) {
            curr_node->next = new_node;
            ready_queue->size=(ready_queue->size+1);
        } else {
            while (curr_node->next != NULL && i > 1) {
                //            printf("Curr process in while loop: %d \n", curr_node->data->pid);
    //                                    printf("Next process in while loop: %d \n", curr_node->next->data->pid);

                i--;
                curr_node = curr_node->next;
            }
    //                printf("Curr process: %d \n", curr_node->data->pid);

            curr_node->next = new_node;
           // printf("set next\n");
            //new_node->prev = (struct QUEUE_NODE*)curr_node;
            ready_queue->size=(ready_queue->size+1);
        }

//            printf("\n\nnew ready_queue size: %d\n", ready_queue->size);


    }
        pthread_cond_broadcast(&idle_cond);

    pthread_mutex_unlock(&other_mutex);
    //printf("Done adding to ready_queue\n");

}

pcb_t* pop(READY_QUEUE* ready_queue) {
   // printf("Popping from ready_queue\n");
    pcb_t* ret_data;
    pthread_mutex_lock(&other_mutex);
    struct QUEUE_NODE* curr_head = ready_queue->head;
   // printf("1\n");
   // printf("ready_queue size: %d\n", ready_queue->size);
    if (curr_head != NULL && ready_queue->size > 0) {
      /// printf("Popping from not null head\n");
           //    printf("RETURN DATA: %d \n", curr_head->data->pid);

        ret_data = curr_head->data;
       // printf("Found return data\n");
       // printf("3\n");
        if (curr_head->next != NULL) {
            ready_queue->head= curr_head->next;
            //ready_queue->head->prev= NULL;
        //    printf("We have a new head!\n");
        } else {
            //printf("ready_queue is going to be empty now\n");
            ready_queue->head=NULL;
        }
        //printf("About to free\n");

        free(curr_head);
          //      printf("Freed\n");
            ready_queue->size=(ready_queue->size-1);

    } else {
        //printf("Trying to pop from empty ready_queue\n");
        ret_data=NULL;
    }
    pthread_mutex_unlock(&other_mutex);
    //printf("Done popping from ready_queue\n");
    return ret_data;
}

pcb_t* remove_high_priority(READY_QUEUE* ready_queue) {
   // printf("Removing high priority\n");
    pcb_t* ret_data;
    pthread_mutex_lock(&other_mutex);
    struct QUEUE_NODE* curr_head = ready_queue->head;
    if (ready_queue->size==0) {
        ret_data= NULL;
    } else if (ready_queue->size==1) {
        ret_data= curr_head->data;
        ready_queue->head=NULL;
        ready_queue->size=(ready_queue->size-1);
        free(curr_head);
    } else {
       // printf("1\n");
        int highest_priority = curr_head->data->static_priority;
       // printf("Original priority: %d \n", highest_priority);
        struct QUEUE_NODE* highest_node = curr_head;
        //printf("ready_queue size: %d\n", ready_queue->size);
        int i = ready_queue->size;
        while(curr_head->next != NULL && i > 1) {
                   // printf("whilin\n");
            if (curr_head->data->static_priority > highest_priority) {
                highest_node= curr_head;
                highest_priority = curr_head->data->static_priority;
            }
            //printf("Other priority: %d\n", curr_head->next->data->static_priority);
            i--;
            curr_head = curr_head->next;
        }
           // printf("meowww\n");
           // printf("\n\nFound highest priority: %d out of %d size \n\n", highest_priority, ready_queue->size);
                       // printf("2\n");

        // found higest priority, now remove it
        curr_head = ready_queue->head;
        if (curr_head == highest_node) {
            ready_queue->head=curr_head->next;
        }  else {
            while (curr_head->next != highest_node && curr_head->next != NULL) {
                curr_head = curr_head->next;
            }
            if (highest_node->next != NULL) {
                curr_head->next = highest_node->next;
            } else {
                curr_head->next = NULL;
            }
        }
        ready_queue->size=(ready_queue->size-1);
                ret_data = highest_node->data;
        //printf("3\n");

        free(highest_node);
    }
    pthread_mutex_unlock(&other_mutex);
        //printf("Done removing high priority\n");

    return ret_data;

}

/*
 * schedule() is your CPU scheduler.  It should perform the following tasks:
 *
 *   1. Select and remove a runnable process from your ready queue which 
 *	you will have to implement with a linked list or something of the sort.
 *
 *   2. Set the process state to RUNNING
 *
 *   3. Call context_switch(), to tell the simulator which process to execute
 *      next on the CPU.  If no process is runnable, call context_switch()
 *      with a pointer to NULL to select the idle process.
 *	The current array (see above) is how you access the currently running
 *	process indexed by the cpu id. See above for full description.
 *	context_switch() is prototyped in os-sim.h. Look there for more information 
 *	about it and its parameters.
 */
static void schedule(unsigned int cpu_id)
{
    /* FIX ME */
    pcb_t* pcb_t = NULL;
    pthread_mutex_lock(&current_mutex);
    if (scheduleType == 0) {
        pcb_t = pop(ready_queue);
        timeslice = -1;
        /** FIFO */
    } else if (scheduleType == 1) {
        /** Round Robin */
        pcb_t = pop(ready_queue);
        printf("Using time slice of : %d\n", timeslice);
    } else if (scheduleType == 2) {
        pcb_t = remove_high_priority(ready_queue);
        timeslice=-1;
        /** Static Priority */
    }
    if (pcb_t != NULL) {
        pcb_t->state = PROCESS_RUNNING;
       // printf("Process running!\n");
    } 
    current[cpu_id] = pcb_t;
    pthread_mutex_unlock(&current_mutex);
    context_switch(cpu_id, pcb_t, timeslice);

}


/*
 * idle() is your idle process.  It is called by the simulator when the idle
 * process is scheduled.
 *
 * This function should block until a process is added to your ready queue.
 * It should then call schedule() to select the process to run on the CPU.
 */
extern void idle(unsigned int cpu_id)
{
    // done
   // schedule(0);

    /*
     * REMOVE THE LINE BELOW AFTER IMPLEMENTING IDLE()
     *
     * idle() must block when the ready queue is empty, or else the CPU threads
     * will spin in a loop.  Until a ready queue is implemented, we'll put the
     * thread to sleep to keep it from consuming 100% of the CPU time.  Once
     * you implement a proper idle() function using a condition variable,
     * remove the call to mt_safe_usleep() below.
     */
    //mt_safe_usleep(1000000);
    pthread_mutex_lock(&other_mutex);
    while (ready_queue->head == NULL) {
        pthread_cond_wait(&idle_cond, &other_mutex);
    }
    pthread_mutex_unlock(&other_mutex);
    schedule(cpu_id);
}


/*
 * preempt() is the handler called by the simulator when a process is
 * preempted due to its timeslice expiring.
 *
 * This function should place the currently running process back in the
 * ready queue, and call schedule() to select a new runnable process.
 */
extern void preempt(unsigned int cpu_id)
{
    // done

    //schedule(cpu_id);
    // lock and put proceduce in ready queue, set state to ready**
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_READY;
    pthread_mutex_unlock(&current_mutex);
    // and a sched8le
    push(current[cpu_id], ready_queue);
    schedule(cpu_id);
}


/*
 * yield() is the handler called by the simulator when a process yields the
 * CPU to perform an I/O request.
 *
 * It should mark the process as WAITING, then call schedule() to select
 * a new process for the CPU.
 */
extern void yield(unsigned int cpu_id)
{
    //done
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_WAITING;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);
}


/*
 * terminate() is the handler called by the simulator when a process completes.
 * It should mark the process as terminated, then call schedule() to select
 * a new process for the CPU.
 */
extern void terminate(unsigned int cpu_id)
{
    // done
    pthread_mutex_lock(&current_mutex);
    current[cpu_id]->state = PROCESS_TERMINATED;
    pthread_mutex_unlock(&current_mutex);
    schedule(cpu_id);

}


/*
 * wake_up() is the handler called by the simulator when a process's I/O
 * request completes.  It should perform the following tasks:
 *
 *   1. Mark the process as READY, and insert it into the ready queue.
 *
 *   2. If the scheduling algorithm is static priority, wake_up() may need
 *      to preempt the CPU with the lowest priority process to allow it to
 *      execute the process which just woke up.  However, if any CPU is
 *      currently running idle, or all of the CPUs are running processes
 *      with a higher priority than the one which just woke up, wake_up()
 *      should not preempt any CPUs.
 *	To preempt a process, use force_preempt(). Look in os-sim.h for 
 * 	its prototype and the parameters it takes in.
 */
extern void wake_up(pcb_t *process)
{
    /* FIX static priority */
    process->state = PROCESS_READY;
    push(process, ready_queue);
    if (scheduleType == 2) {
        /** Static prioriy **/
        pthread_mutex_lock(&current_mutex);
        int low = 10;
        int index;
        int x = -1;
        for(index=0; index < cpu_count; index++) {
                if(current[index] == NULL) {
                    x = -1;
                    break;
                }
                if(current[index]->static_priority < low) {
                    low = current[index]->static_priority;
                    x = index;
                }
        }
        pthread_mutex_unlock(&current_mutex);
        if(low < process->static_priority && x != -1) {
            force_preempt(x); 
        }

    }
}


/*
 * main() simply parses command line arguments, then calls start_simulator().
 * You will need to modify it to support the -r and -p command-line parameters.
 */
int main(int argc, char *argv[])
{
        printf("Meow1\n");

    /* Parse command-line argcguments */
        printf("Arg length: %d\n", argc);

    cpu_count = atoi(argv[1]);
    printf("cpu_count : %d\n", cpu_count);
    printf("arg type: %s\n", argv[2]);
    queue_initialize(cpu_count);
    scheduleType = 0;
    int i;
    for (i = 0; i < argc; i++) {
        if(strcmp(argv[i],"-r")==0) {
            printf("Round-Robin Scheduler\n");
            //printf("Timeslice of: %ld\n", strtol(argv[i+1], NULL, 0));
            timeslice = (int) strtol(argv[i+1], NULL, 0);
            scheduleType = 1;
            printf("Timeslice of: %d\n", timeslice);
        } else if (strcmp(argv[i],"-p")==0) {
            printf("Priority Scheduler\n");
            scheduleType = 2;

        }
    }

    //printf("Meow\n");
    /* FIX ME - Add support for -r and -p parameters*/

    /* Allocate the current[] array and its mutex */
    current = malloc(sizeof(pcb_t*) * cpu_count);
    assert(current != NULL);
    pthread_mutex_init(&current_mutex, NULL);

    /* Start the simulator in the library */
    start_simulator(cpu_count);

    return 0;

}


