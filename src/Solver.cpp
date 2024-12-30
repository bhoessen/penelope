/***************************************************************************************[Solver.cc]
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

#include <math.h>
#include <omp.h>
#include "penelope/utils/Sort.h"
#include "penelope/core/Solver.h"
#include "penelope/core/Cooperation.h"
#include "penelope/core/Determanager.h"
#include <iostream>
#include <sstream>




using namespace penelope;

//=================================================================================================
// Options:


static const char* _cat = "CORE";

static DoubleOption opt_var_decay(_cat, "var-decay", 
        "The variable activity decay factor", 0.95, DoubleRange(0, false, 1, false));
static DoubleOption opt_clause_decay(_cat, "cla-decay", 
        "The clause activity decay factor", 0.999, DoubleRange(0, false, 1, false));
static DoubleOption opt_random_var_freq(_cat, "rnd-freq", 
        "The frequency with which the decision heuristic tries to choose a random variable", 0, DoubleRange(0, true, 1, true));
static DoubleOption opt_random_seed(_cat, "rnd-seed", 
        "Used by the random variable selection", 91648253, DoubleRange(0, false, HUGE_VAL, false));
static IntOption opt_ccmin_mode(_cat, "ccmin-mode", 
        "Controls conflict clause minimization (0=none, 1=basic, 2=deep)", 2, IntRange(0, 2));
static IntOption opt_phase_saving(_cat, "phase-saving", 
        "Controls the level of phase saving (0=none, 1=limited, 2=full)", 2, IntRange(0, 2));
static BoolOption opt_rnd_init_act(_cat, "rnd-init", 
        "Randomize the initial activity", false);
static BoolOption opt_rnd_init_phase(_cat, "rnd-phase", 
        "Randomize the initial phase", false);
static BoolOption opt_luby_restart(_cat, "luby", 
        "Use the Luby restart sequence", true);
static DoubleOption opt_restart_inc(_cat, "rinc", 
        "Restart interval increase factor", 2, DoubleRange(1, false, HUGE_VAL, false));
static DoubleOption opt_garbage_frac(_cat, "gc-frac", 
        "The fraction of wasted memory allowed before a garbage collection is triggered", 0.20, DoubleRange(0, false, HUGE_VAL, false));



//=================================================================================================
// Constructor/Destructor:

Solver::Solver() :

// Parameters (user settable):
//
asynch_interrupt(false)
, model()
, conflict()
, nbClauseUsed(NULL)
, nbClauseImported(NULL)
, verbosity        (0)
, var_decay(opt_var_decay)
, clause_decay(opt_clause_decay)
, random_var_freq(opt_random_var_freq)
, random_seed(opt_random_seed)
, luby_restart(opt_luby_restart)
, ccmin_mode(opt_ccmin_mode)
, phase_saving(opt_phase_saving)
, rnd_pol(false)
, rnd_init_act(opt_rnd_init_act)
, garbage_frac(opt_garbage_frac)
, restart_first(100)
, restart_inc(opt_restart_inc)

// Parameters (the rest):
//
, learntsize_factor((double) 1 / (double) 3), learntsize_inc(1.1)

// Parameters (experimental):
//
, learntsize_adjust_start_confl(100)
, learntsize_adjust_inc(1.5)
, currentLimit(0)
, threadId(-1) //The value will be set later by main
, firstInterpretation(true)
, deterministic_mode(0)
, importedUnits()

// Statistics: (formerly in 'SolverStats')
//
, solves(0), starts(0), decisions(0), rnd_decisions(0), propagations(0)
, conflicts(0), dec_vars(0), clauses_literals(0), learnts_literals(0)
, max_literals(0), tot_literals(0)

, curr_restarts(0)
, tailUnitLit(-1)
, extraUnits()

, nbClausesNeverAttached(0)
, nbClausesNotLearnt(0)
, rejectAtImport(false)
, maxLBDAccepted(10)
, fphase(randomize)
, restartFactor(0.7)
, historicLength(100)
, trailAvgSize(5000)
, trailAvg()
, nbConfBeforeRestartDelay(1000)
, trailAvgFactor(1.4)
, lexicoFirstInterpret(true)
, widthBasedRestart(false)
, widthRestartR(5)      
, widthRestartW(10)
, widthRestartC(1)

, asyncStop(false)
, maxFreeze(7)
, extraImportedFreeze(7)
, controlReduceIncrement(100)
, agility(0.0)
, agilityUpdateFactor(0.9999)
, nbActiveClauses(0)
, nbImportedDeletedNoUse(0)
, controlReduce(INIT_LIMIT)
, minDeviation(1)
, nbReduce(0)
, ok(true)
, clauses()
, learnts()
, cla_inc(1)
, activity()
, var_inc(1)
, watches(WatcherDeleted(ca))
, assigns()
, polarity()
, savePolarity()
, viewVariable()
, hammingDistance(-1)
, lastDeviation(0.1)
, nbNotAttachedDirectly(0)
, decision()
, trail()
, trail_lim()
, vardata()
, qhead(0)
, simpDB_assigns(-1)
, simpDB_props(0)
, assumptions()
, order_heap(VarOrderLt(activity))
, progress_estimate(0)
, remove_satisfied(true)
, ca()
, lbdLocalAvg()
, sumLBD(-1.0f)
, seen()
, permDiff()
, lbdHelperCounter(0)
, analyze_stack()
, analyze_toclear()
, add_tmp()
, max_learnts(0)
, use_learnts(-1) 
, learntsize_adjust_confl(-1.0)
, learntsize_adjust_cnt(-1)

// Resource constraints:
//
, conflict_budget(-1)
, propagation_budget(-1)
, nbExportedClauses(0)
, usePsm(true)
, initLimit(INIT_LIMIT)
, maxLBDExchanged(8)
, maxLBD(20)
, restartAvgLBD(true)
, picoRestart(false)
, picobase(100)
, picoBaseUpdate(1.0)
, picolimit(100)
, picoLimitUpdate(1.1)
, exportPolicy(EXCHANGE_LBD)
, importPolicy(IMPORT_FREEZE){
    
}

Solver::~Solver() {
    if(nbClauseUsed!=NULL){
        delete[](nbClauseUsed);
    }
    if(nbClauseImported!=NULL){
        delete[](nbClauseImported);
    }
}

void Solver::initialize(Cooperation* coop, int t, const INIParser& parser){
    nbClauseUsed = new uint64_t[coop->nThreads()];
    nbClauseImported = new uint64_t[coop->nThreads()];
    for(int i=0; i<coop->nbThreads; i++){
        nbClauseImported[i] = 0;
        nbClauseUsed[i] = 0;
    }

    threadId = t;

    std::stringstream sName;
    sName << "solver" << threadId;
    std::string solver(sName.str());
    if(!parser.configurationExist(solver)){
        solver = "default";
    }

    const std::string& usePsmStr(getValue(solver,"usePsm",parser));
    if(usePsmStr.length()>0){
        if(usePsmStr==std::string("true")){
            usePsm = true;
        }else if (usePsmStr==std::string("false")){
            usePsm = false;
        }else{
            std::cerr << "Unknown value for \"usePsm\" on configuration \"";
            std::cerr << solver << "\". Allowed values are: true/false";
            std::cerr << std::endl;
        }
    }

    const std::string& maxFreezeStr(getValue(solver,"maxFreeze",parser));
    if(maxFreezeStr.length()>0){
        maxFreeze = atoi(maxFreezeStr.c_str());
    }

    const std::string& extraImportedFreezeStr(getValue(solver,"extraImportedFreeze",parser));
    if(extraImportedFreezeStr.length()>0){
        extraImportedFreeze = atoi(extraImportedFreezeStr.c_str());
    }

    const std::string& controlReduceStr(getValue(solver,"initialNbConflictBeforeReduce",parser));
    if(controlReduceStr.length()>0){
        initLimit = atoi(controlReduceStr.c_str());
        controlReduce = initLimit;
    }

    const std::string& controlReduceIncrementStr(getValue(solver,"nbConflictBeforeReduceIncrement",parser));
    if(controlReduceIncrementStr.length()>0){
        controlReduceIncrement = atoi(controlReduceIncrementStr.c_str());
    }

    const std::string& maxLBDExchangeStr(getValue(solver,"maxLBDExchange",parser));
    if(maxLBDExchangeStr.length()>0){
        maxLBDExchanged = atoi(maxLBDExchangeStr.c_str());
    }

    const std::string& maxLBDStr(getValue(solver,"maxLBD",parser));
    if(maxLBDStr.length()>0){
        maxLBD = atoi(maxLBDStr.c_str());
    }

    const std::string& lubyFactorStr(getValue(solver,"lubyFactor",parser));
    if(lubyFactorStr.length()>0){
        restart_first = atoi(lubyFactorStr.c_str());
    }

    const std::string& restartStr(getValue(solver,"restartPolicy",parser));
    if(restartStr.length()>0){
        if(restartStr == std::string("avgLBD")){
            restartAvgLBD = true;
            picoRestart = false;
            widthBasedRestart = false;
        }else if (restartStr == std::string("luby")){
            restartAvgLBD = false;
            picoRestart = false;
            widthBasedRestart = false;
        }else if (restartStr == std::string("picosat")){
            restartAvgLBD = false;
            picoRestart = true;
            widthBasedRestart = false;
        }else if (restartStr == std::string("widthBased")){
            restartAvgLBD = false;
            picoRestart = false;
            widthBasedRestart = true;
        }else{
            std::cerr << "Unknown value for \"restartPolicy\" on configuration \"";
            std::cerr << solver << "\". Allowed values are: avgLBD/luby";
            std::cerr << std::endl;
        }
    }
    
    const std::string& picobaseStr(getValue(solver,"picobase",parser));
    if(picobaseStr.length()>0){
        picobase =atoi(picobaseStr.c_str());
    }
    
    const std::string& picobaseFactStr(getValue(solver,"picobaseFactor",parser));
    if(picobaseFactStr.length()>0){
        picoBaseUpdate = atof(picobaseFactStr.c_str());
    }
    
    const std::string& picolimitStr(getValue(solver,"picolimit",parser));
    if(picolimitStr.length()>0){
        picolimit =atoi(picolimitStr.c_str());
    }
    
    const std::string& picolimitFactStr(getValue(solver,"picolimitFactor",parser));
    if(picolimitFactStr.length()>0){
        picoLimitUpdate = atof(picolimitFactStr.c_str());
    }
    
    const std::string& exportPolicyStr(getValue(solver,"exportPolicy",parser));
    if(exportPolicyStr.length()>0){
        if(exportPolicyStr == std::string("lbd")){
            exportPolicy = EXCHANGE_LBD;
        }else if (exportPolicyStr == std::string("unlimited")){
            exportPolicy = EXCHANGE_UNLIMITED;
        }else if (exportPolicyStr == std::string("legacy")){
            exportPolicy = EXCHANGE_LEGACY;;
        }else{
            std::cerr << "Unknown value for \"exportPolicy\" on configuration \"";
            std::cerr << solver << "\". Allowed values are: lbd/unlimited/legacy";
            std::cerr << std::endl;
        }
    }


    const std::string& importPolicyStr(getValue(solver,"importPolicy",parser));
    if(importPolicyStr.length()>0){
        if(importPolicyStr == std::string("freeze")){
            importPolicy = IMPORT_FREEZE;
        }else if (importPolicyStr == std::string("no-freeze")){
            importPolicy = IMPORT_NO_FREEZE;
        }else if (importPolicyStr == std::string("freeze-all")){
            importPolicy = IMPORT_FREEZE_ALL;
        }else{
            std::cerr << "Unknown value for \"importPolicy\" on configuration \"";
            std::cerr << solver << "\". Allowed values are: freeze/no-freeze/freeze-all";
            std::cerr << std::endl;
        }
    }

    const std::string& rejectAtImportStr(getValue(solver,"rejectAtImport",parser));
    if(rejectAtImportStr.length()>0){
        if(rejectAtImportStr == std::string("true")){
            rejectAtImport = true;
        }else if (rejectAtImportStr == std::string("false")){
            rejectAtImport = false;
        }
    }

    const std::string& maxLBDAcceptedStr(getValue(solver,"rejectLBD",parser));
    if(maxLBDAcceptedStr.length()>0){
        maxLBDAccepted = atoi(maxLBDAcceptedStr.c_str());
    }

    const std::string& lexicoFirstStr(getValue(solver,"lexicographicalFirstPropagation",parser));
    if(lexicoFirstStr.length()>0){
        if(lexicoFirstStr == std::string("true")){
            lexicoFirstInterpret = true;
        }else if (lexicoFirstStr == std::string("false")){
            lexicoFirstInterpret = false;
        }
    }


    const std::string& phasePolicyStr(getValue(solver,"initPhasePolicy",parser));
    if(phasePolicyStr.length()>0){
        if(phasePolicyStr == std::string("true")){
            fphase = allTrue;
        }else if (phasePolicyStr == std::string("false")){
            fphase = allFalse;
        }else if (phasePolicyStr == std::string("random")){
            fphase = randomize;
        }else{
            std::cerr << "Unknown value for \"initPhasePolicy\" on configuration \"";
            std::cerr << solver << "\". Allowed values are: true/false/random";
            std::cerr << std::endl;
        }
    }

    const std::string& restartFactorStr(getValue(solver,"restartFactor",parser));
    if(restartFactorStr.length()>0){
        restartFactor = atof(restartFactorStr.c_str());
    }

    const std::string& historicLengthStr(getValue(solver,"historicLength",parser));
    if(historicLengthStr.length()>0){
        historicLength = atoi(historicLengthStr.c_str());
    }
    
    const std::string& trailAvgSizeStr(getValue(solver,"trailAvgSize",parser));
    if(trailAvgSizeStr.length() > 0){
        trailAvgSize = atoi(trailAvgSizeStr.c_str());
    }
    
    const std::string& nbConfBeforeRestartDelayStr(getValue(solver,"nbConfBeforeRestartDelay", parser));
    if(nbConfBeforeRestartDelayStr.length()>0){
        nbConfBeforeRestartDelay = atoi(nbConfBeforeRestartDelayStr.c_str());
    }
    
    const std::string& trailAvgFactorStr(getValue(solver,"trailAvgFactor", parser));
    if(trailAvgFactorStr.length()>0){
        trailAvgFactor = atof(trailAvgFactorStr.c_str());
    }
    
    const std::string& widthRestartRStr(getValue(solver,"widthRestartR", parser));
    if(widthRestartRStr.length()>0){
        widthRestartR = atoi(widthRestartRStr.c_str());
    }
    
    const std::string& widthRestartWStr(getValue(solver, "widthRestartW", parser));
    if(widthRestartWStr.length()>0){
        widthRestartW = atoi(widthRestartWStr.c_str());
    }
    
    const std::string& widthRestartCStr(getValue(solver, "widthRestartC", parser));
    if(widthRestartCStr.length() > 0){
        widthRestartC = atoi(widthRestartCStr.c_str());
    }

}

void Solver::getProvenLiterals(vec<Lit>& provenLits) const {
    for (int i = 0; i < assigns.size(); i++) {
        if (assigns[i] != l_Undef && level(i) == 0) {
            //we have to check if assigns[i] != l_True because
            //the second arg of mkLit needs to be set to 
            provenLits.push(mkLit(i, assigns[i] != l_True));
        }
    }
}

const std::string& Solver::getValue(const std::string& solv, const std::string& attribute, const INIParser& parser) const{

    const std::string& v(parser.getValueForConf(solv,attribute));
    if(v.length()==0){
        return parser.getValueForConf("default", attribute);
    }else{
        return v;
    }
    
}

//=================================================================================================
// Minor methods:


// Creates a new SAT variable in the solver. If 'decision' is cleared, variable will not be
// used as a decision variable (NOTE! This has effects on the meaning of a SATISFIABLE result).
//

void Solver::initialiseMem(int nbVar, int ){
    watches.initMem(nbVar);
    assigns.capacity(nbVar);
    vardata.capacity(nbVar);
    activity.capacity(nbVar);
    seen.capacity(nbVar);
    permDiff.capacity(nbVar);
    polarity.capacity(nbVar);
    savePolarity.capacity(nbVar);
    viewVariable.capacity(nbVar);
    trail.capacity(nbVar);
    //clauses.capacity(nbClauses);
}

Var Solver::newVar(bool sign, bool dvar) {
    int v = nVars();
    watches .init(mkLit(v, false));
    watches .init(mkLit(v, true));
    assigns .push(l_Undef);
    vardata .push(mkVarData(CRef_Undef, 0));
    //activity .push(0);
    activity .push(rnd_init_act ? drand(random_seed, threadId) * 0.00001 : 0);
    seen .push(0);
    permDiff .push(0);
    switch (fphase){
        case allFalse: {sign=false;break;}
        case allTrue: {sign = true;break;}
        case randomize: {sign = drand(random_seed, threadId) > 0.5;break;}
    }
    polarity.push(sign);
    savePolarity .push(sign);
    viewVariable .push(false);
    decision .push();
    trail .capacity(v + 1);
    setDecisionVar(v, dvar);
    return v;
}

bool Solver::addClause_(vec<Lit>& ps) {
    ASSERT_EQUAL(0, decisionLevel());
    if (!ok) return false;

    // Check if clause is satisfied and remove false/duplicate literals:
    sort(ps);
    Lit p;
    int i, j;
    for (i = j = 0, p = lit_Undef; i < ps.size(); i++)
        if (value(ps[i]) == l_True || ps[i] == ~p)
            return true;
        else if (value(ps[i]) != l_False && ps[i] != p)
            ps[j++] = p = ps[i];
    ps.shrink(i - j);
    
    if (ps.size() == 0)
        return ok = false;
    else if (ps.size() == 1) {
        uncheckedEnqueue(ps[0]);
        return ok = (propagate() == CRef_Undef);
    } else {
        CRef cr = ca.alloc(ps, false);
        ca[cr].setGenerator(-1);
        clauses.push(cr);
        attachClause(cr);
    }
    
    return true;
}

void Solver::attachClause(CRef cr) {
    Clause& c = ca[cr];
    c.incrementNbAttach();
    nbActiveClauses++;
    ASSERT_TRUE(c.size() > 1);
    watches[~c[0]].push(Watcher(cr, c[1]));
    watches[~c[1]].push(Watcher(cr, c[0]));
    if (c.learnt()) learnts_literals += c.size();
    else clauses_literals += c.size(); 
    c.isAttached(1);
    if(c.learnt()) use_learnts++;
}

void Solver::detachClause(CRef cr, bool strict) {
    Clause& c = ca[cr];
    nbActiveClauses--;
    ASSERT_TRUE(c.size() > 1);

    if (strict) {
        remove(watches[~c[0]], Watcher(cr, c[1]));
        remove(watches[~c[1]], Watcher(cr, c[0]));
    } else {
        // Lazy detaching: (NOTE! Must clean all watcher lists before garbage 
        // collecting this clause)
        watches.smudge(~c[0]);
        watches.smudge(~c[1]);
    }

    if (c.learnt()) learnts_literals -= c.size();
    else clauses_literals -= c.size(); 
    c.isAttached(0);
    if(c.learnt()) use_learnts--;
}

void Solver::removeClause(CRef cr) {
    Clause& c = ca[cr];
    if(c.isAttached()) detachClause(cr);
    // Don't leave pointers to free'd memory!
    if (locked(c)) vardata[var(c[0])].reason = CRef_Undef;
    c.mark(1);
    ca.free(cr);
}

bool Solver::satisfied(const Clause& c) const {
    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) == l_True)
            return true;
    return false;
}


void Solver::cancelUntil(int aLevel) {
    if (decisionLevel() > aLevel) {
        for (int c = trail.size() - 1; c >= trail_lim[aLevel]; c--) {
            Var x = var(trail[c]);
            assigns [x] = l_Undef;
            if (phase_saving > 1 || ((phase_saving == 1) && c > trail_lim.last())){
              if(polarity[x] != sign(trail[c])){
                  if(sign(trail[c]) == savePolarity[x]) hammingDistance--;
                  else hammingDistance++;
              }
              polarity[x] = sign(trail[c]);
            }
            insertVarOrder(x);
        }
        qhead = trail_lim[aLevel];
        trail.shrink(trail.size() - trail_lim[aLevel]);
        trail_lim.shrink(trail_lim.size() - aLevel);
    }
}


//=================================================================================================
// Major methods:

Lit Solver::pickBranchLit() {
    Var next = var_Undef;

    // Random decision:
    if(random_var_freq > 0){
        int i = 0;
        i++;
    }
    if(random_seed > 0){
        int i = 0;
        i++;
    }
    if(threadId > 0){
        int i = 0;
        i++;
    }
    if(firstInterpretation){
        int i = 0;
        i++;
    }
    if ((drand(random_seed, threadId) < random_var_freq && !order_heap.empty())
            || firstInterpretation) {
        next = order_heap[irand(random_seed, order_heap.size(), threadId)];
        if (value(next) == l_Undef && decision[next])
            rnd_decisions++;
    }

    // Activity based decision:
    while (next == var_Undef || value(next) != l_Undef || !decision[next])
        if (order_heap.empty()) {
            next = var_Undef;
            break;
        } else
            next = order_heap.removeMin();

    return next == var_Undef ? lit_Undef : mkLit(next, polarity[next]);
}

/*_________________________________________________________________________________________________
 |
 |  analyze : (confl : Clause*) (out_learnt : vec<Lit>&) (out_btlevel : int&)  ->  [void]
 |  
 |  Description:
 |    Analyze conflict and produce a reason clause.
 |  
 |    Pre-conditions:
 |      * 'out_learnt' is assumed to be cleared.
 |      * Current decision level must be greater than root level.
 |  
 |    Post-conditions:
 |      * 'out_learnt[0]' is the asserting literal at level 'out_btlevel'.
 |      * If out_learnt.size() > 1 then 'out_learnt[1]' has the greatest decision level of the 
 |        rest of literals. There may be others from the same level though.
 |  
 |________________________________________________________________________________________________@*/
