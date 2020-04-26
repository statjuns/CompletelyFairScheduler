
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <math.h>
#include <time.h>
#include <string.h>
typedef struct task{
    int pid;
    int status;
	char *name;
	int complete_time;
	int epochs;
	int nice_value;
    double vruntime;
    struct timeval last_executed_time;
    struct task *p_next;
    struct task *p_previous;
} PCB;

double weights[5] = {1/(1.25*1.25), 1/1.25, 1, 1.25, (1.25*1.25)};
typedef struct PCB_queue_{
    PCB* head;
    PCB* tail;
}PCB_queue;

PCB_queue* pcb_queue;
PCB* currunt_running_node;
int time_slice_number, time_slice_counter;
int time_slice_sec =1;
#define nice_number 5
PCB_queue* create_pcb_queue();
PCB* create_pcb(int nice_value, char *name);
pid_t create_process(char* name);

PCB* insert_pcb(PCB_queue* queue, PCB* node);
PCB* take_first_pcb(PCB_queue* queue);
void init_pcb_queue(PCB_queue* queue, int* process_numbers);
void init_process(PCB_queue* queue);

void resume_process(PCB* node);
void pause_process(PCB* node);
void kill_process(PCB_queue* queue);

void set_timer(int sec);
void reschedule();


void AddPCB(PCB **pp_head, PCB **pp_tail, int nice_value){
    if( NULL != *pp_head){
        (*pp_tail) -> p_next = (PCB *)malloc(sizeof(PCB));
        *pp_tail = (*pp_tail) ->p_next;
    }else{
        *pp_head = (PCB *)malloc(sizeof(PCB));
        *pp_tail = *pp_head;
    }
    // (*pp_tail) -> pid = pid;
    (*pp_tail) -> nice_value = nice_value;
    (*pp_tail) ->vruntime = 0;
    // (*pp_tail) -> name = &name;
    // (*pp_tail) -> name = *name;
    (*pp_tail) -> p_next = NULL;
}