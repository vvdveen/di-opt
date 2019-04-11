#include <asm/prctl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <ucontext.h>
#include <unistd.h>


#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>

#define PAGE_SIZE 4096
#define PAGE_MASK (~(PAGE_SIZE-1))
#define PAGE_ALIGN(addr) (((addr))&PAGE_MASK)


#define DEBUG 1
/* Fancy logging */
#define LOG(fmt, ...) \
        do { if (DEBUG) fprintf(stderr, "[%s:%d:%s]: " fmt, __FILE__, \
                                __LINE__, __func__, ##__VA_ARGS__); } while (0)

/* Wrapper around mincore() to test if an address is mapped */
static inline int is_mapped(void *addr) {
    unsigned char vec[1];
    if (mincore((void *)PAGE_ALIGN((uint64_t)addr), PAGE_SIZE, vec) == -1)
        return 0;
    return vec[0] & 0x01;
}

void dump_arg(int index, void *arg) {
	if (is_mapped(arg)) 
		LOG("- arg%d: %p --> %p\n", index, arg, (void *) (* (uint64_t *)arg));
    else
        LOG("- arg%d: %p\n", index, arg);
}



void tracer_call(void *insn, 
				 void *arg0, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5, void *target) {
	LOG("call at %p\n", insn);
	dump_arg(0, arg0);
    dump_arg(1, arg1);
    dump_arg(2, arg1);
    dump_arg(3, arg1);
    dump_arg(4, arg1);
    dump_arg(5, arg1);
    LOG("- target: %p\n", target);
}

void tracer_fentry(void *insn, 
				 void *arg0, void *arg1, void *arg2, void *arg3, void *arg4, void *arg5) {
	LOG("function at %p\n", insn);
	dump_arg(0, arg0);
    dump_arg(1, arg1);
    dump_arg(2, arg1);
    dump_arg(3, arg1);
    dump_arg(4, arg1);
    dump_arg(5, arg1);
}

void tracer_return(void *insn, void *retval, void *target) {
	LOG("return at %p\n", insn);
    LOG("- retval: %p\n", retval);
}


static void tracer_init(void) __attribute__ ((constructor));
static void tracer_init(void) {
	LOG("INIT\n");
}

static void tracer_fini(void) __attribute__ ((destructor));
static void tracer_fini(void) {
	LOG("FINI\n");
}