void Solver::analyze(CRef confl, vec<Lit>& out_learnt, int& out_btlevel, unsigned int& lbd) {
    int pathC = 0;
    Lit p = lit_Undef;
    vec<Lit> lastDecisionLevel;

    // Generate conflict clause:
    //
    out_learnt.push(); // (leave room for the asserting literal)
    int index = trail.size() - 1;
    
    uint32_t abstract_level = 0;

    do {
        ASSERT_TRUE(confl != CRef_Undef); // (otherwise should be UIP)
        Clause& c = ca[confl];

        if (c.learnt())
            claBumpActivity(c);

        for (int j = (p == lit_Undef) ? 0 : 1; j < c.size(); j++) {
            Lit q = c[j];

            if (!seen[var(q)] && level(var(q)) > 0) {
                varBumpActivity(var(q));
                seen[var(q)] = 1;
                if (level(var(q)) >= decisionLevel()) {
                    pathC++;
                    if(restartAvgLBD){
                        // UPDATE VAR ACTIVITY trick (see competition 09 companion paper)
                        if((reason(var(q))!= CRef_Undef)  && ca[reason(var(q))].learnt())
                            lastDecisionLevel.push(q);
                    }
                }else{
                    out_learnt.push(q);
                    abstract_level |= abstractLevel(var(q));
                }
            }
        }

        // Select next clause to look at:
        while (!seen[var(trail[index--])]);
        p = trail[index + 1];
        confl = reason(var(p));
        seen[var(p)] = 0;
        pathC--;
    } while (pathC > 0);
    out_learnt[0] = ~p;

    // Simplify conflict clause:
    //
    int i, j;
    out_learnt.copyTo(analyze_toclear);
    for (i = j = 1; i < out_learnt.size(); i++) {
        if (reason(var(out_learnt[i])) == CRef_Undef ||
                !litRedundant(out_learnt[i], abstract_level))
            out_learnt[j++] = out_learnt[i];
    }

    max_literals += out_learnt.size();
    out_learnt.shrink(i - j);
    tot_literals += out_learnt.size();
    
    // Find the LBD measure 
    lbd = 0;
    lbdHelperCounter++;
    for(int tmpI=0;tmpI<out_learnt.size();tmpI++)
    {
        
        int l = level(var(out_learnt[tmpI]));
        if (permDiff[l] != lbdHelperCounter) {
            permDiff[l] = lbdHelperCounter;
            lbd++;
        }
    }

    // Find correct backtrack level:
    //
    if (out_learnt.size() == 1)
        out_btlevel = 0;
    else {
        int max_i = 1;
        // Find the first literal assigned at the next-highest level:
        for (int tmpI = 2; tmpI < out_learnt.size(); tmpI++)
            if (level(var(out_learnt[tmpI])) > level(var(out_learnt[max_i])))
                max_i = tmpI;
        // Swap-in this literal at index 1:
        Lit curLit = out_learnt[max_i];
        out_learnt[max_i] = out_learnt[1];
        out_learnt[1] = curLit;
        out_btlevel = level(var(curLit));
    }

    if (restartAvgLBD) {
        // UPDATE Activity for good variables.... glucose hack !
        if (lastDecisionLevel.size() > 0) {
            for (int tmpI = 0; tmpI < lastDecisionLevel.size(); tmpI++) {
                if (ca[reason(var(lastDecisionLevel[tmpI]))].lbd() < lbd)
                    varBumpActivity(var(lastDecisionLevel[tmpI]));
            }
            lastDecisionLevel.clear();
        }
    }
    
    for (int tmpJ = 0; tmpJ < analyze_toclear.size(); tmpJ++) {
        // ('seen[]' is now cleared)
        seen[var(analyze_toclear[tmpJ])] = 0;
    }
}


