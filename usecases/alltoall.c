#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include "c2c.h"

#define NUM_OF_SAMPLES 20000
//assuming a system with less than MAX_CORES cores
#define MAX_CORES 8192
#define MAX_ITER 1000
#define MIN_ITER 3


void usage(const char *argv0)
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "%s <core ragen start> <core range end> <iterations> <output path>\n", argv0);
    exit(-1);
}

void parameter_error(const char *err)
{
    fprintf(stderr, "parameter error: %s\n", err);
    exit(-1);
}

int main(int argc, char *argv[])
{
    int i = 0, start, end, iterations, inc, cur_x, cur_y;
    char *output_path;
    uint64_t *sb;
    int64_statistics st;
    FILE *outfp;

    if(argc<5)
    {
        usage(argv[0]);
        return 0;
    }
    start = atol(argv[1]);
    end = atol(argv[2]);
    iterations = atol(argv[3]);
    output_path = argv[4];
    //check input
    if(end > MAX_CORES)
        parameter_error("range end core too high");
    if(end <= start)
        parameter_error("range start must be smaller than range end");
    if(iterations >= MAX_ITER)
        parameter_error("too many iterations");
    if(iterations < MIN_ITER)
        parameter_error("too few iterations");
    if(0 == access(output_path, F_OK))
        parameter_error("output file already exists, will chicken out");

    outfp = fopen(output_path, "w");
    if(NULL == outfp)
    {
        fprintf(stderr, "failed to open file at %s, aborting!\n", output_path);
        exit(-1);
    }
    inc = result_buffer_stride();
    fprintf(outfp,"xcore,ycore,iteration,xylat");
    for(i=0; i<iterations; ++i)
    {
        sleep(1); //let the cores cool a bit
        for(cur_x=start; cur_x<=end; ++cur_x)
        {
            for(cur_y=start; cur_y<=end; ++cur_y)
            {
                if(cur_x == cur_y)
                {
                    continue;
                }
                sb = run_test(cur_x, cur_y);
                generate_statistics(&st, sb+2*inc, sb+inc, NUM_OF_SAMPLES-4, inc);
                clear_spin_barrier_for_next_initialization();
                fprintf(outfp,"%d,%d,%d,%.2lf\n",cur_x,cur_y,i,st.mean);
            }
        }

    }
    fclose(outfp);
    return 0;
}

