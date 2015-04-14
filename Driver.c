#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>
void *prof_thread(void *t);
void *student_thread(void *t);

pthread_mutex_t queue_lock;
pthread_mutex_t student_check_lock;
pthread_cond_t empty_queue_check;
pthread_cond_t full_queue_check;
pthread_mutex_t pid_lock;
pthread_mutex_t sid_lock;
pthread_barrier_t exit_barrier;
struct Exit{
	int valid;
}e;

struct assignment{
	int id;
	int hours;
	int profID;
	int student_count;
};

struct queue{
	int size;
	int count;
	int tail;
	int head;
	struct assignment *data;
}q;

void insert(struct assignment A){
	q.data[q.tail] = A;
	q.tail = (q.tail + 1) % q.size;
	q.count++;
}
int get_head_ID(){
	return q.data[q.head].id;
}
struct assignment access_head(){
	pthread_mutex_lock(&student_check_lock);
	int num_students = q.data[q.head].student_count;
	if(num_students > 1){
		q.data[q.head].student_count--;
		pthread_mutex_unlock(&student_check_lock);
		struct assignment A = q.data[q.head];
		return A;
	}
	else{
		struct assignment A = q.data[q.head];
		q.head = (q.head + 1) % q.size;
		q.count--;
		pthread_mutex_unlock(&student_check_lock);
		return A;	
	}

}
void *prof_thread(void *t){
	int * p = (int *)t;
	int num_assignings = p[0];
	int min_wait = p[1];
	int max_wait = p[2];
	int min_assgnmnts = p[3];
	int max_assgnmnts = p[4];
	int min_time = p[5];
	int max_time = p[6];
	int students_per_assignment = p[7];
	int pid = p[8];
	pthread_mutex_lock(&pid_lock);
	p[8]++;
	pthread_mutex_unlock(&pid_lock);
	printf("STARTING Professor %d\n", pid);
	srand(time(NULL));
	int IDS = 1;
	for(int i = 0; i < num_assignings; i++){
		int rand_wait = (rand() % (max_wait + 1 - min_wait)) + min_wait;
		sleep(rand_wait);
		int rand_num_assgn = (rand() % (max_assgnmnts + 1 - min_assgnmnts)) + min_assgnmnts;
		for(int j = 0; j < rand_num_assgn; j++){
			int rand_time = (rand() % (max_time + 1 - min_time)) + min_time;
			struct assignment A = { id: IDS, hours:rand_time, profID:pid , student_count:students_per_assignment };
			IDS++;
			e.valid = 0;
			pthread_mutex_lock(&queue_lock);
			while(q.count == q.size){
				pthread_cond_wait(&full_queue_check, &queue_lock);	
			}	
			insert(A);
			printf("ASSIGN Professor %d adding Assignment %d: %d Hours.\n", A.profID, A.id, rand_time); 
			pthread_mutex_unlock(&queue_lock);
			pthread_cond_signal(&empty_queue_check);
		}
	}
	pthread_barrier_wait(&exit_barrier);
	e.valid = 1;
	printf("EXITING Professor %d\n", pid);
}

void *student_thread(void *p){
	int *t = (int *)p;
	int num_assign = t[0];
	int sid = t[1];
	pthread_mutex_lock(&sid_lock);
	t[1]++;
	pthread_mutex_unlock(&sid_lock);
	int max_num_assignments = num_assign * 100;
	printf("STARTING Student %d\n" , sid);
	int *seen_id = malloc(sizeof(int) * max_num_assignments);
	while(1){
		pthread_mutex_lock(&queue_lock);
		while(q.count == 0){
			pthread_cond_wait(&empty_queue_check, &queue_lock);
		}
		
		int seen_flag = 0;
		for(int i = 0; i < max_num_assignments; i++){
			if(seen_id[i] == get_head_ID()){
				seen_flag = 1;
			}
		}	
		if(seen_flag){
			continue;
		}	
		pthread_mutex_unlock(&queue_lock);
		pthread_cond_signal(&full_queue_check);
		struct assignment A = access_head();
		int waiting_time = A.hours;
		printf("BEGIN Student %d working on Assignment %d from Professor %d.\n", sid, A.id, A.profID);
		for(int i = 0; i < waiting_time; i++){
			printf("WORK Student %d working on Assignment %d Hour %d from Professor %d\n", sid, A.id, i+1, A.profID);
			sleep(1);
		}
		printf("END Student %d working on Assignment %d from Professor %d\n", sid, A.id, A.profID);
		if(e.valid == 1 && q.count == 0){
			break;
		}
	}
	printf("EXITING Student %d\n" , sid);
}