// Check if 'p' can be removed. 'abstract_levels' is used to abort early if the 
// algorithm is visiting literals at levels that cannot be removed later.

bool Solver::litRedundant(Lit p, uint32_t abstract_levels) {
    analyze_stack.clear();
    analyze_stack.push(p);
    int top = analyze_toclear.size();
    while (analyze_stack.size() > 0) {
        ASSERT_TRUE(reason(var(analyze_stack.last())) != CRef_Undef);
        Clause& c = ca[reason(var(analyze_stack.last()))];
        analyze_stack.pop();

        for (int i = 1; i < c.size(); i++) {
            Lit curLit = c[i];
            Var v = var(curLit);
            if (!seen[v] && level(v) > 0) {
                if (reason(v) != CRef_Undef &&
                        (abstractLevel(v) & abstract_levels) != 0) {
                    seen[v] = 1;
                    analyze_stack.push(curLit);
                    analyze_toclear.push(curLit);
                } else {
                    for (int j = top; j < analyze_toclear.size(); j++) {
                        seen[var(analyze_toclear[j])] = 0;
                    }
                    analyze_toclear.shrink(analyze_toclear.size() - top);
                    return false;
                }
            }
        }
    }

    return true;
}

/*_________________________________________________________________________________________________
 |
 |  analyzeFinal : (p : Lit)  ->  [void]
 |  
 |  Description:
 |    Specialized analysis procedure to express the final conflict in terms of assumptions.
 |    Calculates the (possibly empty) set of assumptions that led to the assignment of 'p', and
 |    stores the result in 'out_conflict'.
 |________________________________________________________________________________________________@*/
