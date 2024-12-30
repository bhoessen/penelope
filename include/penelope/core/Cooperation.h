/*****************************************************************************************[Cooperation.h]
Copyright (c) 2008-20011, Youssef Hamadi, Saïd Jabbour and Lakhdar Saïs
 
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
 *******************************************************************************************/

#include "SolverTypes.h"
#include "Solver.h"
#include "penelope/utils/Semaphore.h"

#ifndef COOPERATION_H
#define COOPERATION_H

namespace penelope {

    //=================================================================================================
    // Options:

#define MAX_EXTRA_CLAUSES     2000
#define MAX_EXTRA_UNITS       2000

#define MAX_IMPORT_CLAUSES    4000
#define LIMIT_CONFLICTS_EVAL  6000

#define INITIAL_DET_FREQUENCE 700

#define AIMDX  0.25
#define AIMDY  8

    /**
     * This class manage clause sharing component between threads,i.e.,
     * It controls read and write operations in extraUnits and extraClauses
     * arrays iterators headExtra... and tailExtra... are used to to this end
     */
    class Cooperation {
    public:

        /** start and end multi-threads search */
        bool start, end;
        /** number of  threads */
        int nbThreads;
        /** initial limit size of shared clauses */
        int limitExportClauses;
        /** pairwised limit limit size clause sharing */
        double** pairwiseLimitExportClauses;
        /** The maximum lbd value that needs to be exported */
        int maxLBD;

        /** set of running CDCL algorithms */
        Solver* solvers;
        /** answer of threads */
        lbool* answers;

        /** where are stored the shared unit clauses  */
        Lit*** extraUnits;
        int** headExtraUnits;
        int** tailExtraUnits;

        /** where are stored the set of shared clauses with size > 1 */
        Lit**** extraClauses;
        int** headExtraClauses;
        int** tailExtraClauses;
        int*** extraClausesLBD;

        //---------------------------------------

        /** barrier synchronization limit conflicts in deterministic case */
        int initFreq;
        /** in dynamic case, #conflicts barrier synchronization */
        int* deterministic_freq;
        /** counters for the total imported Unit clauses */
        int* nbImportedExtraUnits;
        /** counters for the total imported Extra clauses */
        int* nbImportedExtraClauses;
        uint64_t* learntsz;
        /** activate control clause sharing size mode */
        char ctrl;
        /** aimd control approach {aimdx, aimdy are their parameters} */
        double aimdx, aimdy;
        /** imported clause of for and from each thread */
        int** pairwiseImportedExtraClauses;
        /** running Minisat in deterministic mode */
        bool deterministic_mode;
        
        /**
         * The list of array of literals we created during the process that will
         * be destroyed at the end
         */
        vec<Lit*> garbage;
        
        /** The semaphore guarding the access of the garbage */
        Semaphore garbageGuardian;
        
        //=================================================================================================

        /**
         * Creates a new cooperation object
         * @param nbThreads the number of threads
         * @param nbThreadsl the maximum size of clauses in the waiting list to 
         *          be imported by the different threads
         */
        Cooperation(int nbThreads, int limitImport);

        /**
         * Destructor
         */
        virtual ~Cooperation();
        
        /**
         * Reset the solvers
         */
        void resetSolvers();

        /**
         * manage export Extra Unit Clauses 
         * @param s
         * @param unit
         */
        void exportExtraUnit(Solver* s, Lit unit);

        /**
         * manage import Extra Unit Clauses
         * @param s
         */
        void importExtraUnits(Solver* s);
        /**
         * store unit extra clause in the vector unit_learnts
         * @param s
         * @param lits
         */
        void importExtraUnits(Solver* s, vec<Lit>& lits);

        /**
         * each time a new clause is learnt, it is exported to other threads
         * @param s
         * @param learnt
         */
        void exportExtraClause(Solver* s, vec<Lit>& learnt, int lbd);

        /**
         * each time a new clause is learnt, it is exported to other threads
         * @param s
         * @param c
         */
        void exportExtraClause(Solver* s, Clause& c);

        /**
         * import Extra clauses sent from other threads
         * @param s
         */
        void importExtraClauses(Solver* s);

        /**
         * build a clause from the learnt Extra Lit* 
         * watch it correctly, test basic cases distinguich other cases during 
         * watching process 
         * @param s
         * @param t
         * @param lt
         */
        void addExtraClause(Solver* s, int t, Lit* lt, int lbd);

        /**
         * Enqueue the unit literals at level 0
         * @param s
         * @param t
         * @param l
         */
        void uncheckedEnqueue(Solver* s, int t, Lit l);

        /**
         * Enqueue the unit literals at level 0
         * @param s
         * @param t
         * @param l
         * @param lits
         */
        void storeExtraUnits(Solver* s, int t, Lit l, vec<Lit>& lits);

        //---------------------------------------
        /**
         * update limit size of exported clauses in order to maintain exchange 
         * during search: (MAX_IMPORT_CLAUSES / LIMIT_CONFLICTS) is the percent of 
         * clauses that must be shared
         * @param s
         */
        void updateLimitExportClauses(Solver* s);

        /**
         * print statistics of each thread
         * @param id
         */
        void printStats(int& id);

        /**
         * print the final values of size clauses shared between pairwise threads
         */
        void printExMatrix();

        /**
         * more informations about deterministic or non deterministic mode and final
         * matrix print
         */
        void Parallel_Info();

        //=================================================================================================

        /**
         * Retrieve the number of threads
         * @return the number of threads
         */
        inline int nThreads() {
            return nbThreads;
        }

        /**
         * 
         * @return 
         */
        inline int limitszClauses() {
            return limitExportClauses;
        }

        /**
         * 
         * @param t
         * @return 
         */
        inline lbool answer(int t) {
            return answers[t];
        }

        /**
         * 
         * @param id
         * @param lb
         * @return 
         */
        inline bool setAnswer(int id, lbool lb) {
            answers[id] = lb;
            return true;
        }

    };
}


#endif /* COOPERATION_H */


