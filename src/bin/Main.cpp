/*****************************************************************************************[Main.cc]
Copyright (c) 2003-2006, Niklas Een, Niklas Sorensson
Copyright (c) 2007-2010, Niklas Sorensson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
**************************************************************************************************/
#include <omp.h>

#include <errno.h>

#include <signal.h>
#include <iosfwd>
#include <fstream>
#ifdef WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <unistd.h>
#include <sys/resource.h>
#endif

#include <sstream>

#include "penelope/utils/System.h"
#include "penelope/utils/ParseUtils.h"
#include "penelope/utils/Options.h"
#include "penelope/core/Dimacs.h"
#include "penelope/core/Solver.h"
#include "penelope/utils/INIParser.h"

#include <iostream>
#include <limits>

using namespace penelope;

//=================================================================================================

//SAT12 hack: we change the number of threads if we found that there is a lot of
//clauses
void changeNbThreads(const char* fileName, int& nbThread){
    char input_line[2048];
    char *result;

    FILE* f = fopen(fileName,"r");
    long int nbC = 0;

    while((result = fgets(input_line, 2048, f )) != NULL ){
        if(result[0]=='p'){
            char* rest = NULL;
            //read the number of literals
            strtol(&(result[5]),&rest,10);
            //read the number of clauses
            nbC = strtol(rest,&rest,10);
            break;
        }
    }
    if(nbC>30000000){
        nbThread = 2;
    }else if (nbC>25000000){
        nbThread = 4;
    }else if (nbC>20000000){
        nbThread = 6;
    }
    fclose(f);
}

void printExecutionStats(){
    double cpu_time = cpuTime();
    double mem_used = memUsedPeak();
    if (mem_used != 0) printf("c   Memory used           : %.2f MB\n", mem_used);
    printf("c   CPU time              : %g s\n", cpu_time);
}

void printStats(Solver& solver)
{
    double cpu_time = cpuTime();
    printf("c   ----------------------------------------         \n");
    printf("c    winner:  more statistics           \n");
    printf("c   ----------------------------------------         \n");
    printf("c   restarts              : %lu\n", solver.starts);
    printf("c   conflicts             : %-12lu   (%.0f /sec)\n", solver.conflicts   , solver.conflicts   /cpu_time);
    printf("c   decisions             : %-12lu   (%4.2f %% random) (%.0f /sec)\n", solver.decisions, (float)solver.rnd_decisions*100 / (float)solver.decisions, solver.decisions   /cpu_time);
    printf("c   propagations          : %-12lu   (%.0f /sec)\n", solver.propagations, solver.propagations/cpu_time);
    printf("c   conflict literals     : %-12lu   (%4.2f %% deleted)\n", solver.tot_literals, (solver.max_literals - solver.tot_literals)*100 / (double)solver.max_literals);
    printf("c   nb exported clause    : %-12lu\n", solver.getNbExportedClauses());
    printExecutionStats();
    
}

void printStats(Cooperation& coop, int winner, const char* instance, std::ostream& output){
    double cpu_time = cpuTime();
    Solver& s = coop.solvers[winner];
    output << "#file: "<< instance <<"\n";


    output << "#date: " << __DATE__ << '\n';
    output << "#cpuTime\trestart\tconflict\tconflict literals\t";

    for(int i=0; i<coop.nbThreads; i++){
        output << "exported " << i<< "\t";
    }
    output << "total exported \n";

    int64_t totalExported = 0;
    output << cpu_time << '\t' << s.starts << '\t' << s.conflicts << '\t';
    output << s.tot_literals << '\t';

    for(int i=0; i<coop.nbThreads; i++){
        int64_t tmp = coop.solvers[i].getNbExportedClauses();
        totalExported += tmp;
        output << tmp << '\t';
    }
    output << totalExported << std::endl;


}


static Solver* solver;
static int nbThreads = 1;
static Cooperation* cooperator = NULL;

