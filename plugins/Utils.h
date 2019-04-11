#ifndef __DI_UTILS__
#define __DI_UTILS__

#include "pass.h"

using namespace Dyninst;
using namespace InstructionAPI;

static std::string passname;

#define DDEBUG 1

#define LOG(M) \
        do { if (DDEBUG) (errs() << passname << " " << M); } while (0)

#define LOGC(M) \
        do { if (DDEBUG) (errs() << M); } while (0)

/* Helper functions to find a single point/function */
BPatch_point *findPoint(BPatch_image *image, void *addr);
BPatch_function *findFunc(BPatch_image *image, const char *name);
void log_point(BPatch_point *p);
size_t getenvorzero(const char *variable);

#define foreach_func(I, F, B) do { \
    std::vector<BPatch_object *> Os; \
    I->getObjects(Os); \
    for (auto O: Os) { \
        if (skipObject(O)) \
            continue; \
        std::vector<BPatch_module *> Ms; \
        O->modules(Ms); \
        for (auto M: Ms) { \
            if (skipModule(M)) \
                continue; \
            std::vector<BPatch_function *> *Fs = M->getProcedures(); \
            for (auto F: *Fs) { \
                if (skipFunction(F)) \
                    continue; \
                    B \
            } \
        } \
    } \
} while(0)

#define foreach_point(I, T, P, B) do { \
    std::vector<BPatch_object *> Os; \
    I->getObjects(Os); \
    for (auto O: Os) { \
        if (skipObject(O)) \
            continue; \
        std::vector<BPatch_module *> Ms; \
        O->modules(Ms); \
        for (auto M: Ms) { \
            if (skipModule(M)) \
                continue; \
            std::vector<BPatch_function *> *Fs = M->getProcedures(); \
            for (auto F: *Fs) { \
                if (skipFunction(F)) \
                    continue; \
                std::vector<BPatch_point *> *Ps = F->findPoint(T); \
                for (auto P: *Ps) { \
                    B \
                } \
            } \
        } \
    } \
} while(0)


#endif
