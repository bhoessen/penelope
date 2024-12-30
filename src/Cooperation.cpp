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
#include "penelope/core/Cooperation.h"
#include "penelope/core/Solver.h"

using namespace penelope;

Cooperation::Cooperation(int n, int l) : start(true), end(false), nbThreads(n), 
        limitExportClauses(l), pairwiseLimitExportClauses(NULL), maxLBD(0), solvers(NULL),
        answers(NULL), extraUnits(NULL), headExtraUnits(NULL), tailExtraUnits(NULL), 
        extraClauses(NULL),
        headExtraClauses(NULL), tailExtraClauses(NULL),
        extraClausesLBD(NULL),
        initFreq(INITIAL_DET_FREQUENCE), deterministic_freq(NULL), 
        nbImportedExtraUnits(NULL), nbImportedExtraClauses(NULL), learntsz(NULL), 
        ctrl(' '), aimdx(AIMDX), aimdy(AIMDY), pairwiseImportedExtraClauses(NULL), 
        deterministic_mode(false), garbage(), garbageGuardian(false, 1) {

    solvers = new Solver [nbThreads];
    answers = new lbool [nbThreads];

    extraUnits = new Lit** [nbThreads];
    headExtraUnits = new int* [nbThreads];
    tailExtraUnits = new int* [nbThreads];

    extraClauses = new Lit*** [nbThreads];
    headExtraClauses = new int* [nbThreads];
    tailExtraClauses = new int* [nbThreads];
    extraClausesLBD = new int**[nbThreads];

    for (int t = 0; t < nbThreads; t++) {
        extraUnits[t] = new Lit* [nbThreads];
        headExtraUnits[t] = new int [nbThreads];
        tailExtraUnits[t] = new int [nbThreads];

        extraClauses[t] = new Lit**[nbThreads];
        headExtraClauses[t] = new int [nbThreads];
        tailExtraClauses[t] = new int [nbThreads];
        extraClausesLBD[t] = new int*[nbThreads];

        for (int k = 0; k < nbThreads; k++) {
            extraUnits[t][k] = new Lit [MAX_EXTRA_UNITS];
            headExtraUnits[t][k] = 0;
            tailExtraUnits[t][k] = 0;

            extraClauses[t][k] = new Lit* [MAX_EXTRA_CLAUSES];
            headExtraClauses[t][k] = 0;
            tailExtraClauses[t][k] = 0;
            extraClausesLBD[t][k] = new int[MAX_EXTRA_CLAUSES];
        }
    }

    //=================================================================================================

    learntsz = new uint64_t[nbThreads];
    deterministic_freq = new int [nbThreads];
    nbImportedExtraClauses = new int [nbThreads];
    nbImportedExtraUnits = new int [nbThreads];
    pairwiseImportedExtraClauses = new int* [nbThreads];
    pairwiseLimitExportClauses = new double* [nbThreads];

    for (int t = 0; t < nbThreads; t++) {
        learntsz [t] = 0;
        answers [t] = l_Undef;
        deterministic_freq [t] = initFreq;
        nbImportedExtraClauses[t] = 0;
        nbImportedExtraUnits [t] = 0;

        pairwiseImportedExtraClauses [t] = new int [nbThreads];
        pairwiseLimitExportClauses [t] = new double[nbThreads];

        for (int k = 0; k < nbThreads; k++)
            pairwiseLimitExportClauses[t][k] = limitExportClauses;

    }
}

Cooperation::~Cooperation(){
    delete[](solvers);
    delete[](answers);
    delete[](deterministic_freq);
    delete[](nbImportedExtraClauses);
    delete[](nbImportedExtraUnits);
    delete[](learntsz);
    for (int t = 0; t < nbThreads; t++) {
        delete[](headExtraUnits[t]);
        delete[](headExtraClauses[t]);
        delete[](tailExtraUnits[t]);
        delete[](tailExtraClauses[t]);
        delete[](pairwiseImportedExtraClauses[t]);
        for(int k=0; k<nbThreads; k++){
            delete[](extraUnits[t][k]);
            delete[](extraClauses[t][k]);
            delete[](extraClausesLBD[t][k]);
        }
        delete[](extraUnits[t]);
        delete[](extraClauses[t]);
        delete[](extraClausesLBD[t]);
        delete[](pairwiseLimitExportClauses[t]);
    }
    delete[](headExtraUnits);
    delete[](headExtraClauses);
    delete[](tailExtraUnits);
    delete[](tailExtraClauses);
    delete[](pairwiseImportedExtraClauses);
    delete[](extraUnits);
    delete[](extraClauses);
    delete[](extraClausesLBD);
    delete[](pairwiseLimitExportClauses);
    for(int i=0; i<garbage.size(); i++){
        delete[](garbage[i]);
    }
}

