/****************************************************************************************[Solver.h]
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

#ifndef Minisat_Solver_h
#define Minisat_Solver_h

#include "penelope/utils/Vec.h"
#include "penelope/utils/Heap.h"
#include "penelope/utils/Alg.h"
#include "penelope/utils/Options.h"
#include "penelope/utils/INIParser.h"
#include "SolverTypes.h"
#include "BoundedQueue.h"

#define EXCHANGE_LBD 0
#define EXCHANGE_LEGACY 1
#define EXCHANGE_UNLIMITED 2

#define IMPORT_FREEZE 0
#define IMPORT_NO_FREEZE 1
#define IMPORT_FREEZE_ALL 2

/** The limit in number of conflict before the first call to reduceDB */
#define INIT_LIMIT 500



namespace penelope {

    //=================================================================================================
    // Solver -- the main class:

    class Cooperation;

    enum FirstPhaseInit { allTrue, allFalse, randomize};

    class Solver {
    public:


        /** If true, the different solvers must stops */
        bool asynch_interrupt;

        /**
         * Create a new Solver
         */
        Solver();
        /**
         * Destructor
         */
        virtual ~Solver();

        /**
         * Initialize the solver
         * @param coop the cooperator object
         * @param t the number of the thread
         */
        void initialize(Cooperation* coop, int t, const INIParser& parser);

        // Problem specification:
        //

        /**
         * Initialize the memory for a given number of variable and clauses
         * @param nbVar the number of variable
         * @param nbClauses the initial number of clauses
         */
        void initialiseMem(int nbVar, int nbClauses);

        /**
         * Add a new variable with parameters specifying variable mode.
         * @param polarity the polarity of the new variable
         * @param dvar if true, the variable is a decision variable
         * @return the created variable
         */
        Var newVar(bool polarity = true, bool dvar = true);

        /**
         * Add a clause to the solver.
         * @param ps the vector of literals representing the clause
         * @return
         */
        bool addClause(const vec<Lit>& ps);
        
        /**
         * Add the empty clause, making the solver contradictory.
         */
        bool addEmptyClause();

        /**
         * Add a unit clause to the solver.
         */
        bool addClause(Lit p);

        /**
         * Add a binary clause to the solver.
         */
        bool addClause(Lit p, Lit q);

        /**
         * Add a ternary clause to the solver.
         */
        bool addClause(Lit p, Lit q, Lit r);

        /**
         * Add a clause to the solver without making superflous internal copy.
         * Will change the passed vector 'ps'.
         * @param ps the vector containing the literals to add as a clause
         */
        bool addClause_(vec<Lit>& ps);
        
        /**
         * Removes already satisfied clauses.
         */
        bool simplify(Cooperation* coop);

        /**
         * Search for a model that respects a given set of assumptions.
         */
        bool solve(const vec<Lit>& assumps, Cooperation* coop);

        /**
         * Search for a model that respects a given set of assumptions (With
         * resource constraints).
         */
        lbool solveLimited(const vec<Lit>& assumps, Cooperation* coop);

        /**
         * Search without assumptions.
         */
        bool solve(Cooperation* coop);

        /**
         * Search for a model that respects a single assumption.
         */
        bool solve(Lit p, Cooperation* coop);

        /**
         * Search for a model that respects two assumptions.
         */
        bool solve(Lit p, Lit q, Cooperation* coop);

        /**
         * Search for a model that respects three assumptions.
         */
        bool solve(Lit p, Lit q, Lit r, Cooperation* coop);

        /**
         * FALSE means solver is in a conflicting state
         */
        bool okay() const;


        /**
         * Write CNF to file in DIMACS-format.
         */
        void toDimacs(FILE* f, const vec<Lit>& assumps);

        /**
         * Write CNF to file in DIMACS-format.
         */
        void toDimacs(const char *file, const vec<Lit>& assumps);

        /**
         * Write CNF to file in DIMACS-format.
         */
        void toDimacs(FILE* f, Clause& c, vec<Var>& map, Var& max);


        // Variable mode:
        //
        /**
         * Declare if a variable should be eligible for selection in the
         * decision heuristic.
         */
        void setDecisionVar(Var v, bool b);

