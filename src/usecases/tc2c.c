#include "c2c.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{	
    uint64_t total_gettime_cost = 0, max_gettime_cost, min_gettime_cost;
    int64_t total_xy_bias = 0, max_xy_bias, min_xy_bias;
    double average_gettime_cost, average_xy_bias, xy_bias_ratio, xy_bias_stdev, xy_bias_var, crit; 
    int inc, a_random_number, i = 0, x, y;
	uint64_t *shared_buffer;
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
    
    shared_buffer = run_test(x, y);
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
