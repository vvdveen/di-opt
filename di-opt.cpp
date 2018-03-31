#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#include "passi.h"

#include "pass.h"
#include <assert.h>

#include <sys/time.h>

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

//  fprintf(stderr,"[di-opt] parsing arguments\n");

    OptParamParser *parser = OptParamParser::getInstance(argc, argv);
    ret = parser->parse(err);
    if (ret < 0)
        exit_usage(err, parser);
    
    cl::opt<bool>        __PASS_DEBUG     ("debug",      cl::desc("Enables debugging for all the passes."),  cl::init(false)); 
    cl::opt<std::string> __PASS_DEBUG_PASS("debug-pass", cl::desc("Enables debugging for a specific pass."), cl::init(""));
    cl::opt<bool> __CL_TIME_PASSES("time-passes",    cl::desc("Time each pass and print elapsed time."),   cl::init(false)); 
    cl::opt<bool>          detach("detach",          cl::desc("Detach immediately."),                      cl::init(true)); 
    cl::opt<bool>            quit("quit",            cl::desc("Quit immediately."),                        cl::init(false)); 
    cl::opt<bool>  forceRewriting("force-rewriting", cl::desc("Force rewriting even when not necessary."), cl::init(false));

    TimeRegion *dyninstMainTR = new TimeRegion(PassTimer::getPassTimer("di-opt.main", __CL_TIME_PASSES));
    TimeRegion *untilDetachTR = new TimeRegion(PassTimer::getPassTimer("di-opt.detached", __CL_TIME_PASSES));
    TimeRegion *initTimeRegion = new TimeRegion(PassTimer::getPassTimer("di-opt.init", __CL_TIME_PASSES));
    TimeRegion *processTR;

    ret = parser->load(err);
    if (ret < 0)
        exit_usage(err, parser);
    ret = parser->check(err, OPPR_IO_OR_ARGS);
    if (ret < 0) 
        exit_usage(err, parser);

    di_debug      = __PASS_DEBUG.getValue();
    di_debug_pass = __PASS_DEBUG_PASS.getValue();

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
//      fprintf(stderr,"[di-opt] openBinary\n");
        handle = beHandle = bpatch.openBinary(path.c_str());
    } else {
//      fprintf(stderr,"[di-opt] processCreate (this may take some time)\n");
        const char **argv = parser->getArgv();
        handle = pHandle = bpatch.processCreate(argv[0], argv);
//      fprintf(stderr,"[di-opt] done\n");
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
//      fprintf(stderr,"[di-opt] running pass %s\n", pass->getName().c_str());
        TimeRegion timeRegion(PassTimer::getPassTimer(pass->getName(), __CL_TIME_PASSES));
        retB = pass->runOnModule(handle, path, parser->getOutput(), outputWritten);
        if (retB)
            modified = true;
//      fprintf(stderr,"[di-opt] done\n");
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
//          fprintf(stderr,"[di-opt] detaching\n");
            assert(pHandle->isStopped());

            pHandle->detach(true);

            delete untilDetachTR;
            untilDetachTR = NULL;

//          fprintf(stderr,"[di-opt] detached\n");
        } else {
//          fprintf(stderr,"[di-opt] remaining attached\n");

            processTR = new TimeRegion(PassTimer::getPassTimer("di-opt.process", __CL_TIME_PASSES));

            pHandle->continueExecution();
            while (!pHandle->isTerminated()){
//              fprintf(stderr,"--start waiting for status change--\n");
                bpatch.waitForStatusChange();
//              fprintf(stderr,"--status changed--\n");
            }
            delete processTR;

//          fprintf(stderr,"--proc is terminated--\n");
        }
    }

    delete dyninstMainTR;
    PassTimer::printExpiredTimers();

    if(untilDetachTR) delete untilDetachTR;
    
    EXIT(0);
    return 0;
}