void Cooperation::resetSolvers(){
    delete[](solvers);
    delete[](answers);
    for(int i=0; i<garbage.size(); i++){
        delete[](garbage[i]);
    }
    garbage.shrink(garbage.size());
    solvers = new Solver [nbThreads];
    answers = new lbool [nbThreads];
    for (int t = 0; t < nbThreads; t++) {
        learntsz [t] = 0;
        answers [t] = l_Undef;
        for (int k = 0; k < nbThreads; k++) {
            headExtraUnits[t][k] = 0;
            tailExtraUnits[t][k] = 0;
            headExtraClauses[t][k] = 0;
            tailExtraClauses[t][k] = 0;
        }
    }

}

void Cooperation::exportExtraUnit(Solver* s, Lit unit) {

    int id = s->threadId;

    for (int t = 0; t < nbThreads; t++) {

        if (t == id) continue;
        int ind = tailExtraUnits[id][t];
        if (((ind + 1) % MAX_EXTRA_UNITS) == headExtraUnits[id][t]) continue;

        extraUnits[id][t][ind++].x = unit.x;

        if (ind == MAX_EXTRA_UNITS) ind = 0;
        tailExtraUnits[id][t] = ind;
    }
}

void Cooperation::importExtraUnits(Solver* s, vec<Lit>& unit_learnts) {

    int id = s->threadId;

    for (int t = 0; t < nbThreads; t++) {

        if (t == id)
            continue;

        int head = headExtraUnits[t][id];
        int tail = tailExtraUnits[t][id];
        if (head == tail)
            continue;

        int localEnd = tail;
        if (tail < head) localEnd = MAX_EXTRA_UNITS;

        for (int i = head; i < localEnd; i++)
            storeExtraUnits(s, t, extraUnits[t][id][i], unit_learnts);

        if (tail < head)
            for (int i = 0; i < tail; i++)
                storeExtraUnits(s, t, extraUnits[t][id][i], unit_learnts);

        head = tail;
        if (head == MAX_EXTRA_UNITS) head = 0;
        headExtraUnits[t][id] = head;
    }
}

void Cooperation::importExtraUnits(Solver* s) {

    int id = s->threadId;

    for (int t = 0; t < nbThreads; t++) {

        if (t == id)
            continue;

        int head = headExtraUnits[t][id];
        int tail = tailExtraUnits[t][id];
        if (head == tail)
            continue;

        int localEnd = tail;
        if (tail < head) localEnd = MAX_EXTRA_UNITS;

        for (int i = head; i < localEnd; i++)
            uncheckedEnqueue(s, t, extraUnits[t][id][i]);

        if (tail < head)
            for (int i = 0; i < tail; i++)
                uncheckedEnqueue(s, t, extraUnits[t][id][i]);

        head = tail;
        if (head == MAX_EXTRA_UNITS) head = 0;
        headExtraUnits[t][id] = head;
    }
}

void Cooperation::uncheckedEnqueue(Solver* s, int t, Lit l) {

    if (s->value(l) == l_False) answers[s->threadId] = l_False;

    if (s->value(l) != l_Undef) return;
    s->uncheckedEnqueue(l);
    nbImportedExtraUnits[s->threadId]++;
    pairwiseImportedExtraClauses[t][s->threadId]++;
}

void Cooperation::storeExtraUnits(Solver* s, int t, Lit l, vec<Lit>& unitLits) {
    unitLits.push(l);
    nbImportedExtraUnits[s->threadId]++;
    pairwiseImportedExtraClauses[t][s->threadId]++;
}