int main(int argc, char** argv){
	int aval = 10;
	int wval = 1;
	int Wval = 5;
	int nval = 1;
	int Nval = 10;
	int hval = 1;
	int Hval = 5;
	int pval = 2;
	int sval = 2;
	int dval = 2;
	int qval = 8;
	
	int o;
	opterr = 0;
	while((o = getopt(argc, argv, "a:w:W:n:N:h:H:p:s:d:q:"))!= -1){
		switch(o)
		{
			case 'a':
				aval = atoi(optarg);
				if(aval > 100000){
					aval = 10;
				}
				break;
			case 'w':
				wval = atoi(optarg);
				if(wval > 10){
					wval = 1;
				}
				break;
			case 'W':
				Wval = atoi(optarg);
				if(Wval > 100){
					Wval = 5;
				}
				break;
			case 'n':
				nval = atoi(optarg);
				if(nval > 10){
					nval = 1;
				}
				break;
			case 'N':
				Nval = atoi(optarg);
				if(Nval > 100){
					Nval = 10;
				}
				break;
			case 'h':
				hval = atoi(optarg);
				if(hval > 5){
					hval = 1;
				}
				break;
			case 'H':
				Hval = atoi(optarg);
				if(Hval > 10){
					Hval = 5;
				}
				break;
			case 'p':
				pval = atoi(optarg);
				if(pval > 10){
					pval = 2;
				}
				break;
			case 's':
				sval = atoi(optarg);
				if(sval > 10){
					sval = 2;
				}
				break;
			case 'd':
				dval = atoi(optarg);
				break;
			case 'q':
				qval = atoi(optarg);
				if(qval > 256){
					qval = 8;
				}
				break;
			default:
				abort();
		}
	}
	if(dval > 10){
		dval = sval;
	}
	if((wval > Wval) || (aval == 0) || (nval > Nval) || (hval > Hval) || (dval == 0) || (sval < dval)){
		printf("Invalid argument(s)");
		exit(1);
	}
	pthread_barrier_init(&exit_barrier, NULL, pval);
	pthread_t *professors =  malloc(sizeof(pthread_t) * pval);
        pthread_t *students = malloc(sizeof(pthread_t) * sval);	
	struct assignment *assmnt = (struct assignment *) malloc(sizeof(struct assignment) * qval);
	q.data = assmnt;
	q.size = qval;
	q.count = 0;
	q.tail = 0;
	q.head = 0;
	int *argP = malloc(sizeof(int) * 9);
	argP[0] = aval;
	argP[1] = wval;
	argP[2] = Wval;
	argP[3] = nval;
	argP[4] = Nval;
	argP[5] = hval; 
	argP[6] = Hval;
	argP[7] = dval;
	argP[9] = 0;
	for(int i = 0; i < pval; i++){
		//create pval professors
		if((pthread_create(&professors[i], NULL, prof_thread,(void *) argP)) != 0){
			perror("Professor create");
			exit(1);
		}	
	}
	int argS[2];
	argS[0] = aval;
	argS[1] = 0;
	for(int i = 0; i < sval; i++){
		//create sval students
		if((pthread_create(&students[i], NULL, student_thread, (void *) argS)) != 0){
			perror("Student create");
			exit(1);
		}
	}
	for(int i = 0; i < pval; i++){
		pthread_join(professors[i], NULL);
	}
	for(int i = 0; i < sval; i++){
		pthread_join(students[i], NULL);
	}
}
