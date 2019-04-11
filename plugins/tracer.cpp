#include "Utils.h"


namespace {

class TracerPass: public ModulePass {
        
	private:
	bool skipObject(BPatch_object *O) {
		// called by foreach_func()
		string Oname = O->name();
		if (
			   Oname == "libdyninstAPI_RT.so.9.3.2"
			|| Oname == "libdl-2.23.so"
			|| Oname == "libgcc_s.so.1"
			|| Oname == "libm-2.23.so"
			|| Oname == "libpthread-2.23.so"
			|| Oname == "tracer.so"

			|| Oname == "libstdc++.so.6.0.24"
//			|| Oname == "libc-2.23.so"
		   ) {
			LOG("Skipping object: " << Oname << endl);
			return true;
		}
		LOG("Looking at object: " << Oname << endl);
		return false;
	}

	bool skipModule(BPatch_module *M) {
		// called by foreach_func()
		char buf[256];
		M->getName(buf, 256);
		std::string Mname(buf);

		LOG("Looking at module: " << Mname << endl);
		return false;
	}

	bool skipFunction(BPatch_function *F) {
		// called by foreach_func()
		string Fname = F->getName(); 

		BPatch_module *M = F->getModule();
		char buf[256];
		M->getName(buf, 256);
		std::string Mname(buf);

//  	LOG("Looking at function: " << Fname << endl);
		return false;
	}

    public: 
    static char ID;
    TracerPass(): ModulePass(ID) { }

	virtual bool runOnModule(void *M) {
		passname = std::string("[tracer]");
		BPatch_process *bp = (BPatch_process *) M;
		BPatch_image   *bi = bp->getImage();	

		LOG("Loading shared library: " LIBSRCDIR "/tracer.so\n");
		BPatch_object *tracer_lib = bp->loadLibrary(LIBSRCDIR "/tracer.so");
		if (tracer_lib == NULL) {
			LOG("Could not load shared library");
			exit(EXIT_FAILURE);
		}
       
        /* Expressions to use as argument for our inserted calls to tracer_call() and tracer_return() */
        BPatch_snippet *insn_addr = new BPatch_originalAddressExpr();
        BPatch_snippet *arg0 = new BPatch_paramExpr(0);
        BPatch_snippet *arg1 = new BPatch_paramExpr(1);
        BPatch_snippet *arg2 = new BPatch_paramExpr(2);
        BPatch_snippet *arg3 = new BPatch_paramExpr(3);
        BPatch_snippet *arg4 = new BPatch_paramExpr(4);
        BPatch_snippet *arg5 = new BPatch_paramExpr(5);
        BPatch_snippet *retval = new BPatch_retExpr();
        BPatch_snippet *target = new BPatch_dynamicTargetExpr();

        /* Construct the call to tracer_call */
		BPatch_function *fcall = findFunc(bi,"tracer_call");
		std::vector<BPatch_snippet *> fcall_args;
        fcall_args.push_back(insn_addr);
        fcall_args.push_back(arg0);
        fcall_args.push_back(arg1);
        fcall_args.push_back(arg2);
        fcall_args.push_back(arg3);
        fcall_args.push_back(arg4);
        fcall_args.push_back(arg5);
        fcall_args.push_back(target);
		BPatch_funcCallExpr *fcall_snippet   = new BPatch_funcCallExpr(*fcall, fcall_args);

        /* Construct the clal to tracer_fentry */
        BPatch_function *fentry = findFunc(bi,"tracer_fentry");
        std::vector<BPatch_snippet *> fentry_args;
        fentry_args.push_back(insn_addr);
        fentry_args.push_back(arg0);
        fentry_args.push_back(arg1);
        fentry_args.push_back(arg2);
        fentry_args.push_back(arg3);
        fentry_args.push_back(arg4);
        fentry_args.push_back(arg5);
		BPatch_funcCallExpr *fentry_snippet   = new BPatch_funcCallExpr(*fentry, fentry_args);

        /* Construct the call to tracer_return */
		BPatch_function *freturn = findFunc(bi,"tracer_return");
		std::vector<BPatch_snippet *> freturn_args;
        freturn_args.push_back(insn_addr);
        freturn_args.push_back(retval);
		BPatch_funcCallExpr *freturn_snippet = new BPatch_funcCallExpr(*freturn, freturn_args);


		LOG("Inserting snippets" << endl);
		bp->beginInsertionSet();
		
        LOG("- callsites" << endl);
		foreach_point(bi, BPatch_subroutine, P,
			bp->insertSnippet(*fcall_snippet, *P);
		);
        LOG("- function entries" << endl);
		foreach_point(bi, BPatch_entry, P,
          	LOG(hex << P->getAddress() << ": " << P->getFunction()->getName() << endl);
			bp->insertSnippet(*fentry_snippet, *P);
		);

        LOG("- returns" << endl);
        foreach_point(bi, BPatch_exit, P,
			bp->insertSnippet(*freturn_snippet, *P);
        );

		LOG("Writing changes\n");
		bp->finalizeInsertionSet(true);

		LOG("Done!\n");
		return true;
	}
    };
}


char TracerPass::ID = 0;
RegisterPass<TracerPass> MP("tracer", "Trace calls and returns");

