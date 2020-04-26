
#include "ku_cfs.h"

PCB_queue* create_pcb_queue(){
    PCB_queue* queue = (PCB_queue*)malloc(sizeof(PCB_queue));
    PCB* head = create_pcb(0, (char*) NULL);
    PCB* tail = create_pcb(0, (char*) NULL);

    head -> p_next = tail;
    tail -> p_previous = head;
    queue -> head = head;
    queue -> tail = tail;

    return queue;
}

PCB* create_pcb(int nice_value, char* name){
    PCB* node = (PCB*)malloc(sizeof(PCB) * 1);
    node -> nice_value = nice_value;
    node -> vruntime = 0;
    node -> name = name;

    return node;
}

pid_t create_process(char* name){
    pid_t process_id = fork()
    if(process_id == 0) execl("./ku_app", "ku_app", name, NULL);
    return process_id;
}


PCB_queue* insert_pcb(PCB_queue* queue, PCB* node){
    PCB* insert_location;

    for(insert_location = queue->head->p_next; insert_location != queue ->tail;
    insert_location = insert_location->p_next){
        // head부터 시작해서 NULL인곳 까지 반복
        if(insert_location -> vruntime > node ->vruntime)break;
    }

    PCB* p_previous = insert_location -> p_previous;
    PCB* p_next = insert_location;

    p_previous -> p_next = node;
    node -> p_previous = p_previous;
    p_next -> p_previous = node;
    node -> p_next = p_next;
    return node;
    // 입력으로 들어온 node의 vruntime보다 큰 inserttion location을 찾고 거기다 입력으로 들어온 node를 삽입
    // 삽입시 insertion의 p_previous를 삽입된 node의 p_previous로 보냄
}

PCB* take_first_pcb(PCB_queue* queue){
    PCB* first_node = queue -> head -> p_next;
    PCB* previous_node = queue -> head;
    PCB* next_node = queue -> head -> p_next -> p_next;

    previous_node -> next_node = next_node;
    next_node -> p_previous  = previous_node;

    first_node -> p_previous = NULL;
    first_node -> p_next = NULL;
    
    return first_node;
    // 첫번째 pcb만 꺼내고 p_prvious, p_next 초기화
}

void init_pcb_queue(PCB_queue* queue, int* process_numbers){
    int current_char = 'A';
    for(int i = 0; i < nice_number; i++){
        for(int j =0; j < process_numbers[i]; j++){
            char* execution_argument = (char*)malloc(sizeof(char) * 2);
            execution_argument[0] = current_char;
            current_char++;
            insert_pcb(queue, create_process(i-2, execution_argument));
        }
    }
}

void init_process(PCB_queue* queue){
    for(PCB* node = queue->head->p_next; node != queue->tail; node = node->p_next){
        node -> pid = create_process(node -> name);
        while(!WIFSTOPPED(node->status)){
            //반환의 원인이 된 자식프로세스가 현재 정지되어 있다면 참을 반환한다.
            waitpid(node->pid, &node->status, WUNTRACED);
        }
    }
    currrunt_running_node = take_first_pcb(queue);
    resume_process(currunt_running_node);
}


void resume_process(PCB* node){
    struct timeval current_time;
    gettimeofday(&current_time, NULL);

    node -> last_executed_time = current_time;
    kill(node->pid, *node->status, WCONTINUED);
}

void pause_process(PCB* node){
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    kill(node -> pid, SIGSTOP);

    node -> vruntime += weights[currunt_running_node->nice_value+2]
    // deltaexec 1로 고정
    while(!WIFSTOPPED(node -> status)){
        waitpid(node->pid, &node->status, WUNTRACED);
    }
}

void kill_process(PCB_queue* queue){
    for (PCB* node = queue -> head -> p_next; 
    node != queue->tail;
    node = node->p_next){
        kill(node->pid, SIGKILL);
        waitpid(node->pid, NULL, 0);
    }
}

void set_timer(int sec){
    struct itimerval timer;
    timer.it_interval.tv_sec = sec;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = sec;
    timer.it_value.tv_usec = 0;

    time_slice_counter = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
}

void reschedule(){
    pause_process(currunt_running_node);
    insert_pcb(queue,  currunt_running_node);

    currunt_running_node = take_first_pcb(queue);
    resume_process(currunt_running_node);

    time_slice_counter++;
    if(time_slice_counter >= time_slice_number){
        pause_process(currunt_running_node);
        insert_pcb(queue, currunt_running_node);
        kill_process(queue);
        exit(0);
    }
}

int main(int argc, char* argv[]){
    int process_numbers[nice_number];
    for(int i=0; i < nice_number; i++) process_numbers[i] = atoi(argv[i+1]);
    time_slice_number = atoi(argv[nice_number + 1]);

    queue = create_pcb_queue();
    init_pcb_queue(queue, process_numbers);
    init_process(queue);

    signal(SIGALRM, reschedule);
    set_timer(time_slice_sec);

    while(1) pause();
}