void Solver::analyzeFinal(Lit p, vec<Lit>& out_conflict) {
    out_conflict.clear();
    out_conflict.push(p);

    if (decisionLevel() == 0)
        return;

    seen[var(p)] = 1;

    for (int i = trail.size() - 1; i >= trail_lim[0]; i--) {
        Var x = var(trail[i]);
        if (seen[x]) {
            if (reason(x) == CRef_Undef) {
                ASSERT_TRUE(level(x) > 0);
                out_conflict.push(~trail[i]);
            } else {
                Clause& c = ca[reason(x)];
                for (int j = 1; j < c.size(); j++)
                    if (level(var(c[j])) > 0)
                        seen[var(c[j])] = 1;
            }
            seen[x] = 0;
        }
    }

    seen[var(p)] = 0;
}

void Solver::uncheckedEnqueue(Lit p, CRef from) {
    ASSERT_TRUE(value(p) == l_Undef);
    assigns[var(p)] = lbool(!sign(p));
    vardata[var(p)] = mkVarData(from, decisionLevel());
    trail.push_(p);

    viewVariable[var(p)] = true;
    if(from != CRef_Undef){
        Clause &cl = ca[from];
        if (cl.learnt()) cl.isUsed(1);
    }
}

/*_________________________________________________________________________________________________
 |
 |  propagate : [void]  ->  [Clause*]
 |  
 |  Description:
 |    Propagates all enqueued facts. If a conflict arises, the conflicting clause is returned,
 |    otherwise CRef_Undef.
 |  
 |    Post-conditions:
 |      * the propagation queue is empty, even if there was a conflict.
 |________________________________________________________________________________________________@*/
