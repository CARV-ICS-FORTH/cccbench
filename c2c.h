#ifndef _C2C_H_
#define _C2C_H_

#ifndef CACHE_LINE
#define CACHE_LINE 64
#endif
#define spin_barrier_sz() (CACHE_LINE)
#define result_buffer_stride() (CACHE_LINE/sizeof(uint64_t))

#ifdef LOG_CSV_MODE 
#define HIST_SIZE_LOG2 10
#define HIST_SIZE (1<<HIST_SIZE_LOG2)
#endif

typedef struct{
    uint64_t count;
    int64_t sum;
    int64_t min;
    int64_t max;
    int64_t median;
    double mean;
    double var;
    double stdev;
#ifdef LOG_CSV_MODE 
    uint32_t hist[HIST_SIZE];
#endif 
} int64_statistics;

uint64_t *run_test(unsigned int, unsigned int);
void generate_statistics(int64_statistics *, int64_t *, int64_t *, uint64_t , int );
int free_and_set_spin_barriers(uint64_t *);
int set_spin_barriers_no_free(uint64_t *);

#endif //_C2C_H_