void Cooperation::exportExtraClause(Solver* s, vec<Lit>& learnt, int lbd) {

    int id = s->threadId;

    for (int t = 0; t < nbThreads; t++) {

        if ((t == id) || (learnt.size() > pairwiseLimitExportClauses[id][t]))
            continue;

        int ind = tailExtraClauses[id][t];
        if (((ind + 1) % MAX_EXTRA_CLAUSES) == headExtraClauses[id][t])
            continue;

        //TODO: use allocator here to avoid problems and guardian
        Lit* tmp = new Lit [learnt.size() + 1];
        garbageGuardian.wait();
        garbage.push(tmp);
        garbageGuardian.signal();
        extraClauses[id][t][ind] = tmp;
        extraClauses[id][t][ind][0] = mkLit(learnt.size());
        extraClausesLBD[id][t][ind] = lbd;

        for (int j = 0; j < learnt.size(); j++)
            extraClauses[id][t][ind][j + 1] = learnt[j];

        ind++;
        if (ind == MAX_EXTRA_CLAUSES) ind = 0;
        tailExtraClauses[id][t] = ind;
    }
}

void Cooperation::exportExtraClause(Solver* s, Clause& c) {

    int id = s->threadId;

    if(s->getExportPolicy()== EXCHANGE_LBD && c.lbd() > s->getMaxLBDExchanged()){
        return;
    }

    for (int t = 0; t < nbThreads; t++) {

        //Check the size of the clause and if the thread t isn't the one
        //that created the clause
        if ((t == id) || (s->getExportPolicy()== EXCHANGE_LEGACY &&
                c.size() > pairwiseLimitExportClauses[id][t])){
            continue;
        }

        int ind = tailExtraClauses[id][t];
        if (((ind + 1) % MAX_EXTRA_CLAUSES) == headExtraClauses[id][t])
            continue;

        Lit* tmp = new Lit [c.size() + 1];
        garbageGuardian.wait();
        garbage.push(tmp);
        garbageGuardian.signal();
        extraClauses[id][t][ind] = tmp;
        extraClauses[id][t][ind][0] = mkLit(c.size());
        extraClausesLBD[id][t][ind] = c.lbd();

        for (int j = 0; j < c.size(); j++)
            extraClauses[id][t][ind][j + 1] = c[j];

        ind++;
        if (ind == MAX_EXTRA_CLAUSES) ind = 0;
        tailExtraClauses[id][t] = ind;
    }
}

void Cooperation::importExtraClauses(Solver* s) {

    int id = s->threadId;

    for (int t = 0; t < nbThreads; t++) {

        if (t == id)
            continue;

        int head = headExtraClauses[t][id];
        int tail = tailExtraClauses[t][id];
        if (head == tail)
            continue;

        int localEnd = tail;
        if (tail < head) localEnd = MAX_EXTRA_CLAUSES;

        for (int i = head; i < localEnd; i++)
            addExtraClause(s, t, extraClauses[t][id][i], extraClausesLBD[t][id][i]);

        if (tail < head)
            for (int i = 0; i < tail; i++)
                addExtraClause(s, t, extraClauses[t][id][i], extraClausesLBD[t][id][i]);

        head = tail;
        if (head == MAX_EXTRA_CLAUSES) head = 0;
        headExtraClauses[t][id] = head;
    }
}