CRef Solver::propagate(Cooperation* coop) {
    CRef confl = CRef_Undef;
    int num_props = 0;
    watches.cleanAll();

    while (qhead < trail.size()) {
        // 'p' is enqueued fact to propagate.
        Lit p = trail[qhead++];
        vec<Watcher>& ws = watches[p];
        Watcher *i, *j, *end;
        num_props++;

        for (i = j = (Watcher*) ws, end = i + ws.size(); i != end;) {
            // Try to avoid inspecting the clause:
            Lit blocker = i->blocker;
            if (value(blocker) == l_True) {
                *j++ = *i++;
                continue;
            }


            // Make sure the false literal is data[1]:
            CRef cr = i->cref;
            Clause& c = ca[cr];
            Lit false_lit = ~p;
            if (c[0] == false_lit)
                c[0] = c[1], c[1] = false_lit;
            ASSERT_TRUE(c[1] == false_lit);
            i++;

            // If 0th watch is true, then clause is already satisfied.
            Lit first = c[0];
            Watcher w = Watcher(cr, first);
            if (first != blocker && value(first) == l_True) {
                *j++ = w;
                continue;
            }

            // Look for new watch:
            for (int k = 2; k < c.size(); k++)
                if (value(c[k]) != l_False) {
                    c[1] = c[k];
                    c[k] = false_lit;
                    watches[~c[1]].push(w);

                    goto NextClause;
                }

            // Did not find watch -- clause is unit under assignment:
            *j++ = w;

            if(!c.getUsedOnce() && c.getGenerator()>=0){
                if(c.getNbAttach()==0){
                    std::cout << "dans la propagation "<< std::endl;
                    std::string* t = NULL;
                    std::cout << t->length() << std::endl;
                }
                c.setUsedOnce();
                nbClauseUsed[c.getGenerator()]++;
            }
            
            if (value(first) == l_False) {
                confl = cr;
                qhead = trail.size();
                // Copy the remaining watches:
                while (i < end)
                    *j++ = *i++;
            } else {
                //Update the agility of the solver
                agility = agility * agilityUpdateFactor;
                if(sign(first) != polarity[var(first)]){
                    agility += 1-agilityUpdateFactor;
                }
                uncheckedEnqueue(first, cr);
                // DYNAMIC NBLEVEL trick (see competition '09 companion paper)
                if (c.learnt() && c.lbd() > 3) {
                    lbdHelperCounter++;
                    unsigned int nblevels = 0;
                    for (int tmpI = 0; tmpI < c.size(); tmpI++) {
                        int l = level(var(c[tmpI]));
                        if (permDiff[l] != lbdHelperCounter) {
                            permDiff[l] = lbdHelperCounter;
                            nblevels++;
                        }
                    }
                    if (nblevels + 1 < c.lbd()) { // improve the LBD
                        if(exportPolicy == EXCHANGE_LBD && coop != NULL && 
                                c.lbd()> maxLBDExchanged && nblevels <= maxLBDExchanged){
                            //the new value of the lbd is lower than maxLBDExchanged
                            //we should have exported it
                            c.lbd(nblevels);
                            exportClause(coop, c);
                        }
                        c.lbd(nblevels); // Update it
                    }
                }
            }

NextClause:
            ;
        }
        ws.shrink(i - j);
    }
    propagations += num_props;
    simpDB_props -= num_props;

    return confl;
}

struct reduceDB_lt {
    ClauseAllocator& ca;
    reduceDB_lt(ClauseAllocator& ca_) : ca(ca_) {}
    bool operator () (CRef x, CRef y) {
        return ca[x].size() > 2 && (ca[y].size() == 2 || ca[x].activity() < ca[y].activity()); }
};

/*_________________________________________________________________________________________________
 |
 |  reduceDB : ()  ->  [void]
 |  
 |  Description:
 |    Remove half of the learnt clauses, minus the clauses locked by the current assignment. Locked
 |    clauses are clauses that are reason to some assignment. Binary clauses are never removed.
 |________________________________________________________________________________________________@*/