#ifdef WIN32
BOOL WINAPI winSigStop(DWORD dwCtrlType){
	switch(dwCtrlType){
	case CTRL_C_EVENT: printf(" catched CTRL_C_EVENT signal\n");
		break;
	case CTRL_BREAK_EVENT: printf(" catched CTRL_BREAK_EVENT signal\n");
		break;
	case CTRL_CLOSE_EVENT : printf(" catched CTRL_CLOSE_EVENT signal\n");
		break;
	case CTRL_LOGOFF_EVENT : printf(" catched CTRL_LOGOFF_EVENT signal\n");
		break;
	case CTRL_SHUTDOWN_EVENT : printf(" catched CTRL_SHUTDOWN_EVENT signal\n");
		break;
	default: printf(" catched unknown signal\n");
		break;
	}
	for(int i=0; i<cooperator->nbThreads; i++){
            cooperator->solvers[i].asynch_interrupt = true;
        }
	return TRUE;
}
#else

// Terminate by notifying the solver and back out gracefully. This is mainly to have a test-case
// for this feature of the Solver as it may take longer than an immediate call to '_exit()'.
static void SIGINT_interrupt(int signum) {
    printf("\n"); printf("c *** INTERRUPTED, signal %d ***\n", signum);
    for(int i=0; i<cooperator->nbThreads; i++){
        cooperator->solvers[i].asynch_interrupt = true;
    }
}
#endif

#include <iostream>


//=================================================================================================
// Main:


