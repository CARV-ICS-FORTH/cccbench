#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include "c2c.h"

#define CLOCK_TO_USE CLOCK_MONOTONIC_RAW

#ifndef NUM_OF_SAMPLES
#define NUM_OF_SAMPLES 4096
#endif

#ifndef NO_CLOCK
#define timespec_to_ns(ts) ((ts)->tv_nsec + (uint64_t)(ts)->tv_sec * 1000000000)
#endif
#define MAX_FNAME 64
//#define FNAME_PREFIX "samples_from_thread_"
//#define FNAME_POSTFIX ".csv"


uint64_t *shared_buffer;
uint64_t buff_size;
pthread_barrier_t tbarrier;
volatile uint64_t *spin_barriers;

typedef struct{
    uint64_t coordinate;
    uint64_t core_to_pin;
}thread_args;

#ifdef LOG_CSV_MODE 

#define HIST_FLOAT_EXP_BITS 3 
#define HIST_FLOAT_EXP_MASK ((1<<HIST_FLOAT_EXP_BITS)-1)
#define HIST_FLOAT_MANTISSA_BITS (HIST_SIZE_LOG2-1-HIST_FLOAT_EXP_BITS) 
#define HIST_FLOAT_MANTISSA_MASK ((1<<HIST_FLOAT_MANTISSA_BITS)-1)

//First half exact (i.e. 0 to HIST_SIZE/2)
//Other half with log approach up to 64K ( HIST_FLOAT_EXP_BITS bits for exp (with HIST_SIZE_LOG2 bias) and remianing bits for mantissa
//Last value also accounts for overflows
int get_bin_index(int64_t x) {
    if(x<HIST_SIZE/2)
        return (int)x;
    int64_t tmp=x>>(HIST_SIZE_LOG2-1);
    int exp=0;
    while((tmp>>=1) != 0 ) exp++;
    //Is that an overflow?
    if(exp>=(1<<HIST_FLOAT_EXP_BITS))
	return HIST_SIZE-1;
    //Shift to accomodate for implicit one + get MSB fraction part
    int mantissa = (x>>(exp+HIST_SIZE_LOG2-HIST_FLOAT_MANTISSA_BITS - 1 )) & HIST_FLOAT_MANTISSA_MASK;
    return (1<<(HIST_SIZE_LOG2-1)) | (exp <<HIST_FLOAT_MANTISSA_BITS) | mantissa;
}


int64_t get_bin_start(int index) {
    if(index<HIST_SIZE/2)
	return (int64_t)index;
    int64_t exp=(index>>HIST_FLOAT_MANTISSA_BITS)&((1<<HIST_FLOAT_EXP_BITS)-1);
    int64_t mantissa=index & HIST_FLOAT_MANTISSA_MASK;
    return ((1<<HIST_FLOAT_MANTISSA_BITS) | mantissa) <<(exp+HIST_SIZE_LOG2-HIST_FLOAT_MANTISSA_BITS-1);
}

void print_stats(const char* filename, int64_statistics* stats ) {
    FILE* file=fopen(filename,"w");
    //Check if file is empty
    fseek(file,0,SEEK_END);
    long filesize=ftell(file);
    if(filesize == 0 )
        fprintf(file,"count;sum;min;max;median;mean;var;stdev;hist_time_outliers;hist_outliers\n");
    fprintf(file,"%lu;%li;%li;%li;%li;%f;%f;%f;%u\n",stats->count,stats->sum,
		    stats->min,stats->max,stats->median,stats->mean,stats->var,
		    stats->stdev,stats->hist[HIST_SIZE-1]);
    fclose(file);
}

void print_hist(const char* filename, uint32_t hist []  ) {
    FILE* file=fopen(filename,"w");
    //Check if file is empty
    fseek(file,0,SEEK_END);
    long filesize=ftell(file);
    if(filesize == 0 ) {
	for(int i=0;i<HIST_SIZE;i++)
            fprintf(file,"%u;",get_bin_start(i));
        fprintf(file,"\n");
    }
    for(int i=0;i<HIST_SIZE;i++)
       fprintf(file,"%u;",hist[i]);
    fprintf(file,"\n");
    fclose(file);
}