void Solver::reduceDB() {
    int i, j;
    int nbAtt = 0;
    int nbDettached = 0;
    if (usePsm) {
        int nTmp;
        int backJumpInRed = decisionLevel();
        int cpt, maxLevel, nbFree, posMaxLev;
        Lit lTmp;
        bool jump = false;
        int polSize = polarity.size();
        for (i = 0; i < polSize; i++) savePolarity[i] = polarity[i];
        hammingDistance = 0;
        for (i = j = 0; i < learnts.size(); i++) {
            Clause& c = ca[learnts[i]];
            if (!c.isUsed() && c.size() > 2 && !locked(c) && c.lbd() > 3 && c.lbd() <= maxLBD) {
                cpt = maxLevel = nbFree = posMaxLev = 0;
                nTmp = (c.size() * lastDeviation) + 2;
                for (j = 0; (j < c.size()) && (cpt <= (nTmp + 1)); j++) {
                    if (savePolarity[var(c[j])] == sign(c[j])) cpt++;
                    if (!c.isAttached()) {
                        if (nbFree < 2 && value(c[j]) != l_False) {
                            // two literals different to l_False at the beginning
                            lTmp = c[j], c[j] = c[nbFree], c[nbFree++] = lTmp;
                        } else if (maxLevel < level(var(c[j]))) {
                            maxLevel = level(var(c[j]));
                            posMaxLev = j;
                        }
                    }
                }
                c.setUsefull(cpt <= nTmp);
                if (!c.isAttached() && nbFree < 2) {
                    lTmp = c[posMaxLev], c[posMaxLev] = c[nbFree], c[nbFree] = lTmp;
                    c.isUsed(1);
                    if (!nbFree) // conflict clause
                    {
                        // search the backjump level
                        maxLevel = 0;
                        //we start from 1 as we are looking when we should have
                        //propagated the literal at c[0]
                        for (int tmpJ = 1; (tmpJ < c.size()); tmpJ++)
                            if (maxLevel < level(var(c[tmpJ]))) {
                                maxLevel = level(var(c[tmpJ]));
                                posMaxLev = tmpJ;
                            }
                        lTmp = c[posMaxLev], c[posMaxLev] = c[1], c[1] = lTmp;
                    }
                    if (backJumpInRed > maxLevel) {
                        backJumpInRed = maxLevel;
                    }
                    jump = true;
                }
            } else {
                c.setUsefull(c.lbd() <= maxLBD || c.isUsed());
            }
        }
        if (jump) cancelUntil(backJumpInRed != 0 ? backJumpInRed - 1 : 0); // free level
        // Don't delete binary or locked clauses.
        for (i = j = 0; i < learnts.size(); i++) {
            Clause& c = ca[learnts[i]];
            if (c.size() <= 2 || locked(c)) {
                c.isUsed(1);
            } else {
                if (!c.isUsed()) {
                    c.setNbFreezeLeft(c.getNbFreezeLeft() - 1);
                } else if (c.getNbFreezeLeft() < maxFreeze) {
                    c.setNbFreezeLeft(maxFreeze);
                }
                c.isUsed(0);
                if (!c.isUsefull() || c.getNbFreezeLeft() == 0) {
                    if (c.isAttached()) {
                        detachClause(learnts[i], true);
                        nbDettached++;
                    }
                    if (c.getNbFreezeLeft() == 0 || c.lbd() > maxLBD) {
                        // delete clause
                        c.mark(1);
                        if (c.getGenerator() != threadId && !c.getUsedOnce()) {
                            nbImportedDeletedNoUse++;
                        }
                        if(c.getGenerator() != threadId && c.getNbAttach()==0){
                            nbClausesNeverAttached++;
                        }
                        ca.free(learnts[i]);
                        continue;
                    }
                } else {
                    if (!c.isAttached()) {
                        c.setNbFreezeLeft(maxFreeze);
                        attachClause(learnts[i]);
                        nbAtt++;
                    }
                }
            }
            learnts[j++] = learnts[i];
        }
    } else {

        double extra_lim = cla_inc / learnts.size(); // Remove any clause below this activity

        sort(learnts, reduceDB_lt(ca));
        // Don't delete binary or locked clauses. From the rest, delete clauses from the first half
        // and clauses with activity smaller than 'extra_lim':
        for (i = j = 0; i < learnts.size(); i++) {
            Clause& c = ca[learnts[i]];
            if (c.size() > 2 && !locked(c) && (i < learnts.size() / 2 || c.activity() < extra_lim)) {
                removeClause(learnts[i]);
                if (c.getGenerator() != threadId && !c.getUsedOnce()) {
                    nbImportedDeletedNoUse++;
                }
            } else {
                learnts[j++] = learnts[i];
            }
        }

    }
    learnts.shrink(i - j);
    //printf("c | %7d | %9d | %8d | %8d | %8d | %7d | %7.2f | %7.3f%% |\n", threadId, conflicts, nbActiveClauses - clauses.size(), nbAtt, nbDettached, i - j, (totalSumOfDecisionLevel / conflicts), progressEstimate()*100);
    checkGarbage();
}

void Solver::removeSatisfied(vec<CRef>& cs) {
    int i, j;
    for (i = j = 0; i < cs.size(); i++) {
        Clause& c = ca[cs[i]];
        if (satisfied(c))
            removeClause(cs[i]);
        else
            cs[j++] = cs[i];
    }
    cs.shrink(i - j);
}

void Solver::rebuildOrderHeap() {
    vec<Var> vs;
    for (Var v = 0; v < nVars(); v++)
        if (decision[v] && value(v) == l_Undef)
            vs.push(v);
    order_heap.build(vs);
}

/*_________________________________________________________________________________________________
 |
 |  simplify : [void]  ->  [bool]
 |  
 |  Description:
 |    Simplify the clause database according to the current top-level assigment. Currently, the only
 |    thing done here is the removal of satisfied clauses, but more things can be put here.
 |________________________________________________________________________________________________@*/
bool Solver::simplify(Cooperation* coop) {
    ASSERT_TRUE(decisionLevel() == 0);

    if (!ok || propagate(coop) != CRef_Undef)
        return ok = false;

    if (nAssigns() == simpDB_assigns || (simpDB_props > 0))
        return true;

    // Remove satisfied clauses:
    removeSatisfied(learnts);
    if (remove_satisfied) // Can be turned off.
        removeSatisfied(clauses);
    checkGarbage();
    rebuildOrderHeap();

    simpDB_assigns = nAssigns();
    // (shouldn't depend on stats really, but it will do for now)
    simpDB_props = clauses_literals + learnts_literals;

    return true;
}

/*_________________________________________________________________________________________________
 |
 |  search : (nof_conflicts : int) (params : const SearchParams&)  ->  [lbool]
 |  
 |  Description:
 |    Search for a model the specified number of conflicts. 
 |    NOTE! Use negative value for 'nof_conflicts' indicate infinity.
 |  
 |  Output:
 |    'l_True' if a partial assigment that is consistent with respect to the clauseset is found. If
 |    all variables are decision variables, this means that the clause set is satisfiable. 'l_False'
 |    if the clause set is unsatisfiable. 'l_Undef' if the bound on number of conflicts is reached.
 |________________________________________________________________________________________________@*/
