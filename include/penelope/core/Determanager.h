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



//=================================================================================================

using namespace penelope;

lbool Solver::importClauses(Cooperation* coop) {
  
  //Control the limit size clause export
  coop->updateLimitExportClauses(this);


  switch(deterministic_mode){

  case 0:   // non deterministic case
    {
      asyncStop = asynch_interrupt;
      for(int t = 0; t < coop->nThreads(); t++)
	if(coop->answer(t) != l_Undef)
	  return coop->answer(t);
      
      coop->importExtraClauses(this);
      coop->importExtraUnits(this, extraUnits);
      break;
    }
    
  case 1:  // deterministic case static frequency
    {
      if((int) conflicts % coop->initFreq == 0 || coop->answer(threadId) != l_Undef){						

#pragma omp barrier
        asyncStop = asynch_interrupt;
	for(int t = 0; t < coop->nThreads(); t++)
	  if(coop->answer(t) != l_Undef) return coop->answer(t);
	
	coop->importExtraClauses(this);
	coop->importExtraUnits(this, extraUnits);
	
#pragma omp barrier
      }
      
      break;
    }

  case 2: // deterministic case dynamic frequency
    {
      if(((int) conflicts % coop->deterministic_freq[threadId] == 0) || (coop->answer(threadId) != l_Undef)){
        coop->learntsz[threadId] = nLearnts();
#pragma omp barrier
        asyncStop = asynch_interrupt;
	// each thread has its own frequency barrier synchronization
	updateFrequency(coop);

	coop->deterministic_freq[threadId] = updateFrequency(coop);
	
	for(int t = 0; t < coop->nThreads(); t++)
	  if(coop->answer(t) != l_Undef) return coop->answer(t);
	
	coop->importExtraClauses(this);
	coop->importExtraUnits(this, extraUnits);

#pragma omp barrier
      }
      
      break;
    }
      default:
          ASSERT_TRUE(false);
  }
  return l_Undef;
}


int Solver::updateFrequency(Cooperation* coop){
  
  double freq = 0;
  int maxLearnts = 0;
  
  for(int t = 0; t < coop->nThreads(); t++)
    if((int)coop->learntsz[t] > maxLearnts)
      maxLearnts = (int)coop->learntsz[t];
  
  freq = coop->initFreq + (double)coop->initFreq * (maxLearnts -learnts.size()) / maxLearnts;
  return (int) freq;
}

void Solver::exportClause(Cooperation* coop, vec<Lit>& learnt_clause, int lbd) {
  
  if(coop->limitszClauses() < 1) 
    return;

  if(decisionLevel() == 0){
    for(int i = tailUnitLit; i < trail.size(); i++)
      coop->exportExtraUnit(this, trail[i]) ;
    tailUnitLit = trail.size();
  }else
    coop->exportExtraClause(this, learnt_clause, lbd);
}


void Solver::exportClause(Cooperation* coop, Clause& generatedClause) {
  
  if(coop->limitszClauses() < 1) 
    return;

  
  if(decisionLevel() == 0){
    for(int i = tailUnitLit; i < trail.size(); i++)
      coop->exportExtraUnit(this, trail[i]) ;
    tailUnitLit = trail.size();
  }else
    coop->exportExtraClause(this, generatedClause);
}

CRef Solver::addExtraClause(vec<Lit>& lits, int lbd){
  CRef cr = CRef_Undef;

  if (importPolicy == IMPORT_FREEZE) {
      //Compute the psm condition before adding extra clause
      int nTmp = 1;
      int cpt = 0;
      int sz = lits.size();
      if (lbd > 3) {
          
           nTmp = ( sz * lastDeviation) + 2;
          for (int j = 0; (j < sz)/* && (cpt <= (nTmp + 1))*/; j++) {
              if (polarity[var(lits[j])] == sign(lits[j])) cpt++;
          }
          
      }

      //printf("cpt:%d sz:%d nTmp:%d lastDeviation:%4.2f lbd:%d\n", cpt, sz, nTmp,lastDeviation, lbd);
      if(!rejectAtImport || (rejectAtImport && lbd<maxLBDAccepted)){
          cr = ca.alloc(lits, true);
          Clause& c = ca[cr];
          c.setUsefull(cpt <= nTmp);
          c.lbd(lbd);
          learnts.push(cr);
          if (c.lbd() <= 3 || c.isUsefull()) {    
              attachClause(cr);
          } else {
              nbNotAttachedDirectly++;
              c.setNbFreezeLeft(maxFreeze + extraImportedFreeze);
              c.isUsed(false);
              c.isAttached(0);
          }
          claBumpActivity(c);
      }else{
          nbClausesNotLearnt++;
      }
      
  }else if(importPolicy==IMPORT_NO_FREEZE){
      cr = ca.alloc(lits, true);
      Clause& c = ca[cr];
      c.lbd(lbd);
      learnts.push(cr);
      attachClause(cr);
      claBumpActivity(c);
  }else if (importPolicy==IMPORT_FREEZE_ALL){
      cr = ca.alloc(lits, true);
      Clause& c = ca[cr];
      c.lbd(lbd);
      learnts.push(cr);
      nbNotAttachedDirectly++;
      c.setNbFreezeLeft(maxFreeze + extraImportedFreeze);
      c.isUsed(false);
      c.isAttached(0);
      claBumpActivity(c);
  }

  
  return cr;
}

void Solver::propagateExtraUnits(){
  for(int i = 0; i < extraUnits.size(); i++)
    if(value(extraUnits[i]) == l_Undef)
      uncheckedEnqueue(extraUnits[i]);
}
