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
#define MAX_P 100

// Uncomment the following line to turn on debugging messages
// #define DEBUG
#define Timer

// long number[MAX_COUNT];
int N; // Number of integers to sort
int P;

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

int compare_and_swap(int i, int j,long * number) 
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
int bubble(int start, int n, int pass,long * number) 
{
#ifdef DEBUG
	fprintf(stderr, "start %d n %d pass %d\n", start, n, pass);
#endif

	int swap_count = 0;
	int next = start;

	assert (start < N-1); // bug if we start at the end of array

	if (pass) { // sort odd-even index pairs
		if ( !(next % 2) ) 
			next = next + 1;
	} else  { // sort even-odd index pairs
		if (next % 2) 
			next = next + 1;
	}

	while ( (next+1) < (start+n) ) {
		swap_count += compare_and_swap(next, next+1,number);
		next+=2;
	}

	return swap_count;
}

void bubble_sort(long * number) 
{
	int last_count, swap_count = N;
	int pass = 0;

#ifdef DEBUG
	print_numbers();
#endif

	do {
		last_count = swap_count;
		swap_count = bubble(0, N, pass,number); // 0 for single-process sorting
#ifdef DEBUG
		print_numbers();
		fprintf(stderr,"last_count %d swap_count %d\n", last_count, swap_count);
#endif
		pass = 1-pass;
	} while((swap_count!=0) || (last_count != 0));
}

long* generate_shm(){
	key_t key;
	int shmid;
	long *data;
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

/* attach to the segment to get a pointer to it: */
	data = (long *)shmat(shmid, (void *)0, 0);
	if (data == (long *)(-1)) {
		perror("shmat");
		exit(1);
	}

	return data;
}

void DoWorkInChild( int index,long * number ){
	//set up parameters
	int n = (int)ceil(N/P);
	int start = index * (n-1);
	if( index == P - 1 && start + n < N){
		n += 1;
	}

	//exec sort
	int last_count, swap_count = N;
	int pass = 0;

#ifdef DEBUG
	print_numbers();
#endif

	do {
		last_count = swap_count;
		swap_count = bubble(0, N, pass,number); // 0 for single-process sorting
#ifdef DEBUG
		print_numbers();
		fprintf(stderr,"last_count %d swap_count %d\n", last_count, swap_count);
#endif
		pass = 1-pass;
	} while((swap_count!=0) || (last_count != 0));

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

	//fprintf(stderr, "Generating.\n");
	long* number = generate_shm();

	generate_numbers(number);

	// fprintf(stdout, "Generated sequence is as follows:\n");
	// print_numbers(number);

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
			DoWorkInChild(i,number);
			exit(0);
		}
	}

	// Wait for children to exit.
	int status;
	pid_t pid;
	while (n > 0) {
	pid = wait(&status);
	// printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
	n--;
	}
	// Parent Work
	// if(getpid() > 0){
	// 	DoWorkInParent(number);
	// }

	//fprintf(stderr, "Sorting.\n");
	// bubble_sort(number);

	fprintf(stdout, "Check Sort:\n");
	if(CheckSort(number) == 0){
		fprintf(stdout, "Sort Failed\n");
	}
	else{
		fprintf(stdout, "Sort Succeeded\n");
	}

	// fprintf(stdout, "Sorted sequence is as follows:\n");
	// print_numbers(number);
	// fprintf(stderr, "Done.\n");

	

	#ifdef Timer
	gettimeofday(&end,NULL);
	int totalTime = end.tv_sec - start.tv_sec;
	fprintf(stderr, "Time Cost: %d Seconds.\n",totalTime);
	#endif

	return 0;
}