void Cooperation::addExtraClause(Solver* s, int t, Lit* lt, int lbd) {

    assert(s->threadId != t);
    vec<Lit> extra_clause;
    int extra_backtrack_level = 0;
    int id = s->threadId;
    int size = var(lt[0]);
    int wtch = 0;

    for (int i = 1, j = 0; i < size + 1; i++) {
        Lit q = lt[i];
        if (s->value(q) == l_False && s->level(var(q)) == 0)
            continue;
        extra_clause.push(q);
        if (s->value(q) != l_False && wtch < 2) {
            extra_clause[j] = extra_clause[wtch];
            extra_clause[wtch++] = q;
        } else if (s->level(var(q)) >= extra_backtrack_level)
            extra_backtrack_level = s->level(var(q));
        j++;
    }


    //conflict clause at level 0 --> formula is UNSAT
    if (extra_clause.size() == 0)
        setAnswer(id, l_False);

        // case of unit extra clause
    else if (extra_clause.size() == 1) {
        s->cancelUntil(0);
        if (s->value(extra_clause[0]) == l_Undef) {
            s->uncheckedEnqueue(extra_clause[0]);
            CRef cs = s->propagate();
            if (cs != CRef_Undef) answers[id] = l_False;
        }
    } else {
        // build clause from lits and add it to learnts(s) base
        CRef cr = s->addExtraClause(extra_clause, lbd);
        if(cr != CRef_Undef){
            Clause& tmpClause = s->getClause(cr);
            tmpClause.setGenerator(t);
            tmpClause.lbd(lbd);
            s->nbClauseImported[t]++;

#ifndef FREEZE_ALL
            // Case of Unit propagation: literal to propagate or bad level
            if (wtch == 1 && (s->value(extra_clause[0]) == l_Undef || extra_backtrack_level < s->level(var(extra_clause[0])))) {
                s->cancelUntil(extra_backtrack_level);
                s->uncheckedEnqueue(extra_clause[0], cr);
            }            // Case of Conflicting Extra Clause --> analyze that conflict
            else if (wtch == 0) {
                extra_clause.clear();
                s->cancelUntil(extra_backtrack_level);

                unsigned int newLbd = 0;
                s->analyze(cr, extra_clause, extra_backtrack_level, newLbd);
                s->cancelUntil(extra_backtrack_level);
                
                // analyze lead to unit clause
                if (extra_clause.size() == 1) {
                    ASSERT_TRUE(extra_backtrack_level == 0);
                    if (s->value(extra_clause[0]) == l_Undef)s->uncheckedEnqueue(extra_clause[0]);
                }// analyze lead to clause with size > 1
                else {
                    CRef cs = s->addExtraClause(extra_clause, newLbd);
                    if (cs != CRef_Undef) {
                        Clause& generatedClause = s->getClause(cs);
                        generatedClause.setGenerator(s->threadId);
                        s->nbClauseImported[s->threadId]++;
                        s->getClause(cs).lbd(newLbd);
                        s->uncheckedEnqueue(extra_clause[0], cs);
                        //exportExtraClause(s, extra_clause);
                    }
                }
            }
#endif /* FREEZE_ALL */
        }
    }

    nbImportedExtraClauses[id]++;
    pairwiseImportedExtraClauses[t][id]++;

}

void Cooperation::updateLimitExportClauses(Solver* s) {

    int id = s->threadId;

    if ((int) s->conflicts % LIMIT_CONFLICTS_EVAL != 0) return;

    double sumExtImpCls = 0;

    for (int t = 0; t < nbThreads; t++)
        sumExtImpCls += pairwiseImportedExtraClauses[t][id];

    switch (ctrl) {

        case 1:
        {
            if (sumExtImpCls <= MAX_IMPORT_CLAUSES)
                for (int t = 0; t < nbThreads; t++)
                    pairwiseLimitExportClauses[t][id] += 1;
            else
                for (int t = 0; t < nbThreads; t++)
                    pairwiseLimitExportClauses[t][id] -= 1;

            for (int t = 0; t < nbThreads; t++)
                pairwiseImportedExtraClauses[t][id] = 0;
            break;
        }


        case 2:
        {
            if (sumExtImpCls <= MAX_IMPORT_CLAUSES)
                for (int t = 0; t < nbThreads; t++)
                    pairwiseLimitExportClauses[t][id] += aimdy / pairwiseLimitExportClauses[t][id];
            else
                for (int t = 0; t < nbThreads; t++)
                    pairwiseLimitExportClauses[t][id] -= aimdx * pairwiseLimitExportClauses[t][id];

            for (int t = 0; t < nbThreads; t++)
                pairwiseImportedExtraClauses[t][id] = 0;
            break;
        }
    }
}

