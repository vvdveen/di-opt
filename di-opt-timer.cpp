#include <stdio.h>
#include <time.h>

#define DDEBUG 1
#define LOG(fmt, ...) \
        do { if (DDEBUG) fprintf(stderr, "[%s:%d:%s]: " fmt, __FILE__, \
                                __LINE__, __func__, ##__VA_ARGS__); } while (0)

/* set by di-opt - starts when the process first gains control */
struct timespec di_opt_process_start;

/* set by di_opt_timer_main below - starts when the process starts main */
struct timespec di_opt_main_start;

const char *di_opt_timers_output;

double di_opt_timer_delta(struct timespec *start, struct timespec *stop) {
    struct timespec time;
    if ((stop->tv_nsec - start->tv_nsec) < 0) {
        time.tv_sec = stop->tv_sec - start->tv_sec - 1;
        time.tv_nsec = 1000000000 + stop->tv_nsec - start->tv_nsec;
    } else {
        time.tv_sec = stop->tv_sec - start->tv_sec;
        time.tv_nsec = stop->tv_nsec - start->tv_nsec;
    }
    double delta = (double)time.tv_sec + ((double)time.tv_nsec)/1000000000;
    return delta;
}

/* this will be the last hook before main is executed */
void di_opt_timer_main(const char *fname) {
    LOG("Appending results to %s\n", fname);
    di_opt_timers_output = fname;

    LOG("Starting the *MAIN* timer\n");
    clock_gettime(CLOCK_MONOTONIC_RAW, &di_opt_main_start);
}

static void di_opt_timer_fini(void) __attribute__ ((destructor));
static void di_opt_timer_fini(void) {
    struct timespec stop;
    clock_gettime(CLOCK_MONOTONIC_RAW, &stop);
    
    double process_time = di_opt_timer_delta(&di_opt_process_start, &stop);
    double    main_time = di_opt_timer_delta(&di_opt_main_start, &stop);
    LOG("Done.\n");
    LOG("*PROCESS* time: %.3f\n", process_time);
    LOG("   *MAIN* time: %.3f\n",    main_time);

    FILE *fp = stderr;
    if (*di_opt_timers_output != 0) {
        fp = fopen(di_opt_timers_output,"a+");
    	if (!fp) {
            fprintf(stderr,"WARNING! Could not open %s\n",di_opt_timers_output);
            fp = stderr;
        }
    }
    fprintf(fp,"[time-runtime-info]\n");
    fprintf(fp,"process.proc_secs = %.3f\n", process_time);
    fprintf(fp,"process.main_secs = %.3f\n", main_time);
    fprintf(fp,"\n");
    fclose(fp);
}