int main(int argc, char** argv)
{
    BoolOption   clean_exit("MAIN", "clean-exit", "If set, the exit code will be 0 if everything went well\n", true);
    StringOption configsFile("MAIN", "config", "The configuration file that will be used to configure the solvers","configuration.ini");

    try {
#if defined(_MSC_VER)
		const char* usage = "USAGE: penelope [options] <input-file> <result-output-file>\n\n  where input is in plain DIMACS.\n";
#else
		char usage[2048];
        snprintf(usage,2048,"USAGE: %s [options] <input-file> <result-output-file>\n\n  where input is in plain DIMACS.\n", argv[0]);
#endif
        setUsageHelp(usage);

#if defined(__linux__)
        fpu_control_t oldcw, newcw;
        _FPU_GETCW(oldcw); newcw = (oldcw & ~_FPU_EXTENDED) | _FPU_DOUBLE; _FPU_SETCW(newcw);
        printf("c WARNING: for repeatability, setting FPU to use double precision\n");
#endif
        // Extra options:
        //
        IntOption    verb   ("MAIN", "verb",   "Verbosity level (0=silent, 1=some, 2=more).", 1, IntRange(0, 2));
#ifndef WIN32
        IntOption    cpu_lim("MAIN", "cpu-lim","Limit on CPU time allowed in seconds.\n", std::numeric_limits<int>::max(), IntRange(0, std::numeric_limits<int>::max()));
        IntOption    time_lim("MAIN", "time-lim","Limit on time allowed in seconds.\n", 0, IntRange(0, std::numeric_limits<int>::max()));
        IntOption    mem_lim("MAIN", "mem-lim","Limit on memory usage in megabytes.\n", std::numeric_limits<int>::max(), IntRange(0, std::numeric_limits<int>::max()));
#endif /* WIN32 */

	IntOption    limitEx("MAIN", "limitEx","Limit size clause exchange.\n", 10, IntRange(0, std::numeric_limits<int>::max()));
	IntOption    ctrl   ("MAIN", "ctrl","Dynamic control clause sharing with 2 modes.\n", 0, IntRange(0, 2));
        StringOption statsFile("MAIN", "stats", "The file where we will print the statistics of the winner",NULL);
        BoolOption force_print("MAIN", "force-print", "force to print the solution", false);

        parseOptions(argc, argv, true);

        INIParser parser(std::string((const char*)configsFile));
        parser.parse();

	double initial_time = cpuTime();


        const std::string& ncoresStr(parser.getValueForConf("global","ncores"));
        if(ncoresStr.length()>0){
        if (ncoresStr == std::string("max")) {
                nbThreads = omp_get_num_procs();
            } else {
                nbThreads = atoi(ncoresStr.c_str());
                }
            }

        int determ = 0;
        const std::string& detStr(parser.getValueForConf("global","deterministic"));
        if(detStr.length()>0){
            if (detStr == std::string("true")) {
                determ = 2;
            } else if (detStr == std::string("false")) {
                determ = 0;
            } else{
                std::cerr << "c unknown value for deterministic mode: " << detStr << std::endl;
            }
        }

        changeNbThreads(argv[1],nbThreads);

        omp_set_num_threads(nbThreads);

	int limitExport = limitEx;
	Cooperation coop(nbThreads, limitExport);
        cooperator = &coop;
        solver = coop.solvers;

	coop.ctrl = ctrl;
	coop.deterministic_mode = determ;

#pragma omp parallel
	{
	  int t = omp_get_thread_num();
          coop.solvers[t].initialize(&coop, t, parser);
	  coop.solvers[t].threadId = t;
	  coop.solvers[t].verbosity = verb;
	  coop.solvers[t].deterministic_mode = determ;
	}


	printf("c  -----------------------------------------------------------------------------------------------------------------------\n");
	printf("c |                                 PeneLoPe      %i thread(s) on %i core(s)                                                |\n", coop.nbThreads, omp_get_num_procs());
	printf("c  -----------------------------------------------------------------------------------------------------------------------\n");

        // Use signal handlers that forcibly quit until the solver will be able to respond to
        // interrupts:
#ifdef WIN32
        SetConsoleCtrlHandler(winSigStop,true);
#else
        signal(SIGINT, SIGINT_interrupt);
        signal(SIGXCPU,SIGINT_interrupt);
        signal(SIGALRM, SIGINT_interrupt);

        if(time_lim > 0){
            alarm(time_lim);
        }

        // Set limit on CPU-time:
        if (cpu_lim != std::numeric_limits<int>::max()){
            rlimit rl;
            getrlimit(RLIMIT_CPU, &rl);
            if (rl.rlim_max == RLIM_INFINITY || (rlim_t)cpu_lim < rl.rlim_max){
                rl.rlim_cur = cpu_lim;
                if (setrlimit(RLIMIT_CPU, &rl) == -1)
                    printf("c WARNING! Could not set resource limit: CPU-time.\n");
            } }

        // Set limit on virtual memory:
        if (mem_lim != std::numeric_limits<int>::max()){
            rlim_t new_mem_lim = (rlim_t)mem_lim * 1024*1024;
            rlimit rl;
            getrlimit(RLIMIT_AS, &rl);
            if (rl.rlim_max == RLIM_INFINITY || new_mem_lim < rl.rlim_max){
                rl.rlim_cur = new_mem_lim;
                if (setrlimit(RLIMIT_AS, &rl) == -1)
                    printf("c WARNING! Could not set resource limit: Virtual memory.\n");
            } }

#endif
        if (argc == 1){
            printf("c Can't read from standard input... You must specify the input file \n");
            exit(0);
        }




        if (coop.solvers[0].verbosity > 0){
            printf("c  ===============================================[ Problem Statistics ]==================================================\n");
            printf("c |                                                                                                                       |\n");
            printf("c |  Instance: %-106s |\n", (argc==1 ? "<stdin>" : argv[1]));
        }


        FILE* in = fopen(argv[1], "rb");
        DimacsParser::parse_DIMACS(in, &coop);
        fclose(in);

		
        FILE* res = (!force_print && argc >= 3) ? fopen(argv[2], "wb") : NULL;

        if (coop.solvers[0].verbosity > 0){
	  printf("c |  Number of cores:      %12d                                                                                   |\n", coop.nbThreads);
	  printf("c |  Number of variables:  %12d                                                                                   |\n", coop.solvers[0].nVars());
	  printf("c |  Number of clauses:    %12d                                                                                   |\n", coop.solvers[0].nClauses());

}

        double parsed_time = cpuTime();
        if (coop.solvers[0].verbosity > 0){
            printf("c |  Parse time:           %12.2f s                                                                                 |\n", parsed_time - initial_time);
            printf("c |                                                                                                                       |\n"); }


        // Change to signal-handlers that will only notify the solver and allow it to terminate
        // voluntarily:
#ifndef WIN32
        signal(SIGINT, SIGINT_interrupt);
        signal(SIGXCPU,SIGINT_interrupt);
#endif /* WIN32 */



	if (coop.solvers[0].verbosity > 0){
	  printf("c  ==============================================[ Search Statistics ]====================================================\n");
	  printf("c |  Thread | Conflicts |  Active  | Attached | Detached | Deleted | avg lbd | Progress |\n");
	  printf("c  =======================================================================================================================\n");
    }

        bool clean_exit_set = clean_exit;

	if (!coop.solvers[0].simplify(&coop)){
	  if (res != NULL) fprintf(res, "UNSATISFIABLE\n"), fclose(res);
	  if (coop.solvers[0].verbosity > 0){
	    printf("c ========================================================================================================================\n");
	    printf("c Solved by unit propagation\n");
	    printStats(coop.solvers[0]);
	    printf("c \n"); }
	    if(res == NULL) printf("s UNSATISFIABLE\n");
          if(clean_exit_set){
              exit(0);
          }else{
              exit(20);
          }
        }

        vec<Lit> dummy;



	int winner = 0;
	lbool ret;
	lbool result = l_Undef;

	// launch threads in Parallel
#pragma omp parallel
	{
	  int t = omp_get_thread_num();
          //coop.solvers[t].initialize(&coop, t, parser);
	  coop.start = true;
	  ret = coop.solvers[t].solveLimited(dummy, &coop);
	}

        bool interrupted = false;
        for(int i = 0; i < coop.nbThreads && !interrupted; i++){
            interrupted = coop.solvers[i].asynch_interrupt;
        }
        if(interrupted){
            printf("c INDETERMINATE\n");
            int i = -1;
            coop.printStats(i);
            printExecutionStats();
            return 0;
        }

	// select winner threads with respect to deterministic mode
        bool winnerFound= false;
	for(int t = 0; t < coop.nThreads(); t++){
	  if(!winnerFound && coop.answer(t) != l_Undef){
	    winner = t;
	    result = coop.answer(t);
            winnerFound = true;
	  }
        }

        if(res==NULL || force_print){
            if (result == l_True) {
                printf("s SATISFIABLE\n");
                printf("v ");
                for (int i = 0; i < coop.solvers[winner].nVars(); i++)
                    if (coop.solvers[winner].model[i] != l_Undef)
                        printf("%s%s%d", (i == 0) ? "" : " ", (coop.solvers[winner].model[i] == l_True) ? "" : "-", i + 1);
                printf(" 0\n");
            } else if (result == l_False) {
                printf("s UNSATISFIABLE\n");
            }
        }else if (res != NULL){
            if (result == l_True){
              fprintf(res,"SAT\n");
                for (int i = 0; i < coop.solvers[winner].nVars(); i++)
                    if (coop.solvers[winner].model[i] != l_Undef)
                        fprintf(res, "%s%s%d", (i==0)?"":" ", (coop.solvers[winner].model[i]==l_True)?"":"-", i+1);
                fprintf(res, " 0\n");
            }else if (result == l_False){
                fprintf(res,"UNSAT\n");
            }
            else
                fprintf(res, "c INDET\n");
            fclose(res);
        }

        /*std::cout << "c [global]" << std::endl;
        std::cout << "c ncores = " << coop.nThreads() << std::endl;
        std::cout << "c deterministic = "<< (determ == 0 ? "false" : "true") << std::endl;


        for(int t=0; t<coop.nThreads(); t++){
            coop.solvers[t].printConfiguration();
        }*/


	coop.printStats(coop.solvers[winner].threadId);
	printStats(coop.solvers[winner]);

        const char* statsFileName(statsFile);
        if(statsFileName!=NULL){
            std::ofstream outputStats(statsFileName);
            printStats(coop, winner, (argc>1? argv[1] : "input"), outputStats);
            outputStats.flush();
            outputStats.close();
        }


#ifdef NDEBUG
        exit(result == l_True ? 10 : result == l_False ? 20 : 0);     // (faster than "return", which will invoke the destructor for 'Solver')
#else
        if (clean_exit_set) {
            return 0;
        } else {
            return (result == l_True ? 10 : result == l_False ? 20 : 0);
        }
#endif
    } catch (OutOfMemoryException&){
        printf("===============================================================================\n");
        printf("INDETERMINATE\n");
        exit(0);
    }
}