void Cooperation::printStats(int& id) {

    uint64_t nbSharedExtraClauses = 0;
    uint64_t nbSharedExtraUnits = 0;
    uint64_t totalConflicts = 0;

    for (int t = 0; t < nbThreads; t++) {

        nbSharedExtraClauses += nbImportedExtraClauses[t];
        nbSharedExtraUnits += nbImportedExtraUnits [t];
        totalConflicts += solvers[t].conflicts;
    }


    printf("c  -----------------------------------------------------------------------------------------------------------------------\n");
    printf("c | Penelope - all threads statistics                DETERMINISTIC MODE ? %s            initial limit export clauses : %3d |\n",
            deterministic_mode == true ? "Y" : "N",
            limitExportClauses);
    printf("c |--------------------------------------------------------------------------------------------------------------------------------------------|\n");
    printf("c |  Thread  | winner  |   #restarts   |   decisions   |  #conflicts   |   %% shared cls  |  #extra units | #extra clauses | #not used directly | \n");
    printf("c |----------|---------|---------------|---------------|---------------|-----------------|---------------|----------------|--------------------|\n");

    for (int t = 0; t < nbThreads; t++)
        printf("c | %8d | %7s | %13d | %13d | %13d | %13d %% | %13d | %14d | %18d |\n",
            t,
            t == id ? "x" : " ",
            (int) solvers[t].starts,
            (int) solvers[t].decisions,
            (int) solvers[t].conflicts,
            (int) solvers[t].conflicts == 0 ? 0 : (int) (nbImportedExtraUnits[t] + nbImportedExtraClauses[t]) * 100 / (int) solvers[t].conflicts,
            (int) nbImportedExtraUnits[t],
            (int) nbImportedExtraClauses[t],
            solvers[t].getNbNotAttachedDirectly());


    printf("c |----------------------------------------------------|---------------|-----------------|---------------|----------------|--------------------|\n");
    printf("c |                                                    | %13d | %13d %% | %13d | %14d |                    |\n",
            (int) totalConflicts,
            (int) totalConflicts == 0 ? 0 : (int) (nbSharedExtraUnits + nbSharedExtraClauses) * 100 / (int) totalConflicts,
            (int) nbSharedExtraUnits,
            (int) nbSharedExtraClauses);
    printf("c  --------------------------------------------------------------------------------------------------------------------------------------------\n");

    uint64_t globImported = 0;
    uint64_t globUsed = 0;
    for (int i = 0; i < nbThreads; i++) {
        printf("c solver #%d usage of imported clauses: ", i);
        uint64_t totImported = 0;
        uint64_t totUsed = 0;
        for (int j = 0; j < nbThreads; j++) {
            if (i != j) {
                totImported += solvers[i].nbClauseImported[j];
                totUsed += solvers[i].nbClauseUsed[j];

            }
            globImported += totImported;
            globUsed += totUsed;
        }
        printf("%4.2f%% (%7ld/%7ld), deleted not used: %4.2f%% (%7d/%7ld) \n", (100.0 * totUsed) / totImported, totUsed, totImported, (100.0 * solvers[i].getNbImportedDeletedNotUsed() / totImported), solvers[i].getNbImportedDeletedNotUsed(), totImported);
        printf("c nb clauses never attached: %4.2f%% (%7d/%7ld)\n", (100.0 * solvers[i].nbClausesNeverAttached) / totImported, solvers[i].nbClausesNeverAttached, totImported);
        printf("c nb exported but not learnt: %7d\n", solvers[i].nbClausesNotLearnt);
    }
    printf("c global usage of imported clauses: %4.2f%% (%7ld/%7ld)\n", (100.0 * globUsed) / globImported, globUsed, globImported);

    printf("c Import usage matrix:\n");
    for(int i=0; i<nbThreads; i++){
        printf("c matrix | ");
        for(int j=0; j<nbThreads; j++){
            if(solvers[i].nbClauseImported[j]!=0){
                printf("%5.2f%% | ",(100.0*solvers[i].nbClauseUsed[j])/solvers[i].nbClauseImported[j]);

            }else{
                printf("%5.2f%% | ",0.0);
            }
        }
        printf("\n");
    }

    Parallel_Info();

}

void Cooperation::printExMatrix() {

    printf("\nc  Final Matrix extra shared clauses limit size \n");
    printf("c  ----------------------------------------------------\n");
    for (int i = 0; i < nbThreads; i++) {
        printf("c |");
        for (int j = 0; j < nbThreads; j++)
            if (i != j)
                printf(" e(%d,%d)=%3d  ", i, j, (int) pairwiseLimitExportClauses[i][j]);
            else
                printf(" e(%d,%d)=%3d  ", i, j, 0);
        printf(" |\n");
    }
    printf("c  ----------------------------------------------------\n\n");
}

void Cooperation::Parallel_Info() {
    printf("c  DETERMINISTIC_MODE? : %s \n", (deterministic_mode == true ? "YES" : "NO"));
    printf("c  CONTROL POLICY?	 : %s  \n", (ctrl != 0 ? (ctrl == 1 ? "DYNAMIC (INCREMENTAL  en= +- 1)" : "DYNAMIC (AIMD: en=  en - a x en, en + b/en)") : "STATIC (fixed Limit Export Clauses)"));
    if (ctrl == 0)
        printf("c  LIMIT EXPORT CLAUSES (PAIRWISE) : [identity] x %d\n\n", limitExportClauses);
    else
        printExMatrix();
}