        // Read state:
        //
        /** The current value of a variable. */
        lbool value(Var x) const;
        /** The current value of a literal. */
        lbool value(Lit p) const;
        /**
         * The value of a variable in the last model. The last call to solve
         * must have been satisfiable.
         */
        lbool modelValue(Var x) const;
        /**
         * The value of a literal in the last model. The last call to solve must
         * have been satisfiable.
         */
        lbool modelValue(Lit p) const;

        /** The current number of assigned literals. */

        int nAssigns() const;

        /** The current number of original clauses. */
        int nClauses() const;

        /** The current number of learnt clauses. */
        int nLearnts() const;
        
        /**
         * Fill a given vector with the proven literals
         * @param provenLits the vector that will contain the proven litterals
         */
        void getProvenLiterals(vec<Lit>& provenLits) const;
        
        /**
         * Retrieve the n-th learnt clause
         * @return the n-th learnt clause
         */
        CRef getLearntClause(int n) const{
            return learnts[n];
        }

        /** The current number of variables. */
        int nVars() const;

        /** The current number of free variables. */
        int nFreeVars() const;

        // Resource contraints:
        //
        void setPropBudget(int64_t x);
        void budgetOff();

        // Memory managment:
        //
        virtual void garbageCollect();
        void checkGarbage(double gf);
        void checkGarbage();

        // Extra results: (read-only member variable)
        //
        /** If problem is satisfiable, this vector contains the model (if any). */
        vec<lbool> model;
        /**
         * If problem is unsatisfiable (possibly under assumptions), this vector
         * represent the final conflict clause expressed in the assumptions.
         */
        vec<Lit> conflict;

        /**
         * This array of nbThread elements contains the number of clauses that
         * were used according to the thread that generated them
         */
        uint64_t* nbClauseUsed;
        /**
         * This array of nbThread elements contains the number of clauses
         * imported from each thread
         */
        uint64_t* nbClauseImported;

        // Mode of operation:
        //
        int verbosity;
        double var_decay;
        double clause_decay;
        double random_var_freq;
        double random_seed;
        bool luby_restart;
        /** Controls conflict clause minimization (0=none, 1=basic, 2=deep). */
        int ccmin_mode;
        /** Controls the level of phase saving (0=none, 1=limited, 2=full). */
        int phase_saving;
        /** Use random polarities for branching heuristics. */
        bool rnd_pol;
        /** Initialize variable activities with a small random value. */
        bool rnd_init_act;
        /**
         * The fraction of wasted memory allowed before a garbage collection
         * is triggered.
         */
        double garbage_frac;

        /** The initial restart limit. (default 100) */
        int restart_first;
        /**
         * The factor with which the restart limit is multiplied in each
         * restart. (default 1.5)
         */
        double restart_inc;
        /**
         * The intitial limit for learnt clauses is a factor of the original
         * clauses. (default 1 / 3)
         */
        double learntsize_factor;
        /**
         * The limit for learnt clauses is multiplied with this factor each
         * restart. (default 1.1)
         */
        double learntsize_inc;

        int learntsize_adjust_start_confl;
        double learntsize_adjust_inc;
        /**
         * The current limit in number of conflict before the next reduceDB.
         * It will be initialized at INIT_LIMIT, and then it will be incremented
         * by controlReduceIncrement
         */
        int currentLimit;

        /** identifier of the thread */
        int threadId;
        /** first interpretation variables are chosen randomly */
        bool firstInterpretation;
        int deterministic_mode;
        vec<Lit> importedUnits;
        // Statistics: (read-only member variable)
        //
        uint64_t solves, starts, decisions, rnd_decisions, propagations, conflicts;
        uint64_t dec_vars, clauses_literals, learnts_literals, max_literals, tot_literals;

        int curr_restarts;

