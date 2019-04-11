#include "Utils.h"

/* Helper functions to find a single point/function */
BPatch_point *findPoint(BPatch_image *image, void *addr) {
    std::vector<BPatch_point *> points;

    image->findPoints((Address) addr, points);
    if (points.empty()) {
        printf("WARNING! Could not find any point at %p\n", addr);
        return NULL;
    }
    if (points.size() > 1) {
        printf("WARNING! Found more than one point at %p\n", addr);
//      exit(EXIT_FAILURE);
    }
    return points[0];
}
BPatch_function *findFunc(BPatch_image *image, const char *name) {
    std::vector<BPatch_function*> funcs;

    image->findFunction(name, funcs);
    if (funcs.empty()) {
        printf("Could not find any function named %s\n",name);
        exit(EXIT_FAILURE);
    }
    if (funcs.size() > 1) {
        printf("Found more than one function named %s:\n",name);
        for (auto f: funcs) {
            char modulename[1024];
            f->getModuleName(modulename, 1024);
            cout << modulename << "." << hex << f->getBaseAddr() << endl;
        }
        exit(EXIT_FAILURE);
    }
    return funcs[0];
}

void log_point(BPatch_point *p) {
    BPatch_function *f= p->getFunction();
    BPatch_module *m = f->getModule();

    char buf[256];
    m->getName(buf, 256);
    std::string modulename(buf);

    LOG(modulename << "." << f->getName() << "." << std::hex << p->getAddress() << endl);
}

size_t getenvorzero(const char *variable) {
    char *max_env = getenv(variable);
    if (max_env)
      return strtol(max_env, NULL, 10);
    return 0;
}
