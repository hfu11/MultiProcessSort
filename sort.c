/* Even-Odd Sequential Bubble Sort
 * Author: Kartik Gopalan
 * Date: Aug 31 2020
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <math.h>

#define MAX_COUNT 1000000 // Max integers to sort
#define MAX_NUM 100 // Generate numbers between 0 and MAX_NUM
#define MAX_P 10

// Uncomment the following line to turn on printing messages
#define PRINT
// #define Timer

// long number[MAX_COUNT];
int N; // Number of integers to sort
int P;

long* number;
int* pass_exit;
int* doneArr;
int* countArr;


// generate N random numbers between 0 and MAX_NUM
void generate_numbers(long * number)
{
	int i;

	srandom(time(NULL));

	for(i=0; i<N; i++) 
		number[i] = random()%MAX_NUM;
}

void print_numbers(long * number) 
{
	int i;

	for(i=0; i<N; i++) 
		printf("%ld ", number[i]);
	printf("\n");
}

int compare_and_swap(int i, int j) 
{
#ifdef DEBUG
	fprintf(stderr,"i %d j %d\n", i, j);
#endif
	assert ( i<N );
	assert ( j<N );

	if( number[i] > number[j]) {
		long temp = number[i];
		number[i] = number[j];
		number[j] = temp;
		return 1;
	}

	return 0;
}

// even-odd pass bubbling from start to start+n
int bubble(int start, int n, int pass) 
{
#ifdef DEBUG
	fprintf(stderr, "start %d n %d pass %d\n", start, n, pass);
#endif
	
	int swap_count = 0;
	int next = start;

	assert (start < N-1); // bug if we start at the end of array

	if (pass == 1) { // sort odd-even index pairs
		if ( next%2 == 0 ) 
			next = next + 1;
	} else  { // sort even-odd index pairs
		if (next%2 != 0) 
			next = next + 1;
	}

	while ( (next+1) < (start+n) ) {
		swap_count += compare_and_swap(next, next+1);
		next+=2;
	}

	return swap_count;
}

void generate_shm(){
	key_t key;
	int shmid;
	int mode;


	/* make the key: */
	if ((key = ftok("test_shm", 'B')) == -1) {
		perror("ftok");
		exit(1);
	}
	/* connect to t
]=h\e segment. 
	NOTE: There's no IPC_CREATE. What happens if you place IPC_CREATE here? */
	if ((shmid = shmget(key, MAX_COUNT*4 , IPC_CREAT | 0644)) == -1) {
		perror("shmget");
		exit(1);
	}
	number = (long *)shmat(shmid, (void *)0, 0);
	if (number == (long *)(-1)) {
		perror("shmat");
		exit(1);
	}

	if ((key = ftok("test_shm", 'C')) == -1) {
		perror("ftok");
		exit(1);
	}
	if ((shmid = shmget(key, 10*4 , IPC_CREAT | 0644)) == -1) {
		perror("shmget");
		exit(1);
	}
	pass_exit = (int *)shmat(shmid, (void *)0, 0);
	if (pass_exit == (int*)-1) {
		perror("shmat");
		exit(1);
	}

	if ((key = ftok("test_shm", 'D')) == -1) {
		perror("ftok");
		exit(1);
	}
	if ((shmid = shmget(key, MAX_P*4 , IPC_CREAT | 0644)) == -1) {
		perror("shmget");
		exit(1);
	}
	doneArr = (int *)shmat(shmid, (void *)0, 0);
	if (doneArr == (int*)-1) {
		perror("shmat");
		exit(1);
	}

	if ((key = ftok("test_shm", 'E')) == -1) {
		perror("ftok");
		exit(1);
	}
	if ((shmid = shmget(key, MAX_P*4 , IPC_CREAT | 0644)) == -1) {
		perror("shmget");
		exit(1);
	}
	countArr = (int *)shmat(shmid, (void *)0, 0);
	if (countArr == (int*)-1) {
		perror("shmat");
		exit(1);
	}
}



void DoWorkInChild(int index){
	//set up parameters
	int n = N/P+1;
	
	int start = index * (n-1);
	if( index == P - 1){
		n = N - start;
	}

	fprintf(stderr, "start %d n %d\n", start, n);

	// printf("child %d here\n",index);

	//barrier
	while(1){
		if(pass_exit[0] == 1) while(pass_exit[0] == 1);
		countArr[index] = bubble(start, n, 0); // even
		doneArr[index] = 1;
		if(pass_exit[1] == 1) exit(0);

		if(pass_exit[0] == 0) while(pass_exit[0] == 0);
		countArr[index] = bubble(start, n, 1); // odd
		doneArr[index] = 1;
		if(pass_exit[1] == 1) exit(0);
	}


	exit(0);
}