        /**
         * add Clauses received from others threads
         * @param lits
         * @return
         */
        CRef addExtraClause(vec<Lit>& lits, int lbd);
        /** Enqueue a literal. Assumes value of literal is undefined. */
        void uncheckedEnqueue(Lit p, CRef from = CRef_Undef);
        int tailUnitLit;
        vec<Lit> extraUnits;
        /**
         * Backtrack until a certain level.
         * Revert to the state at given level (keeping all assignment at 'level'
         * but not beyond).
         * @param level
         */
        void cancelUntil(int level);
        /**
         * Analyze conflict and produce a reason clause.
         * Current decision level must be greater than root level. out_learnt is
         * assumed to be cleared.
         * @param confl the reference of the conflict clause
         * @param out_learn will be filled with the reason clause.
         *        out_learnt[0] the asserting literal at level 'out_btlevel'. If
         *        out_learnt.size() > 1 then 'out_learnt[1]' has the greatest
         *        decision level of the rest of literals. There may be others
         *        from the same level though.
         * @param out_btlevel the level to backtrack to
         * @param lbd will be the lbd of the generated clause
         */
        void analyze(CRef confl, vec<Lit>& out_learnt, int& out_btlevel, unsigned int &lbd);

        /**
         * Retrieve for a given variable the level of the variable. If the
         * variable isn't assigned, the result is unknown
         * @param x the variable
         * @return the level at which the variable was assigned.
         */
        int level(Var x) const;

        /**
         * Gives the current decision level.
         * @return
         */
        int decisionLevel() const;

        /**
         * Perform unit propagation. Returns possibly conflicting clause.
         * @return
         */
        CRef propagate(Cooperation* coop = NULL);

        /**
         * Retrieve the number of clauses that were exported from this thread
         * @return the number of exported clauses
         */
        uint64_t getNbExportedClauses(){
            return nbExportedClauses;
        }

        /**
         * Retrieve a clause from its reference
         * @param ref the reference of the clause we wish
         * @return the requested clause
         */
        Clause& getClause(CRef ref){
            return ca[ref];
        }
        
        /**
         * Retrieve the pointer to the clause
         * @return 
         */
        Clause* getClausePtr(CRef ref){
            return ca.lea(ref);
        }
        
        
        /**
         * Retrieve the number of clause that were shared by other solver but
         * were directly frozen
         * @return the number of directly frozen shared clauses
         */
        int getNbNotAttachedDirectly() const{
            return nbNotAttachedDirectly;
        }

        /**
         * Retrieve the number of imported clause that were deleted without being
         * used
         */
        int getNbImportedDeletedNotUsed() const{
            return nbImportedDeletedNoUse;
        }

        unsigned int getMaxLBDExchanged() const{
            return maxLBDExchanged;
        }

        int getExportPolicy() const{
            return exportPolicy;
        }

        int getImportPolicy() const{
            return importPolicy;
        }

        void printConfiguration() const;

        int nbClausesNeverAttached;
        int nbClausesNotLearnt;

        /** If set to true, clauses might be rejected at import */
        bool rejectAtImport;

        /**
         * The maximum lbd accepted at import. If the lbd of the imported
         * clause is higher, the clause won't be attached, nor put in the database
         */
        int maxLBDAccepted;

        /** The first phase initialization policy */
        FirstPhaseInit fphase;

        /** The restart factor in the case of avglbd restart */
        double restartFactor;

        /** The length of the historic used for avglbd restart */
        int historicLength;
        
        /** The maximum number of values to use when computing trailAvg */
        int trailAvgSize;
        
        /** The average trail length when we encounter conflicts */
        bqueue<int> trailAvg;
        
        /** The minimum number of conflict before activating the restart delay*/
        unsigned int nbConfBeforeRestartDelay;
        
        /** The factor used to delay restarts (glucose 2.1 restart delay)*/
        double trailAvgFactor;

        /** 
         * If set to true, the first interpretation will use the lexicographical
         * order for the literals for the first propagation
         */
        bool lexicoFirstInterpret;
        
        /** If true, we will use width based restarts */
        bool widthBasedRestart;
        
        /** 
         * If we generate more clauses with a width higher than this value, 
         * a restart will be used (if we use width based restarts)
         */
        unsigned int widthRestartR;
        
        /** 
         * The current limit for clause's width when using width based restarts
         */
        int widthRestartW;
        
