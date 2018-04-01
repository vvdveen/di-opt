#ifndef _OPT_PASSI_H_
#define _OPT_PASSI_H_

#include <ostream>
#include <fstream>
#include <iostream>
#include <ctime>
#include "cli.h"
#include "passcli.h"

/* LLVM-like pass interface (freely modified from LLVM 3.3). */

#define errs() cerr
#define dbgs() cout

// Different types of passes.
enum PassKind {
  PT_Module
};

//===---------------------------------------------------------------------------
/// RegisterPass<t> template - This template class is used to notify the system
/// that a Pass is available for use, and registers it into the internal
/// database maintained by the PassManager.  Unless this template is used, opt,
/// for example will not be able to see the pass and attempts to create the pass
/// will fail. This template is used in the follow manner (at global scope, in
/// your .cpp file):
///
/// static RegisterPass<YourPassClassName> tmp("passopt", "My Pass Name");
///

template<typename passName>
struct RegisterPass {
  RegisterPass(const char *PassArg, const char *Name, bool CFGOnly = false,
               bool is_analysis = false)
  {
      passName* pass = new passName();
      pass->setName(PassArg);
      OptParam src(PassArg, "", pass);
      assert(OptParamParser::__OptParamParser);
      OptParamParser::__OptParamParser->registerParam(src, NULL, true);
  }
};

//===----------------------------------------------------------------------===//
/// Pass interface - Implemented by all 'passes'.  Subclass this if you are an
/// interprocedural optimization or you do not fit into any of the more
/// constrained passes described below.
///
class Pass {
  const void *PassID;
  PassKind Kind;
  void operator=(const Pass&);
  Pass(const Pass &);

public:
  explicit Pass(PassKind K, char &pid) : PassID(&pid), Kind(K) { }
  virtual ~Pass() { }

  PassKind getPassKind() const { return Kind; }

  const std::string &getName() const { return name; }
  void setName(std::string str) { name = str; }

protected:
  std::string name;
};


//===----------------------------------------------------------------------===//
/// ModulePass class - This class is used to implement unstructured
/// interprocedural optimizations and analyses.  ModulePasses may do anything
/// they want to the program.
///
class ModulePass : public Pass {
public:
  // doInitialization - Virtual method which can be overriden by subclasses to 
  // do any initialization before a process is created or attached to
  virtual void doInitialization(){};

  /// runOnModule - Virtual method overriden by subclasses to process the module
  /// being operated on.
  virtual bool runOnModule(void *M) = 0;
  virtual bool runOnModule(void *M, std::string path, std::string pathout,
    bool &outputWritten) {
    return runOnModule(M);
  }

  explicit ModulePass(char &pid) : Pass(PT_Module, pid) {}
  virtual ~ModulePass() { }
};

/// Timer - This class is used to track the amount of time spent between
/// invocations of its startTimer()/stopTimer() methods.
///
class Timer {
  std::string Name;      // The name of this time variable.
  struct timespec time_start;
  struct timespec time_end;
  double elapsed;
public:
  explicit Timer(std::string N) { init(N); }

  // Create an uninitialized timer, client must use 'init'.
  explicit Timer() {}
  void init(std::string N) { Name=N; elapsed=0; }

  const std::string &getName() const { return Name; }
  const double getElapsed() const { return elapsed; }

  /// startTimer - Start the timer running.  Time between calls to
  /// startTimer/stopTimer is counted by the Timer class.  Note that these calls
  /// must be correctly paired.
  ///
  virtual void startTimer() {
      clock_gettime(CLOCK_MONOTONIC_RAW, &time_start);
  }

  virtual void adjustStart(uint64_t sec, uint64_t nsec) {
      time_start.tv_sec = sec;
      time_start.tv_nsec = nsec;
  }

  /// stopTimer - Stop the timer.
  ///
  virtual void stopTimer() {
      clock_gettime(CLOCK_MONOTONIC_RAW, &time_end);
      struct timespec time;
      if ((time_end.tv_nsec-time_start.tv_nsec)<0) {
          time.tv_sec = time_end.tv_sec-time_start.tv_sec-1;
          time.tv_nsec = 1000000000+time_end.tv_nsec-time_start.tv_nsec;
      } else {
          time.tv_sec = time_end.tv_sec-time_start.tv_sec;
          time.tv_nsec = time_end.tv_nsec-time_start.tv_nsec;
      };
      elapsed += (double)time.tv_sec + ((double)time.tv_nsec)/1000000000;
  }

  virtual void print(std::ostream *out) const {
      *out << Name << "_pass_secs = " << std::setiosflags(ios::fixed) << std::setprecision(3) << elapsed << std::endl;
  }
};

class PassTimer : public Timer {
public:
  virtual void stopTimer() {
      Timer::stopTimer();
      PassTimer::expiredTimers.push_back(this);
  }
  static PassTimer* getPassTimer(const std::string &name, bool timePasses) {
      return timePasses ? new PassTimer(name) : NULL;
  }
  static void printExpiredTimers(std::string outputFile) {
      streambuf *buf;
      ofstream outf;
      if (outputFile != "") {
          errs() << "OPENING OUTPUTFILE: " << outputFile << endl;
          outf.open(outputFile);
          buf = outf.rdbuf();
      } else
          buf = std::cerr.rdbuf();
      ostream out(buf);

      for (unsigned i=0;i<expiredTimers.size();i++) {
          if (i==0) {
              out << "[time-passes-info]" << std::endl;
          }
          expiredTimers[i]->print(&out);
      }
      if (outputFile != "")
          outf.close();
  }
  static std::vector<const PassTimer*> expiredTimers;

private:
  PassTimer(std::string N) : Timer(N) {}
};

/// The TimeRegion class is used as a helper class to call the startTimer() and
/// stopTimer() methods of the Timer class.  When the object is constructed, it
/// starts the timer specified as its argument.  When it is destroyed, it stops
/// the relevant timer.  This makes it easy to time a region of code.
///
class TimeRegion {
  Timer *T;
public:
  explicit TimeRegion(Timer &t) : T(&t) {
    T->startTimer();
  }
  explicit TimeRegion(Timer *t) : T(t) {
    if (T) T->startTimer();
  }
  void adjustStart(uint64_t sec, uint64_t nsec) {
    if (T) T->adjustStart(sec, nsec);
  } 
  
  ~TimeRegion() {
    if (T) T->stopTimer();
  }
};

extern bool di_debug;
extern std::string di_debug_pass;

#define DEBUG(X) if (di_debug || di_debug_pass == this->name) { X; }


#endif