lbool Solver::search(int nof_conflicts, Cooperation* coop) {
    
    ASSERT_TRUE(ok);
    int backtrack_level;
    int conflictC = 0;
    CRef generatedClause = CRef_Undef;
    vec<Lit> learnt_clause;
    unsigned int lbd;
    lbool answer;
    starts++;

    int nbClausesLarger = 0;


    for (;;) {
        
        
        /* <Cooperation */
        //cooperation- unit clauses received from other thread stored in
        //extraUnits vector are propagated
        if (decisionLevel() == 0) {
            propagateExtraUnits();
            extraUnits.clear();
        }
        /* Cooperation> */
        
        CRef confl = propagate(coop);
        if (confl != CRef_Undef) {


            controlReduce--;
            // CONFLICT
            conflicts++;
            conflictC++;
            
            generatedClause = CRef_Undef;
            if (decisionLevel() == 0) {
                //cooperation- unsat case: store the answer and goto Cooperation section
                coop->answers[threadId] = l_False;
                goto switchMode;
            }
            
            trailAvg.push(trail.size());
            if( conflicts>nbConfBeforeRestartDelay && lbdLocalAvg.isvalid()  
                    && trail.size()>trailAvgFactor*trailAvg.getavg()) {
                lbdLocalAvg.fastclear();
            }

            learnt_clause.clear();
            analyze(confl, learnt_clause, backtrack_level, lbd);
            cancelUntil(backtrack_level);
            lbdLocalAvg.push(lbd);
            sumLBD += lbd;

            firstInterpretation = false;

            if (learnt_clause.size() == 1) {
                uncheckedEnqueue(learnt_clause[0]);
            } else {
                generatedClause = ca.alloc(learnt_clause, true);
                learnts.push(generatedClause);
                attachClause(generatedClause);
                ca[generatedClause].lbd(lbd);
                ca[generatedClause].setGenerator(threadId);
                nbClauseImported[threadId]++;
                claBumpActivity(ca[generatedClause]);
                uncheckedEnqueue(learnt_clause[0], generatedClause);
                if(learnt_clause.size()>widthRestartW){
                    nbClausesLarger++;
                }
            }

            varDecayActivity();
            claDecayActivity();

            if (--learntsize_adjust_cnt == 0) {
                learntsize_adjust_confl *= learntsize_adjust_inc;
                learntsize_adjust_cnt = (int) learntsize_adjust_confl;
                
                if(!usePsm){
                    max_learnts *= learntsize_inc;
                }

            }
            
            /* <Cooperation */
switchMode:
            //export learnt clause wrt to size limit condition
            if(generatedClause == CRef_Undef){
                //if we reach this code it's because either learnt_clause is of
                //size 1 or because we found a conflict at decision level 0
                //If the size is 1, the lbd MUST be 1. otherwise, we don't
                //really care about the lbd because we have proven UNSAT
                exportClause(coop, learnt_clause, 1);
            }else{
                exportClause(coop, ca[generatedClause]);
                nbExportedClauses++;
            }
            //Cooperation- switch deterministic mode barrier or not
            answer = importClauses(coop);
            if (answer != l_Undef) return answer;
            if(asyncStop){
                return l_Undef;
            }
            /* Cooperation> */

        } else {

/*
 TODO: activate this when NO_USE_PSM ?
            if (learnts.size() - nAssigns() >= max_learnts)
                // Reduce the set of learnt clauses:
                reduceDB();
*/

            if (controlReduce < 0){

                controlReduce = (currentLimit += controlReduceIncrement);
                int cpt = 0;
                for (int i = 0; i < viewVariable.size(); i++) {
                    if (viewVariable[i]) {
                        viewVariable[i] = false;
                        cpt++;
                    }
                }
                // Reached bound on number of conflicts:
                progress_estimate = progressEstimate();
                //fprintf(stderr, "Deviation = %lf\n", (double) hammingDistance / (double) cpt);
                if (minDeviation > (double) hammingDistance / (double) cpt)
                    minDeviation = (double) hammingDistance / (double) cpt;
                // Reduce the set of learned clauses:
                lastDeviation = (minDeviation < 0.01) ? 0.1 : minDeviation;
                reduceDB();
                ++nbReduce;
            }

            // NO CONFLICT
            bool restart = false;
            if(luby_restart || picoRestart){
                restart = (nof_conflicts >= 0 && conflictC >= nof_conflicts) || !withinBudget();
            }else if (restartAvgLBD){
                restart = lbdLocalAvg.isvalid() && ((lbdLocalAvg.getavg()*restartFactor) > (sumLBD / conflicts));
            }else if (widthBasedRestart){
                //TODO: add parameter for this?
                restart =  nbClausesLarger > 0;
            }
            if ( restart ){
                lbdLocalAvg.fastclear();
                // Reached bound on number of conflicts:
                progress_estimate = progressEstimate();
                cancelUntil(0);
                return l_Undef;
	    }

            // Simplify the set of problem clauses:
            if (decisionLevel() == 0 && !simplify(coop)){
                return l_False;
            }

            Lit next = lit_Undef;
            while (decisionLevel() < assumptions.size()) {
                // Perform user provided assumption:
                Lit p = assumptions[decisionLevel()];
                if (value(p) == l_True) {
                    // Dummy decision level:
                    newDecisionLevel();
                } else if (value(p) == l_False) {
                    analyzeFinal(~p, conflict);
                    return l_False;
                } else {
                    next = p;
                    break;
                }
            }

            if (next == lit_Undef) {
                // New variable decision:
                decisions++;
                next = pickBranchLit();


                if (next == lit_Undef) {
                    //Cooperation- Model found: store the answer and goto Cooperation section
                    coop->answers[threadId] = l_True;
                    importClauses(coop);
                    return l_True;
                }
            }

            // Increase decision level and enqueue 'next'
            newDecisionLevel();
            uncheckedEnqueue(next);
        }
    }
}

double Solver::progressEstimate() const {
    double progress = 0;
    double F = 1.0 / nVars();

    for (int i = 0; i <= decisionLevel(); i++) {
        int beg = i == 0 ? 0 : trail_lim[i - 1];
        int end = i == decisionLevel() ? trail.size() : trail_lim[i];
        progress += pow(F, i) * (end - beg);
    }

    return progress / nVars();
}

/*
 Finite subsequences of the Luby-sequence:
 
 0: 1
 1: 1 1 2
 2: 1 1 2 1 1 2 4
 3: 1 1 2 1 1 2 4 1 1 2 1 1 2 4 8
 ...
 
 
 */

static double luby(double y, int x) {

    // Find the finite subsequence that contains index 'x', and the
    // size of that subsequence:
    int size, seq;
    for (size = 1, seq = 0; size < x + 1; seq++, size = 2 * size + 1);

    while (size - 1 != x) {
        size = (size - 1) >> 1;
        seq--;
        x = x % size;
    }

    return pow(y, seq);
}

// NOTE: assumptions passed in member-variable 'assumptions'.

lbool Solver::solve_(Cooperation* coop) {
    model.clear();
    conflict.clear();
    
    
    if (!ok){
        coop->answers[threadId] = l_False;
        return l_False;
    }


    firstInterpretation = !lexicoFirstInterpret;

    solves++;
    tailUnitLit = 0;
    
    sumLBD = 0;
    lbdLocalAvg.initSize(historicLength);
    trailAvg.initSize(trailAvgSize);

    if (!usePsm) {
        max_learnts = nClauses() * learntsize_factor;
    }
    currentLimit = initLimit;
    use_learnts               = 0;
    learntsize_adjust_confl = learntsize_adjust_start_confl;
    learntsize_adjust_cnt = (int) learntsize_adjust_confl;
    lbool status = l_Undef;


    // Search:
    int nbRestartsPerformed = 0;
    int nbMaxConflicts = picobase;
    int nbRestBefIncWidth = widthRestartR;
    while (status == l_Undef) {
        if(!picoRestart){
            double rest_base = luby_restart ?
                luby(restart_inc, nbRestartsPerformed) : pow(restart_inc, nbRestartsPerformed);
            nbMaxConflicts = rest_base * restart_first;
        }
        status = search(nbMaxConflicts, coop);
        if (!withinBudget()||asyncStop) break;
        if(picoRestart){
            nbMaxConflicts += picobase;
            if(nbMaxConflicts>picolimit){
                picobase *= picoBaseUpdate;
                picolimit *= picoLimitUpdate;
                nbMaxConflicts = picobase;
            }
        }else if(widthBasedRestart){
            nbRestBefIncWidth--;
            if(nbRestBefIncWidth == 0){
                widthRestartW += widthRestartC;
                nbRestBefIncWidth = widthRestartR;
            }
        }
        nbRestartsPerformed++;
    }

    int polSize = polarity.size();
    for(int i = 0 ; i<polSize ; i++) savePolarity[i] = polarity[i];
    hammingDistance = 0;

    if ((threadId == 0) && (verbosity >= 1))
        printf("c  =======================================================================================================================\n");


    if (status == l_True) {
        // Extend & copy model:
        model.growTo(nVars());
        for (int i = 0; i < nVars(); i++) model[i] = value(i);
    } else if (status == l_False && conflict.size() == 0)
        ok = false;

    cancelUntil(0);
    return status;
}
//=================================================================================================
// Writing CNF to DIMACS:
// 
// FIXME: this needs to be rewritten completely.