        /**
         * The update factor of the limit in width based restarts
         */
        unsigned int widthRestartC;

    protected:

        // Solver state:
        //

        /** If true, an async stop has been requested */
        bool asyncStop;

        /**
         * The maximum number of times in a row a clause can be frozen before
         * being removed
         */
        unsigned int maxFreeze;

        /** The extra freeze for the imported clauses */
        int extraImportedFreeze;

        /**
         * The increment factor between each call to reduceDB.
         * Expressed in number of conflicts
         */
        int controlReduceIncrement;

        /** The current agility of the solver */
        double agility;

        /** The updating factor that will be used for the agility */
        double agilityUpdateFactor;

        /** The number of clauses that were attached */
        int nbActiveClauses;

        /**
         * The number of imported clauses that were removed without even being
         * used at least once
         */
        int nbImportedDeletedNoUse;

        /** The number of conflict left before the next call to reduceDB */
        int controlReduce;
        /** The minimal deviation found in the process */
        double minDeviation;
        /** The number of times we called reduceDB*/
        int nbReduce;
        /**
         * If FALSE, the constraints are already unsatisfiable.
         * No part of the solver state may be used!
         */
        bool ok;
        /** List of problem clauses */
        vec<CRef> clauses;
        /** List of learnt clauses. */
        vec<CRef> learnts;
        /** Amount to bump next clause with. */
        double cla_inc;
        /** A heuristic measurement of the activity of a variable. */
        vec<double> activity;
        /** Amount to bump next variable with. */
        double var_inc;
        /**
         * 'watches[lit]' is a list of constraints watching 'lit' (will go there if
         * literal becomes true).
         */
        OccLists<Lit, Watcher, WatcherDeleted> watches;
        /** The current assignments. */
        vec<lbool> assigns;
        /** The preferred polarity of each variable. */
        vec<char> polarity;
        /**
         * The initial preferred polarity of each variable for the current run.
         */
        vec<char> savePolarity;

        /**
         * The initial preferred polarity of each variable for the current run.
         */
        vec<char> viewVariable;

        /**
         * The hamming distance between the current polarity and the saved
         * polarity
         */
        int hammingDistance;

        /** The last computed deviation */
        double lastDeviation;

        /** The number of clauses that weren't attached directly when shared */
        int nbNotAttachedDirectly;

        /**
         * Declares if a variable is eligible for selection in the decision
         * heuristic.
         */
        vec<char> decision;
        /**
         * Assignment stack; stores all assignments made in the order they were
         * made.
         */
        vec<Lit> trail;

        /** Separator indices for different decision levels in 'trail'. */
        vec<int> trail_lim;
        /** Stores reason and level for each variable. */
        vec<VarData> vardata;
        /**
         * Head of queue (as index into the trail -- no more explicit propagation
         * queue in MiniSat).
         */
        int qhead;
        /** Number of top-level assignments since last execution of 'simplify()'. */
        int simpDB_assigns;
        /**
         * Remaining number of propagations that must be made before next execution
         * of 'simplify()'.
         */
        int64_t simpDB_props;
        /** Current set of assumptions provided to solve by the user. */
        vec<Lit> assumptions;
        /**
         * A priority queue of variables ordered with respect to the variable
         * activity.
         */
        Heap<VarOrderLt> order_heap;
        /** Set by 'search()'. */
        double progress_estimate;
        /**
         * Indicates whether possibly inefficient linear scan for satisfied clauses
         * should be performed in 'simplify'.
         */
        bool remove_satisfied;

        /** The clause allocator for this thread */
        ClauseAllocator ca;

        /** Set of last decision level in conflict clauses */
        bqueue<unsigned int> lbdLocalAvg;
        float sumLBD;



        // Temporaries (to reduce allocation overhead). Each variable is prefixed by the method in which it is
        // used, exept 'seen' wich is used in several places.
        //
        vec<char> seen;
        /** TODO: (gilles) doc */
        vec<int> permDiff;
        /** 
         * counter used when computing the lbd. This value needs to be. It allows
         * to determine which literal were already taken into account for the
         * computation
         */
        int lbdHelperCounter;
        vec<Lit> analyze_stack;
        vec<Lit> analyze_toclear;
        vec<Lit> add_tmp;