void initDoneArr(){
	int i;
	for(i=0;i<P;i++){
		doneArr[i] = 0;
	}
}

void initCountArr(){
	int i;
	for(i=0;i<P;i++){
		countArr[i] = 0;
	}
}

int checkDone(){
	int i;
	for(i=0;i<P;i++){
		if(doneArr[i] == 0) return 0;
	}
	return 1;
}

int checkCount(){
	int i;
	int count = 0;
	for(i=0;i<P;i++){
		count += countArr[i];
	}
	return count;
}


int CheckSort(long * number){
	int i;
	for(i = 1;i<N;i++){
		if(number[i-1] > number[i]){
			fprintf(stdout, "Sort Failed %ld %ld \n",number[i-1],number[i]);
			return 0;
		}
	}
	return 1;
}

int main(int argc, char *argv[])
{
	#pragma region Data
	if( argc != 3) {
		fprintf(stderr, "Usage: %s N P\n", argv[0]);
		return 1;
	}

	N = strtol(argv[1], (char **)NULL, 10);
	if( (N < 2) || (N > MAX_COUNT) ) {
		fprintf(stderr, "Invalid N. N should be between 2 and %d.\n", MAX_COUNT);
		return 2;
	}

	P = strtol(argv[2], (char **)NULL, 10);
	if( (P < 1) || (P > MAX_P) ) {
		fprintf(stderr, "Invalid P. P should be between 1 and %d.\n", MAX_P);
		return 3;
	}
	// P++;
	#pragma endregion
	#ifdef Timer
	struct timeval start,end;
	gettimeofday(&start,NULL);
	#endif
#pragma region number, pass_exit, doneArr, countArr
	generate_shm();
	pass_exit[0] = 0;
	pass_exit[1] = 0;

	initDoneArr();
	initCountArr();

	//fprintf(stderr, "Generating.\n");

	generate_numbers(number);
#pragma endregion

#ifdef PRINT
	fprintf(stdout, "Generated sequence is as follows:\n");
	print_numbers(number);
#endif

	pid_t pids[P];
	int i;
	int n = P;

	// Create P children
	for (i = 0; i < n; ++i) {
		if ((pids[i] = fork()) < 0) {
			perror("fork");
			abort();
		} 
		else if (pids[i] == 0) {
			DoWorkInChild(i);
			// exit(0);
		}
	}

	// Wait for children to exit.
	// int status;
	// pid_t pid;
	// while (n > 0) {
	// pid = wait(&status);
	// // printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
	// n--;
	// }

	//parent work
	int itr;
	int last_count,count = N;
	
	do{
		last_count = count;
		// printf("\ndoneArr:\n");
		// for(itr=0;itr<P;itr++){
		// 	printf("%d ",doneArr[itr]);
		// }
		// printf("\n");

		while(checkDone() != 1){
			printf("\ndoneArr:\n");
			for(itr=0;itr<P;itr++){
				printf("%d ",doneArr[itr]);
			}
			printf("\n");
		}

		
		count = checkCount();

		// print_numbers(number);

		// for(itr=0;itr<P;itr++){
		// 	printf("%d ",countArr[itr]);
		// }
		// printf("\n");

		pass_exit[0] = 1 - pass_exit[0];
		initDoneArr(); //clear done array
	}while( (count != 0) || (last_count != 0));
	pass_exit[1] = 1;

	// fprintf(stdout, "Check Sort:\n");
	if(CheckSort(number) == 0){
		fprintf(stdout, "Sort Failed\n");
	}
	else{
		fprintf(stdout, "Sort Succeeded\n");
	}
#ifdef PRINT
	fprintf(stdout, "Sorted sequence is as follows:\n");
	print_numbers(number);
	// fprintf(stderr, "Done.\n");
#endif
	

	#ifdef Timer
	gettimeofday(&end,NULL);
	int totalTime = end.tv_sec - start.tv_sec;
	fprintf(stderr, "Time Cost: %d Seconds.\n",totalTime);
	#endif

	return 0;
}