void print_matrix(const char* filename, const char* varname,  uint32_t mat []  ) {
    FILE* file=fopen(filename,"a");
    fprintf(file,"%s=[\n",varname);
    for(int i=0;i<HIST_SIZE;i++){
       if(i==0)
	   fprintf(file,"   [");
       else
	   fprintf(file,"  ,[");
       for(int j=0;j<HIST_SIZE;j++) {
          if(j!=0)
	    fprintf(file,",");
	  int32_t x=mat[i*HIST_SIZE+j];
	  if(x)
            fprintf(file,"%u",x);
       }
       fprintf(file,"]\n");
    }
    fprintf(file,"  ]\n");
    fclose(file);
}

void add_to_hist(uint32_t hist[], int64_t x) {
    hist[get_bin_index(x)]++;
}

void add_to_matrix(uint32_t mat[], int64_t x, int64_t y) {
    mat[get_bin_index(x)*HIST_SIZE+get_bin_index(y)]++;
}

void generate_matrix(uint32_t mat[], int64_t *x_prev, int64_t *x_next, 
		         int64_t *y_prev, int64_t *y_next, 
                         uint64_t count, int stride) 
{
   int64_t i, x, y;
   stride = (stride) ? stride : 1; //if stride is not passed
   memset(mat, 0, HIST_SIZE*HIST_SIZE*sizeof(int32_t));
   for(i=0; i<count; ++i)
   {
       x = x_next[stride*i] - x_prev[stride*i];
       y = y_next[stride*i] - y_prev[stride*i];
       add_to_matrix(mat,x,y);
   }
}

#endif

int cmp_large_ints(const void *int0, const void *int1)
{
    return (*(int64_t *)int0) > (*(int64_t *)int1) ? 1 : -1;
}


void generate_statistics(int64_statistics *stats, int64_t *raw0, int64_t *raw1, 
                         uint64_t count, int stride) 
{
   int64_t i, x, *x_array;
   stride = (stride) ? stride : 1; //if stride is not passed
   memset(stats, 0, sizeof(int64_statistics)); //in IEEE 754 we trust
   stats->count = count;

   x_array = malloc(sizeof(int64_t) * count);
   if(NULL == x_array)
   {
       //problem
       exit(-1);
   }
   for(i=0; i<count; ++i)
   {
       x = raw0[stride*i] - raw1[stride*i];
       x_array[i] = x;
       if(0 == stats->sum)
       {
           stats->min = x;
           stats->max = x;
       }
       stats->sum += x;
       stats->min = (stats->min>x) ? x : stats->min;
       stats->max = (stats->max<x) ? x : stats->max;
       
#ifdef LOG_CSV_MODE 
       add_to_hist(stats->hist,x);
#endif
   }
   stats->mean = stats->sum / ((double) count);
   for(i=0; i<count; ++i)
   {
       x = x_array[i];
       stats->var += (stats->mean - x) * (stats->mean - x);
   }
   stats->var = stats->var / count;
   stats->stdev = sqrt(stats->var);
   qsort(x_array, count, sizeof(int64_t), cmp_large_ints);
   stats->median = x_array[count/2];
   free(x_array);
}


#ifndef NO_CLOCK
static inline uint64_t get_time_in_ns(struct timespec *tspec)
{
    do{
        
    }while(clock_gettime(CLOCK_TO_USE, tspec));
    return timespec_to_ns(tspec);
}
#endif

void sef_set_affinity(uint64_t core_to_pin)
{
    cpu_set_t cset;
    pthread_t selft;
    int res;

    selft = pthread_self();
    CPU_ZERO(&cset);
    CPU_SET(core_to_pin, &cset);

    res = pthread_setaffinity_np(selft, sizeof(cpu_set_t), &cset);
    if (0 != res)
    {
        printf("Fatal Error: failed to set thread affinity core:%lu\n", core_to_pin);
        fflush(stdout);
        exit(-1);
    }
}

int free_and_set_spin_barriers(uint64_t *new_spin_barrier_buffer)
{
    if(NULL != spin_barriers)
    {
        free((void *)spin_barriers);
    }
    spin_barriers = new_spin_barrier_buffer;
    return 0;
}

int set_spin_barriers_no_free(uint64_t *new_spin_barrier_buffer)
{
    spin_barriers = new_spin_barrier_buffer;
    return 0;
}

int clear_spin_barrier_for_next_initialization()
{
    return free_and_set_spin_barriers((uint64_t *)NULL);
}