        double max_learnts;
        int use_learnts;
        double learntsize_adjust_confl;
        int learntsize_adjust_cnt;

        // Resource contraints:
        //
        int64_t conflict_budget; // -1 means no budget.
        int64_t propagation_budget; // -1 means no budget.

        /** The number of clauses that were exported in this thread */
        int64_t nbExportedClauses;

        // The different options for the solver
        bool usePsm;

        int initLimit;

        unsigned int maxLBDExchanged;

        unsigned int maxLBD;

        bool restartAvgLBD;
        
        /** If true, we will use the restart mechanism like the one of picosat*/
        bool picoRestart;
        
        /** The initial base for restarts */
         int picobase;
        
        /** The update factor for the base */
        double picoBaseUpdate;
        
        /** The initial limit for restarts */
        int picolimit;
        
        /** The update factor for the limit */
        double picoLimitUpdate;

        int exportPolicy;

        int importPolicy;

        // Main internal methods:
        //

        const std::string& getValue(const std::string& solv, const std::string& attribute, const INIParser& parser) const;

        /**
         * at level 0, unit extra clauses stored are propagated
         */
        void propagateExtraUnits();

        /**
         * In detreministic case, the two barriers guaranty that during import
         * process no other thread can return to search. Otherwise, each found a
         * solution go out.
         * @param coop
         * @param learnt_clause
         */
        lbool importClauses(Cooperation* coop);

        /**
         * At level 0, units literals propaged are exported to others threads
         * @param coop
         * @param learnt_clause
         * @param id
         * @param thread
         */
        void exportClause(Cooperation* coop, vec<Lit>& learnt_clause, int lbd);

        void exportClause(Cooperation* coop, Clause& generatedClause);

        /**
         * when det=2, each thread try to estimate the number of conflicts under
         * which it must to join the barrier.
         * This estimation based on the calculus of the number of learnts clauses of
         * all learnts and assume that greater the learnts base slower is the unit
         * propagation, which stay a not bad estimation.
         * @param coop
         * @return
         */
        int updateFrequency(Cooperation* coop);

        /**
         * Insert a variable in the decision order priority queue.
         * @param x
         */
        void insertVarOrder(Var x);

        /**
         * Return the next decision variable.
         * @return
         */
        Lit pickBranchLit();

        /**
         * Begins a new decision level.
         */
        void newDecisionLevel();

        /**
         * Test if fact 'p' contradicts current state, enqueue otherwise.
         * @param p
         * @param from
         * @return
         */
        bool enqueue(Lit p, CRef from = CRef_Undef);

        /**
         * COULD THIS BE IMPLEMENTED BY THE ORDINARIY "analyze" BY SOME REASONABLE GENERALIZATION?
         * @param p
         * @param out_conflict
         */
        void analyzeFinal(Lit p, vec<Lit>& out_conflict);

        /**
         * (helper method for 'analyze()')
         * @param p
         * @param abstract_levels
         * @return
         */
        bool litRedundant(Lit p, uint32_t abstract_levels);

        /**
         * Search for a given number of conflicts.
         * @param nof_conflicts
         * @param coop
         * @return
         */
        lbool search(int nof_conflicts, Cooperation* coop);

        /**
         * Main solve method (assumptions given in 'assumptions').
         * @param coop
         * @return
         */
        lbool solve_(Cooperation* coop);

        /**
         * Reduce the set of learnt clauses.
         */
        void reduceDB();

        /**
         * Shrink 'cs' to contain only non-satisfied clauses.
         * @param cs
         */
        void removeSatisfied(vec<CRef>& cs);
        void rebuildOrderHeap();

        // Maintaining Variable/Clause activity:
        //

        /**
         * Decay all variables with the specified factor. Implemented by increasing
         * the 'bump' value instead.
         */
        void varDecayActivity();

        /**
         * Increase a variable with the current 'bump' value.
         * @param v
         * @param inc
         */
        void varBumpActivity(Var v, double inc);

