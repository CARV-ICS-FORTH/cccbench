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
#define MAX_ITER 10000
#define MIN_ITER 3
#define PER_GROUP 5


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
    int i=0, offset, start, end, iterations, inc, cur_x, cur_y;
    unsigned int page_size;
    char *output_path;
    uint64_t *sb, *fixedbuff=(uint64_t *)0;
    char *buff_head;
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

    page_size = getpagesize();
    outfp = fopen(output_path, "w");
    if(NULL == outfp)
    {
        fprintf(stderr, "failed to open file at %s, aborting!\n", output_path);
        exit(-1);
    }
    fixedbuff = mmap((void *)0, CACHE_LINE*iterations*PER_GROUP, PROT_READ|PROT_WRITE,
                 MAP_ANONYMOUS|MAP_PRIVATE,-1, 0);
    if(!fixedbuff || (void *)-1 == fixedbuff)
    {
        fprintf(stderr, "failed to allocate fixed buffer, aborting!\n");
        exit(-1);
    }
    inc = result_buffer_stride();
    fprintf(outfp,"xcore,ycore,iteration,offset,xylat,xylat_max,xylat_std\n");
    offset = 0;
    for(i=0; i<(iterations*PER_GROUP); ++i)
    {
        buff_head = (char *)fixedbuff;
        buff_head += offset;
        if(i && (i%PER_GROUP)==0)
        {
            offset+=CACHE_LINE;
        }
        //sleep(1); //let the cores cool a bit
        set_spin_barriers_no_free((uint64_t *)buff_head);
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
                fprintf(outfp,"%d,%d,%d,%d,%.2lf,%d,%.2lf\n",cur_x,cur_y,i,offset,st.mean,st.max,st.stdev);
                fflush(outfp);
            }
        }
    }
    munmap((void *)fixedbuff, CACHE_LINE*iterations*PER_GROUP);
    fclose(outfp);
    return 0;
}