uint64_t initialize_and_callibrate(int a_random_number)
{
    int ret=0, i=0;
    struct timespec ltspec;
 
    buff_size = CACHE_LINE * (NUM_OF_SAMPLES+1);
    if(shared_buffer)
    {
        //buffer allready initialized, handle

        //since this is not the first call of this function,
        //the barrier must be destroyed before reused
        pthread_barrier_destroy(&tbarrier); 
    }
    else
    {
        ret = posix_memalign((void **)&shared_buffer, getpagesize(), buff_size);    
    }
    if(NULL == spin_barriers)
    {
        ret |= posix_memalign((void **)&spin_barriers, getpagesize(), spin_barrier_sz());
    }
    memset((uint64_t *)spin_barriers, 0, spin_barrier_sz());
    pthread_barrier_init(&tbarrier, NULL, 2);
    if(ret || !shared_buffer || !spin_barriers)
    {
        printf("failed to allocate required memory");
        exit(-ENOMEM);
    }
    //compiler superstition
    if(get_time_in_ns(&ltspec) | a_random_number)
    {
        a_random_number = get_time_in_ns(&ltspec);
    }
    for(i=0; i<(buff_size/sizeof(uint64_t)) ; ++i)
    {
        shared_buffer[i] = a_random_number;
    }
}

void *threadcode(void *data)
{
    uint64_t id, notid, core;
    uint64_t i, j=0, start, stop;
    uint64_t *private_buffer, *thread_buffer_head;
    int ret, inc;
    struct timespec *ttspec;

    id = ((thread_args *)data)->coordinate;
    core = ((thread_args *)data)->core_to_pin;
    sef_set_affinity(core);
    notid = id ^ 0x1LU;
    inc = result_buffer_stride();
    thread_buffer_head = shared_buffer + id*2;
    //compiler superstition once again
    if(0 == thread_buffer_head[0])
    {
        return (void  *)NULL;
    }
    ret = posix_memalign((void **)&private_buffer, getpagesize(), 2*buff_size);
    ret = posix_memalign((void **)&ttspec, getpagesize(), sizeof(struct timespec));
    for(i=0; i<(buff_size/sizeof(uint64_t)) ; ++i)
    {
        private_buffer[i] = id;
    }
    pthread_barrier_wait(&tbarrier);
    for(i=0; i<NUM_OF_SAMPLES ; ++i)
    {
#ifndef NO_CLOCK
        private_buffer[2*i] = get_time_in_ns(ttspec);
#else
        private_buffer[2*i] = i; //write something in the cacheline to be on par with the clocked mode
#endif
        spin_barriers[notid] = i;
        while(i > spin_barriers[id])
            j++;
        private_buffer[2*i+1] = j;
    }
    for(i=0; i<NUM_OF_SAMPLES ; ++i)
    {
        thread_buffer_head[inc*i] = private_buffer[2*i]; //we do this independently of NO_CLOCK for future debugging reasons
        thread_buffer_head[inc*i+1] = private_buffer[2*i+1];
    }
    free(private_buffer);
    free(ttspec);
    return NULL;
}

uint64_t *run_test(unsigned int x, unsigned int y)
{	
    double average_gettime_cost, average_xy_bias, xy_bias_ratio, xy_bias_stdev, xy_bias_var, crit; 
    int inc, a_random_number, i = 0;
    pthread_t thread0, thread1;
    int64_statistics st_time[4];
    int64_statistics st0,st1,st2,st3;

    sef_set_affinity(x);
    thread_args zero = {.coordinate=0, .core_to_pin=x};
    thread_args one = {.coordinate=1, .core_to_pin=y}; 
    //Don't pay too much attention to this random number stuff, it is just
    //compiler optimization avoidance superstition, just let it be
    srand(time(NULL));
    a_random_number = rand()%7; //just to make it more readable
    initialize_and_callibrate(a_random_number + 2);
    pthread_create(&thread0, NULL, threadcode, &zero);
    pthread_create(&thread1, NULL, threadcode, &one);
    pthread_join(thread1, NULL);
    pthread_join(thread0, NULL);
    return shared_buffer;
}