        /**
         * Increase a variable with the current 'bump' value.
         * @param v
         */
        void varBumpActivity(Var v);

        /**
         * Decay all clauses with the specified factor. Implemented by increasing
         * the 'bump' value instead.
         */
        void claDecayActivity();

        /**
         * Increase a clause with the current 'bump' value.
         * @param c
         */
        void claBumpActivity(Clause& c);

        // Operations on clauses:
        //

        /**
         * Attach a clause to watcher lists.
         * @param cr
         */
        void attachClause(CRef cr);

        /**
         * Detach a clause to watcher lists.
         * @param cr
         * @param strict
         */
        void detachClause(CRef cr, bool strict = false);

        /**
         * Detach and free a clause.
         * @param cr
         */
        void removeClause(CRef cr);

        /**
         * Returns TRUE if a clause is a reason for some implication in the current
         * state.
         * @param c
         * @return
         */
        bool locked(const Clause& c) const;

        /**
         * Returns TRUE if a clause is satisfied in the current state.
         * @param c
         * @return
         */
        bool satisfied(const Clause& c) const;

        void relocAll(ClauseAllocator& to);

        // Misc:
        //

        /**
         * Used to represent an abstraction of sets of decision levels.
         * @param x
         * @return
         */
        uint32_t abstractLevel(Var x) const;

        CRef reason(Var x) const;

        /**
         * DELETE THIS ?? IT'S NOT VERY USEFUL ...
         */
        double progressEstimate() const;
        bool withinBudget() const;

        // Static helpers:
        //

        // Returns a random float 0 <= x < 1. Seed must never be 0.

        static inline double drand(double& seed, int threadId) {
            seed /= (1 + threadId);
            seed *= 1389796;
            int q = (int) (seed / 2147483647);
            seed -= (double) q * 2147483647;
            return seed / 2147483647;
        }

        // Returns a random integer 0 <= x < size. Seed must never be 0.

        static inline int irand(double& seed, int size, int threadId) {
            return (int) (drand(seed, threadId) * size);
        }
    };


    //=================================================================================================
    // Implementation of inline methods:

    inline CRef Solver::reason(Var x) const {
        return vardata[x].reason;
    }

    inline int Solver::level(Var x) const {
        return vardata[x].level;
    }

    inline void Solver::insertVarOrder(Var x) {
        if (!order_heap.inHeap(x) && decision[x]) order_heap.insert(x);
    }

    inline void Solver::varDecayActivity() {
        var_inc *= (1 / var_decay);
    }

    inline void Solver::varBumpActivity(Var v) {
        varBumpActivity(v, var_inc);
    }

    inline void Solver::varBumpActivity(Var v, double inc) {
        if ((activity[v] += inc) > 1e100) {
            // Rescale:
            for (int i = 0; i < nVars(); i++)
                activity[i] *= 1e-100;
            var_inc *= 1e-100;
        }

        // Update order_heap with respect to new activity:
        if (order_heap.inHeap(v))
            order_heap.decrease(v);
    }

    inline void Solver::claDecayActivity() {
        cla_inc *= (1 / clause_decay);
    }

    inline void Solver::claBumpActivity(Clause& c) {
        if ((c.activity() += cla_inc) > 1e20) {
            // Rescale:
            for (int i = 0; i < learnts.size(); i++)
                ca[learnts[i]].activity() *= 1e-20;
            cla_inc *= 1e-20;
        }
    }

    inline void Solver::checkGarbage(void) {
        return checkGarbage(garbage_frac);
    }

    inline void Solver::checkGarbage(double gf) {
        if (ca.wasted() > ca.size() * gf)
            garbageCollect();
    }

    // NOTE: enqueue does not set the ok flag! (only public methods do)

    inline bool Solver::enqueue(Lit p, CRef from) {
        return value(p) != l_Undef ? value(p) != l_False : (uncheckedEnqueue(p, from), true);
    }

    inline bool Solver::addClause(const vec<Lit>& ps) {
        ps.copyTo(add_tmp);
        return addClause_(add_tmp);
    }
    