static Var mapVar(Var x, vec<Var>& map, Var& max) {
    if (map.size() <= x || map[x] == -1) {
        map.growTo(x + 1, -1);
        map[x] = max++;
    }
    return map[x];
}

void Solver::toDimacs(FILE* f, Clause& c, vec<Var>& map, Var& max) {
    if (satisfied(c)) return;

    for (int i = 0; i < c.size(); i++)
        if (value(c[i]) != l_False)
            fprintf(f, "%s%d ",
                sign(c[i]) ? "-" : "", mapVar(var(c[i]), map, max) + 1);
    fprintf(f, "0\n");
}

void Solver::toDimacs(const char *file, const vec<Lit>& assumps) {
    FILE* f = fopen(file, "wr");
    if (f == NULL)
        fprintf(stderr, "could not open file %s\n", file), exit(1);
    toDimacs(f, assumps);
    fclose(f);
}

void Solver::toDimacs(FILE* f, const vec<Lit>& ) {
    // Handle case when solver is in contradictory state:
    if (!ok) {
        fprintf(f, "p cnf 1 2\n1 0\n-1 0\n");
        return;
    }

    vec<Var> map;
    Var max = 0;

    // Cannot use removeClauses here because it is not safe
    // to deallocate them at this point. Could be improved.
    int cnt = 0;
    for (int i = 0; i < clauses.size(); i++)
        if (!satisfied(ca[clauses[i]]))
            cnt++;

    for (int i = 0; i < clauses.size(); i++)
        if (!satisfied(ca[clauses[i]])) {
            Clause& c = ca[clauses[i]];
            for (int j = 0; j < c.size(); j++)
                if (value(c[j]) != l_False)
                    mapVar(var(c[j]), map, max);
        }

    // Assumptions are added as unit clauses:
    cnt += assumptions.size();

    fprintf(f, "p cnf %d %d\n", max, cnt);

    for (int i = 0; i < assumptions.size(); i++) {
        ASSERT_TRUE(value(assumptions[i]) != l_False);
        fprintf(f, "%s%d 0\n", sign(assumptions[i]) ? "-" : "",
                mapVar(var(assumptions[i]), map, max) + 1);
    }

    for (int i = 0; i < clauses.size(); i++)
        toDimacs(f, ca[clauses[i]], map, max);

    if (verbosity > 0)
        printf("c Wrote %d clauses with %d variables.\n", cnt, max);
}


//=================================================================================================
// Garbage Collection methods:

void Solver::relocAll(ClauseAllocator& to) {
    // All watchers:
    //
    // for (int i = 0; i < watches.size(); i++)
    watches.cleanAll();
    for (int v = 0; v < nVars(); v++)
        for (int s = 0; s < 2; s++) {
            Lit p = mkLit(v, s);
            // printf(" >>> RELOCING: %s%d\n", sign(p)?"-":"", var(p)+1);
            vec<Watcher>& ws = watches[p];
            for (int j = 0; j < ws.size(); j++)
                ca.reloc(ws[j].cref, to);
        }

    // All reasons:
    //
    for (int i = 0; i < trail.size(); i++) {
        Var v = var(trail[i]);

        if (reason(v) != CRef_Undef &&
                (ca[reason(v)].reloced() || locked(ca[reason(v)]))){
            ca.reloc(vardata[v].reason, to);
        }
    }

    // All learnt:
    //
    for (int i = 0; i < learnts.size(); i++){
        ca.reloc(learnts[i], to);
    }

    // All original:
    //
    for (int i = 0; i < clauses.size(); i++){
        ca.reloc(clauses[i], to);
    }
    
    
}

void Solver::garbageCollect() {
    // Initialize the next region to a size corresponding to the estimated utilization degree. This
    // is not precise but should avoid some unnecessary reallocations for the new region:
    ClauseAllocator to(ca.size() - ca.wasted());

    relocAll(to);
    if (verbosity >= 2){
        printf("c |  Garbage collection:   %12d bytes => %12d bytes             |\n",
            ca.size() * ClauseAllocator::Unit_Size, to.size() * ClauseAllocator::Unit_Size);
    }
    to.moveTo(ca);
}


void Solver::printConfiguration() const{

    std::stringstream sName;
    sName << "solver" << threadId;
    std::string solver(sName.str());

    std::cout << "c ["<<solver<<"]"<<std::endl;
    std::cout << "c usePsm = " << (usePsm? "true" : "false") << std::endl;
    std::cout << "c maxFreeze = " << maxFreeze << std::endl;
    std::cout << "c extraImportedFreeze = " << extraImportedFreeze << std::endl;
    std::cout << "c initialNbConflictBeforeReduce = " << initLimit << std::endl;
    std::cout << "c nbConflictBeforeRestartIncrement = " << controlReduceIncrement << std::endl;
    std::cout << "c maxLBDExchange = " << maxLBDExchanged << std::endl;
    std::cout << "c maxLBD = " << maxLBD << std::endl;
    std::cout << "c restartPolicy = " << (restartAvgLBD? "avgLBD" : "luby") << std::endl;
    std::cout << "c exportPolicy = ";
    if(exportPolicy == EXCHANGE_LBD){
        std::cout << "lbd";
    }else if(exportPolicy == EXCHANGE_UNLIMITED){
        std::cout << "unlimited";
    }else if(exportPolicy == EXCHANGE_LEGACY){
        std::cout << "legacy";
    }
    std::cout << std::endl;
    std::cout << "c importPolicy = ";
    if(importPolicy == IMPORT_FREEZE){
        std::cout << "freeze";
    }else if(importPolicy == IMPORT_NO_FREEZE){
        std::cout << "no-freeze";
    }else if(importPolicy == IMPORT_FREEZE_ALL){
        std::cout << "freeze-all";
    }
    std::cout << std::endl;
    std::cout << "c rejectAtImport = ";
    if(rejectAtImport){
        std::cout << "true";
    }else{
        std::cout << "false";
    }
    std::cout << std::endl;
    std::cout << "c rejectLBD = " << maxLBDAccepted << std::endl;
    std::cout << "c lexicographicalFirstPropagation = ";
    if(rejectAtImport){
        std::cout << "true";
    }else{
        std::cout << "false";
    }
    std::cout << std::endl;
    std::cout << "c initPhasePolicy = ";
    switch (fphase){
        case allTrue: {std::cout << "true";break;}
        case allFalse: {std::cout << "false";break;}
        case randomize: {std::cout << "random";break;}
    }
    std::cout << std::endl;

    std::cout << "c restartFactor = " << restartFactor << std::endl;

    std::cout << "c historicLength = " << historicLength << std::endl;
    


}

