#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#include "passi.h"

#include "pass.h"
#include <assert.h>

#include <sys/time.h>

#define DDEBUG 1
#define LOG(fmt, ...) \
        do { if (DDEBUG) fprintf(stderr, "[%s:%d:%s]: " fmt, __FILE__, \
                                __LINE__, __func__, ##__VA_ARGS__); } while (0)


OptParamParser* OptParamParser::__OptParamParser = NULL; 
std::vector<const PassTimer*> PassTimer::expiredTimers;

TEMPLATE_INSTANTIATION(class cl::basic_parser<bool>); 
TEMPLATE_INSTANTIATION(class cl::basic_parser<int>); 
TEMPLATE_INSTANTIATION(class cl::basic_parser<unsigned>); 
TEMPLATE_INSTANTIATION(class cl::basic_parser<unsigned long long>); 
TEMPLATE_INSTANTIATION(class cl::basic_parser<double>); 
TEMPLATE_INSTANTIATION(class cl::basic_parser<float>); 
TEMPLATE_INSTANTIATION(class cl::basic_parser<std::string>); 
TEMPLATE_INSTANTIATION(class cl::basic_parser<char>);

bool di_debug;
std::string di_debug_pass;

BPatch bpatch;

void EXIT(int status) {
    _exit(status);
}

void exit_usage(string err, OptParamParser *parser) {
    cerr << err << endl;
    cerr << parser->usage();
    EXIT(1);
}


int main(int argc, char **argv) {
    int ret;
    bool retB;
    bool modified = false;
    bool outputWritten = false;
    string err;

    LOG("[di-opt] parsing arguments\n");

    OptParamParser *parser = OptParamParser::getInstance(argc, argv);
    ret = parser->parse(err);
    if (ret < 0)
        exit_usage(err, parser);
    
    cl::opt<bool>        __PASS_DEBUG     ("debug",      cl::desc("Enables debugging for all the passes."),  cl::init(false)); 
    cl::opt<std::string> __PASS_DEBUG_PASS("debug-pass", cl::desc("Enables debugging for a specific pass."), cl::init(""));
    cl::opt<bool>        timePasses("time-passes",      cl::desc("Time each pass and print elapsed time."),   cl::init(false)); 
    cl::opt<std::string> timeOutput("time-passes-fname",cl::desc("Write time-passes to this file instead of stdout."), cl::init(""));
    cl::opt<std::string>  mainStart("main-start-fname", cl::desc("Filename that will hold the real start-time of main. First line should hold seconds, second line should hold nanoseconds."), cl::init(""));
    cl::opt<bool>          detach("detach",            cl::desc("Detach immediately."),                      cl::init(true)); 
    cl::opt<bool>            quit("quit",              cl::desc("Quit immediately."),                        cl::init(false)); 
    cl::opt<bool>  forceRewriting("force-rewriting",   cl::desc("Force rewriting even when not necessary."), cl::init(false));

    TimeRegion *dyninstMainTR = new TimeRegion(PassTimer::getPassTimer("di-opt.main", timePasses));
    TimeRegion *untilDetachTR = new TimeRegion(PassTimer::getPassTimer("di-opt.detached", timePasses));
    TimeRegion *initTimeRegion = new TimeRegion(PassTimer::getPassTimer("di-opt.init", timePasses));
    TimeRegion *processFullTR;
    TimeRegion *processRealTR;

    ret = parser->load(err);
    if (ret < 0)
        exit_usage(err, parser);
    ret = parser->check(err, OPPR_IO_OR_ARGS);
    if (ret < 0) 
        exit_usage(err, parser);

    di_debug      = __PASS_DEBUG.getValue();
    di_debug_pass = __PASS_DEBUG_PASS.getValue();
    
    std::string realStartFilename = mainStart.getValue();

    vector<OptParam> passes = parser->getPasses();
    for (unsigned i=0;i<passes.size();i++) {
        ModulePass *pass = dynamic_cast<ModulePass*>(passes[i].owner);
        assert(pass);
        pass->doInitialization();
    }

    BPatch_binaryEdit* beHandle = NULL;
    BPatch_process* pHandle = NULL;
    BPatch_addressSpace* handle;
    std::string path = parser->getInput();
    if (parser->hasIO()) {
        LOG("openBinary\n");
        handle = beHandle = bpatch.openBinary(path.c_str());
    } else {
        LOG("processCreate (this may take some time)\n");
        const char **argv = parser->getArgv();
        handle = pHandle = bpatch.processCreate(argv[0], argv);
        LOG("done\n");
        delete[] argv;
    }
    delete initTimeRegion;

    for (unsigned i=0;i<passes.size();i++) {
        ModulePass *pass = dynamic_cast<ModulePass*>(passes[i].owner);
        assert(pass);
        if (outputWritten){
            errs() << "WARNING: pass " << pass->getName() << " skipped because output already written\n";
            continue;
        }
        LOG("running pass %s\n", pass->getName().c_str());
        TimeRegion timeRegion(PassTimer::getPassTimer(pass->getName(), timePasses));
        retB = pass->runOnModule(handle, path, parser->getOutput(), outputWritten);
        if (retB)
            modified = true;
        LOG("done\n");
    }

    if (quit) {
        if (pHandle)
            pHandle->terminateExecution();
        EXIT(0);
    }
    if (forceRewriting)
        modified = true;
    if (modified && !outputWritten && beHandle)
        beHandle->writeFile(parser->getOutput().c_str());
    else if (pHandle) {
        if (detach) {
            LOG("detaching\n");
            assert(pHandle->isStopped());

            pHandle->detach(true);

            delete untilDetachTR;
            untilDetachTR = NULL;

            LOG("detached\n");
        } else {
            LOG("remaining attached\n");

            processFullTR = new TimeRegion(PassTimer::getPassTimer("process.full", timePasses));
            processRealTR = new TimeRegion(PassTimer::getPassTimer("process.real", timePasses));

            pHandle->continueExecution();
            while (!pHandle->isTerminated()){
                LOG("--start waiting for status change--\n");
                bpatch.waitForStatusChange();
                LOG("--status changed--\n");
            }

            LOG("--proc is terminated--\n");
        }
    }

    if (realStartFilename != "") {
        LOG("reading real start time from %s\n", realStartFilename.c_str());
        std::ifstream infile(realStartFilename);
        if (infile.fail()) 
            errs() << "WARNING: could not open " << realStartFilename << endl;
        else {
            uint64_t start_sec, start_nsec;
            infile >> start_sec;
            infile >> start_nsec;
            LOG("- real start sec: %lu\n", start_sec);
            LOG("- real start nsec: %lu\n", start_nsec);
            processRealTR->adjustStart(start_sec, start_nsec);
        }
    }

    if (processFullTR) delete processFullTR;
    if (processRealTR) delete processRealTR;


    delete dyninstMainTR;
    PassTimer::printExpiredTimers(timeOutput.getValue());

    if(untilDetachTR) delete untilDetachTR;
    
    EXIT(0);
    return 0;
}

