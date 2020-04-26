#include "ku_cfs.h"


process_queue* new_process_queue() {
	process_queue* queue = (process_queue*)calloc(1, sizeof(process_queue));
	process_node* head = new_process_node(0, (char*)NULL);
	process_node* tail = new_process_node(0, (char*)NULL);

	head->next_node = tail;
	tail->previous_node = head;
	queue->head = head;
	queue->tail = tail;

	return queue;
}

process_node* new_process_node(int nice_value, char* execution_argument) {
	process_node* node = (process_node*)calloc(1, sizeof(process_node));
	node->nice_value = nice_value;
	node->virtual_runtime = 0;
	node->execution_argument = execution_argument;

	return node;
}

pid_t new_process(char* execution_argument) {
	pid_t process_id = fork();
	if (process_id == 0) execl(process_file_path, process_file_path, execution_argument, NULL);

	return process_id;
}


process_node* insert_process_node(process_queue* queue, process_node* node) {
	process_node* insert_location;

	for (insert_location = queue->head->next_node;
		insert_location != queue->tail;
		insert_location = insert_location->next_node)
		if (insert_location->virtual_runtime > node->virtual_runtime) break;

	process_node* previous_node = insert_location->previous_node;
	process_node* next_node = insert_location;

	previous_node->next_node = node;
	node->previous_node = previous_node;
	next_node->previous_node = node;
	node->next_node = next_node;

	return node;
}

process_node* take_out_first_node(process_queue* queue) {
	process_node* first_node = queue->head->next_node;
	process_node* previous_node = queue->head;
	process_node* next_node = queue->head->next_node->next_node;

	previous_node->next_node = next_node;
	next_node->previous_node = previous_node;

	first_node->previous_node = NULL;
	first_node->next_node = NULL;

	return first_node;
}


void initialize_process_queue(process_queue* queue, int* process_numbers) {
	int current_char = 'A';
	for (int i = 0; i < nice_number; i++)
		for (int j = 0; j < process_numbers[i]; j++) {
			char* execution_argument = (char*)calloc(2, sizeof(char));
			execution_argument[0] = current_char;
			current_char++;
			insert_process_node(queue, new_process_node(i - 2, execution_argument));
		}
}

void initialize_processes(process_queue* queue) {
	for (process_node* node = queue->head->next_node; node != queue->tail; node = node->next_node) {
		node->process_id = new_process(node->execution_argument);
		while (!WIFSTOPPED(node->status))
			waitpid(node->process_id, &node->status, WUNTRACED);
	}

	currunt_running_node = take_out_first_node(queue);
	resume_process(currunt_running_node);
}


void resume_process(process_node* node) {
	struct timeval current_time;
	gettimeofday(&current_time, NULL);

	node->last_executed_time = current_time;
	kill(node->process_id, SIGCONT);

	while (!WIFCONTINUED(node->status))
		waitpid(node->process_id, &node->status, WCONTINUED);
}

void pause_process(process_node* node) {
	struct timeval current_time;
	gettimeofday(&current_time, NULL);
	kill(node->process_id, SIGSTOP);

	node->virtual_runtime += (
		+current_time.tv_sec
		+ (double)current_time.tv_usec / 1000000
		- currunt_running_node->last_executed_time.tv_sec
		- (double)currunt_running_node->last_executed_time.tv_usec / 1000000
		) * pow(1.25, currunt_running_node->nice_value);

	while (!WIFSTOPPED(node->status))
		waitpid(node->process_id, &node->status, WUNTRACED);
}

void kill_processes(process_queue* queue) {
	for (process_node* node = queue->head->next_node;
		node != queue->tail;
		node = node->next_node) {
		kill(node->process_id, SIGKILL);
		waitpid(node->process_id, NULL, 0);
	}
}

void set_timer(int sec) {
	struct itimerval timer;
	timer.it_interval.tv_sec = sec;
	timer.it_interval.tv_usec = 0;
	timer.it_value.tv_sec = sec;
	timer.it_value.tv_usec = 0;

	time_slice_counter = 0;
	setitimer(ITIMER_REAL, &timer, NULL);
}

void reschedule() {
	pause_process(currunt_running_node);
	insert_process_node(queue, currunt_running_node);

	currunt_running_node = take_out_first_node(queue);
	resume_process(currunt_running_node);

	time_slice_counter++;
	if (time_slice_counter >= time_slice_number) {
		pause_process(currunt_running_node);
		insert_process_node(queue, currunt_running_node);
		kill_processes(queue);
		exit(0);
	}
}

int main(int argc, char* argv[]) {
	int process_numbers[nice_number];
	for (int i = 0; i < nice_number; i++) process_numbers[i] = atoi(argv[i + 1]);
	time_slice_number = atoi(argv[nice_number + 1]);

	queue = new_process_queue();
	initialize_process_queue(queue, process_numbers);
	initialize_processes(queue);

	signal(SIGALRM, reschedule);
	set_timer(time_slice_size);

	while (1) pause();
}


/*
int main(int argc, char *argv[]){
    int i, n_process=0;
    double weights[5] = {1/(1.25*1.25), 1/1.25, 1, 1.25, (1.25*1.25)};
    char *alphabet[] ={"A","B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","X","Z","U","V","W","S","Y","Z"};
    for(i=1; i<argc-1; i++){
        // printf("%s %d\n",argv[i], atoi(argv[i]));
        n_process = n_process + atoi(argv[i]);
    }
    // printf("%d\n", n_process);
    int nicevalue_list[] ={atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5])};
    int n_nicevalue_list = argc-2;
    int counter=1;
    PCB *p_head = NULL, *p_tail = NULL, *p;
    int sum =0, temp=0;
    int pids[n_process];
    //Process Creation
    for(i=1; i<argc-1; i++){
        // printf("%d : %d\n", (i-3), nicevalue_list[i-1]);
        for(int a =0; a < nicevalue_list[i-1]; a++){
            AddPCB(&p_head, &p_tail, i-3);
            strcpy((*p_tail).name, alphabet[temp]);
            
            printf("%s\n", p_tail->name);
            int id = fork();
            

            if(id ==0){
                sleep(5);
                p_tail -> pid = getpid();
                printf("pid> %d\n", p_tail->pid);
                execl("./ku_app", "ku_app", (*p_tail).name, NULL);
                // p_tail -> pid = getpid();
                // printf("pid> %d\n", p_tail->pid);
                
            }else if(id <0){
                exit(-1);
            }else{

            }
            // printf("%d ", temp);
            temp++;
        }
    }
    sleep(5);
    p = p_head;
    while(NULL != p){
        if(p != p_head) printf(" / ");
        printf(" %s %d %d ", p -> name, p -> nice_value, p->pid);
        p = p->p_next;
    }
*/
    // printf("%d   %s\n",p_tail->pid,"DONE!");
    // create 
    
    // for(i=0; i<1; i++){
    //     // printf("%d\n", i);
    //     pid = fork();
    //     if(pid <0){
    //         fprintf(stderr, "Fork Failed");
    //         exit(-1);
    //     }else if (pid==0){
    //         // child process
    //         // printf("%s", alphabet[i]);
    //         execl("./ku_app","ku_app","A",NULL);
            
    //         sleep(5);

    //         // printf("%ld\n", (long)getpid());
    //     }

    // }

    // printf("%d \n", n_process);



}
