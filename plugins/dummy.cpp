
#include "pass.h"

static cl::opt<std::string>
clOpt1("dummy-opt1",
    cl::desc("A test command-line option."),
    cl::init(""));

static cl::opt<int>
clOpt2("dummy-opt2",
    cl::desc("A test command-line option."),
    cl::init(0));

static cl::opt<bool>
clOpt3("dummy-opt3",
    cl::desc("A test command-line option."),
    cl::init(false));

static cl::opt<double>
clOpt4("dummy-opt4",
    cl::desc("A test command-line option."),
    cl::init(3.4));


namespace {

    class DummyPass: public ModulePass {

        public: 
            static char ID;
            DummyPass(): ModulePass(ID) { }

            virtual bool runOnModule(void *M) {
				//BPatch_addressSpace *as = (BPatch_addressSpace*) M;

				cerr << "clOpt1: " << clOpt1.getValue() << endl;
				cerr << "clOpt2: " << clOpt2.getValue() << endl;
				cerr << "clOpt3: " << clOpt3.getValue() << endl;
				cerr << "clOpt4: " << clOpt4.getValue() << endl;

				return false;
            }
    };
}


char DummyPass::ID = 0;
RegisterPass<DummyPass> MP("dummy", "Dummy Pass");