#ifndef LIB_VERSION
int main(int argc, char *argv[])
{	
    uint64_t total_gettime_cost = 0, max_gettime_cost, min_gettime_cost;
    int64_t total_xy_bias = 0, max_xy_bias, min_xy_bias;
    double average_gettime_cost, average_xy_bias, xy_bias_ratio, xy_bias_stdev, xy_bias_var, crit; 
    int inc, a_random_number, i = 0, x, y;
    pthread_t thread0, thread1;
    int64_statistics st_time[4];
    int64_statistics st_counter[2];
    int64_statistics st0,st1,st2,st3;

    if(argc<3)
    {
        printf("nooooo.... nooooo...\n");
        return 0;
    }
    x = atol(argv[1]);
    y = atol(argv[2]);
    
    run_test(x, y);
    inc = result_buffer_stride();
//    for(i=1; i<NUM_OF_SAMPLES; ++i)
//    {
//        uint64_t diff = shared_buffer[i] - shared_buffer[i-1];
//        if(!total_gettime_cost)
//        {
//            max_gettime_cost = diff;
//            min_gettime_cost = diff;
//        }
//        total_gettime_cost += diff;
//        min_gettime_cost = (min_gettime_cost>diff) ? diff : min_gettime_cost;
//        max_gettime_cost = (max_gettime_cost<diff) ? diff : max_gettime_cost;
//    }

#ifdef LOG_CSV_MODE 
    char stats_time_filename[]="stats_time_x_x.csv";
    char hist_time_filename[]= "hist_time_x_x.csv";
    char mat_filename[]=  "mat.py";
    char stats_counter_filename[]=    "stats_counter_x.csv";
    char hist_counter_filename[]=     "hist_counter_x.csv";

    char counter_varname[]=     "counter_x_x";
    char time_varname[]=        "time_x_x";
    char time_counter_varname[]="time_counter_x";

    uint64_t* time_raw[2][2]={
	{shared_buffer+inc,shared_buffer+inc+1},
	{shared_buffer+2*inc,shared_buffer+2*inc+1}
     };

    uint64_t* counter_raw[2][2]={
	{shared_buffer+inc+2,shared_buffer+inc+3},
	{shared_buffer+2*inc+2,shared_buffer+2*inc+3}
     };

    int32_t *mat=(int32_t*)malloc(HIST_SIZE*HIST_SIZE*sizeof(int32_t));
    for(int i=0;i<2;i++) {
    	stats_time_filename[11]='0'+i;
    	hist_time_filename[10]='0'+i;
    	stats_counter_filename[14]='0'+i;
    	hist_counter_filename[13]='0'+i;


        generate_statistics(st_counter+i, counter_raw[1][i], counter_raw[0][i] , NUM_OF_SAMPLES-2, inc);
        print_stats(stats_counter_filename,st_counter+i);
        print_hist(hist_counter_filename,st_counter[i].hist);
	
	counter_varname[8]='0'+i;
	time_varname[5]='0'+i;
	time_counter_varname[13]='0'+i;
        generate_matrix(mat, time_raw[0][i], time_raw[1][i], counter_raw[0][i], counter_raw[1][i], 
                         NUM_OF_SAMPLES-2,inc); 
	print_matrix(mat_filename, time_counter_varname, mat );

	for(int j=0;j<2;j++) {
    	    stats_time_filename[13]='0'+j;
    	    hist_time_filename[12]='0'+j;
            generate_statistics(st_time+i*2+j, time_raw[1][j], time_raw[0][i], NUM_OF_SAMPLES-2, inc);
            print_stats(stats_time_filename,st_time+i*2+j);
            print_hist(hist_time_filename,st_time[i*2+j].hist);
	
	    counter_varname[10]='0'+i;
	    time_varname[7]='0'+i;
            generate_matrix(mat, counter_raw[0][i], counter_raw[1][i], counter_raw[0][j], counter_raw[1][j], 
                         NUM_OF_SAMPLES-2,inc);
	    print_matrix(mat_filename, counter_varname, mat );
            generate_matrix(mat, time_raw[0][i], time_raw[1][i], time_raw[0][j], time_raw[1][j], 
                         NUM_OF_SAMPLES-2,inc);
	    print_matrix(mat_filename, time_varname, mat );
	}
    }
    free(mat);
#endif

    generate_statistics(&st0, shared_buffer+2*inc, shared_buffer+inc, NUM_OF_SAMPLES-2, inc);
    generate_statistics(&st1, shared_buffer+2*inc+2, shared_buffer+inc+2, NUM_OF_SAMPLES-2, inc);
    generate_statistics(&st2, shared_buffer+2*inc+1, shared_buffer+inc+1, NUM_OF_SAMPLES-2, inc);
    generate_statistics(&st3, shared_buffer+2*inc+3, shared_buffer+inc+3, NUM_OF_SAMPLES-2, inc);
    printf("%.2lf,%.2lf,%.2lf,%.2lf\n",st0.mean, st1.mean,st2.mean,st3.mean);

    return 0;
}

#endif //LIB_VERSION