    inline bool Solver::addEmptyClause() {
        add_tmp.clear();
        return addClause_(add_tmp);
    }

    inline bool Solver::addClause(Lit p) {
        add_tmp.clear();
        add_tmp.push(p);
        return addClause_(add_tmp);
    }

    inline bool Solver::addClause(Lit p, Lit q) {
        add_tmp.clear();
        add_tmp.push(p);
        add_tmp.push(q);
        return addClause_(add_tmp);
    }

    inline bool Solver::addClause(Lit p, Lit q, Lit r) {
        add_tmp.clear();
        add_tmp.push(p);
        add_tmp.push(q);
        add_tmp.push(r);
        return addClause_(add_tmp);
    }

    inline bool Solver::locked(const Clause& c) const {
        return value(c[0]) == l_True && reason(var(c[0])) != CRef_Undef && ca.lea(reason(var(c[0]))) == &c;
    }

    inline void Solver::newDecisionLevel() {
        trail_lim.push(trail.size());
    }

    inline int Solver::decisionLevel() const {
        return trail_lim.size();
    }

    inline uint32_t Solver::abstractLevel(Var x) const {
        return 1 << (level(x) & 31);
    }

    inline lbool Solver::value(Var x) const {
        return assigns[x];
    }

    inline lbool Solver::value(Lit p) const {
        return assigns[var(p)] ^ sign(p);
    }

    inline lbool Solver::modelValue(Var x) const {
        return model[x];
    }

    inline lbool Solver::modelValue(Lit p) const {
        return model[var(p)] ^ sign(p);
    }

    inline int Solver::nAssigns() const {
        return trail.size();
    }

    inline int Solver::nClauses() const {
        return clauses.size();
    }

    inline int Solver::nLearnts() const {
        return learnts.size();
    }

    inline int Solver::nVars() const {
        return vardata.size();
    }

    inline int Solver::nFreeVars() const {
        return (int) dec_vars - (trail_lim.size() == 0 ? trail.size() : trail_lim[0]);
    }

    inline void Solver::setDecisionVar(Var v, bool b) {
        if (b && !decision[v]) dec_vars++;
        else if (!b && decision[v]) dec_vars--;

        decision[v] = b;
        insertVarOrder(v);
    }

    inline void Solver::budgetOff() {
        conflict_budget = propagation_budget = -1;
    }

    inline bool Solver::withinBudget() const {
        return (conflict_budget < 0 || conflicts < (uint64_t) conflict_budget) &&
                (propagation_budget < 0 || propagations < (uint64_t) propagation_budget);
    }

    // FIXME TODO: after the introduction of asynchronous interrruptions the solve-versions that return a
    // pure bool do not give a safe interface. Either interrupts must be possible to turn off here, or
    // all calls to solve must return an 'lbool'. I'm not yet sure which I prefer.

    inline bool Solver::solve(Cooperation* coop) {
        budgetOff();
        assumptions.clear();
        return solve_(coop) == l_True;
    }

    inline bool Solver::solve(Lit p, Cooperation* coop) {
        budgetOff();
        assumptions.clear();
        assumptions.push(p);
        return solve_(coop) == l_True;
    }

    inline bool Solver::solve(Lit p, Lit q, Cooperation* coop) {
        budgetOff();
        assumptions.clear();
        assumptions.push(p);
        assumptions.push(q);
        return solve_(coop) == l_True;
    }

    inline bool Solver::solve(Lit p, Lit q, Lit r, Cooperation* coop) {
        budgetOff();
        assumptions.clear();
        assumptions.push(p);
        assumptions.push(q);
        assumptions.push(r);
        return solve_(coop) == l_True;
    }

    inline bool Solver::solve(const vec<Lit>& assumps, Cooperation* coop) {
        budgetOff();
        assumps.copyTo(assumptions);
        return solve_(coop) == l_True;
    }

    inline lbool Solver::solveLimited(const vec<Lit>& assumps, Cooperation* coop) {
        assumps.copyTo(assumptions);
        return solve_(coop);
    }

    inline bool Solver::okay() const {
        return ok;
    }

}

#endif
