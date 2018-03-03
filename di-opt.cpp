#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

#include "passi.h"

#include "pass.h"
#include <assert.h>

#include <sys/time.h>

OPT_CLI_ONCE();
PASSICLI_ONCE();

void EXIT(int status)
{
    _exit(status);
}

void exit_usage(string err, OptParamParser *parser)
{
    cerr << err << endl;
    cerr << parser->usage();
    EXIT(1);
}

/* Standard command-line options. */
#define STD_CL_OPTS() \
    STD_GEN_CL_OPTS(); \
    cl::opt<bool> \
    detach("detach", \
        cl::desc("Detach immediately."), \
        cl::init(true)); \
    cl::opt<bool> \
    quit("quit", \
        cl::desc("Quit immediately."), \
        cl::init(false)); \
    cl::opt<bool> \
    forceRewriting("force-rewriting", \
        cl::desc("Force rewriting even when not necessary."), \
        cl::init(false))

int main(int argc, char **argv)
{
    int ret;
    bool retB;
    bool modified = false;
    bool outputWritten = false;
    string err;

    fprintf(stderr,"[di-opt] parsing arguments\n");

    OptParamParser *parser = OptParamParser::getInstance(argc, argv);
    ret = parser->parse(err);
    if (ret < 0)
        exit_usage(err, parser);
    STD_CL_OPTS();

    TimeRegion *dyninstMainTR = new TimeRegion(PassTimer::getPassTimer("di-opt.main", __CL_TIME_PASSES));
    TimeRegion *untilDetachTR = new TimeRegion(PassTimer::getPassTimer("di-opt.detached", __CL_TIME_PASSES));
    TimeRegion *stayAttachedTR = new TimeRegion(PassTimer::getPassTimer("di-opt.stay_attached", __CL_TIME_PASSES));

    TimeRegion *initTimeRegion = new TimeRegion(PassTimer::getPassTimer("di-opt.init", __CL_TIME_PASSES));
    ret = parser->load(err);
    if (ret < 0)
        exit_usage(err, parser);
    ret = parser->check(err, OPPR_IO_OR_ARGS);
    if (ret < 0) 
        exit_usage(err, parser);

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
        fprintf(stderr,"[di-opt] openBinary\n");
        handle = beHandle = bpatch.openBinary(path.c_str());
    } else {
        fprintf(stderr,"[di-opt] processCreate (this may take some time)\n");
        const char **argv = parser->getArgv();
        handle = pHandle = bpatch.processCreate(argv[0], argv);
        fprintf(stderr,"[di-opt] done\n");
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
        fprintf(stderr,"[di-opt] running pass %s\n", pass->getName().c_str());
        TimeRegion timeRegion(PassTimer::getPassTimer(pass->getName(), __CL_TIME_PASSES));
        retB = pass->runOnModule(handle, path, parser->getOutput(), outputWritten);
        if (retB)
            modified = true;
        fprintf(stderr,"[di-opt] done\n");
    }
    //PassTimer::printExpiredTimers();

    if (quit) {
        if (pHandle)
            pHandle->terminateExecution();
        EXIT(0);
    }
    if (forceRewriting)
        modified = true;
    if (modified && !outputWritten && beHandle) {
        beHandle->writeFile(parser->getOutput().c_str());
    }
    else if (pHandle) {
        if (detach) {
            fprintf(stderr,"[di-opt] detaching\n");
	    assert(pHandle->isStopped());

	    pHandle->detach(true);

            delete untilDetachTR;
            untilDetachTR = NULL;

            fprintf(stderr,"[di-opt] detached\n");
        }
        else {
            fprintf(stderr,"[di-opt] remaining attached\n");

            delete stayAttachedTR;
            stayAttachedTR = NULL;

            pHandle->continueExecution();
            while (!pHandle->isTerminated()){
                fprintf(stderr,"--start waiting for status change--\n");
                bpatch.waitForStatusChange();
                fprintf(stderr,"--status changed--\n");
            }
            fprintf(stderr,"--proc is terminated--\n");
        }
    }

    delete dyninstMainTR;
    PassTimer::printExpiredTimers();

    if(untilDetachTR) delete untilDetachTR;
    if(stayAttachedTR) delete stayAttachedTR;

    EXIT(0);
    return 0;
}

