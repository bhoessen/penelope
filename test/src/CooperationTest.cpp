#include "CooperationTest.h"
#include "penelope/core/Cooperation.h"
#include "penelope/core/Dimacs.h"
#include "Thread.h"

CPPUNIT_TEST_SUITE_REGISTRATION(CooperationTest);

using namespace penelope;

void CooperationTest::testdp10() {
    CPPUNIT_ASSERT_EQUAL(true, solve("instances/dp10s10.shuffled.cnf"));
}

void CooperationTest::testdp04s() {
    CPPUNIT_ASSERT_EQUAL(true, solve("instances/dp04s04.shuffled.cnf"));
}

void CooperationTest::testdp04u() {
    CPPUNIT_ASSERT_EQUAL(false, solve("instances/dp04u03.shuffled.cnf"));
}

void CooperationTest::testaaai10() {
    CPPUNIT_ASSERT_EQUAL(false, solve("instances/aaai10-planning-ipc5-rovers-18-step11.cnf", 2));
}

class SolverLauncher : public Thread {
public:

    SolverLauncher(Solver* aSolver, Cooperation* aCoop) : Thread(), s(aSolver), c(aCoop) {
    }

    void run() {
        vec<Lit> assumpt;
        s->solveLimited(assumpt, c);
    }

private:
    Solver* s;
    Cooperation* c;

};

bool CooperationTest::solve(const char* fileName, int nbThreads ) {
    int limitExport = 10;
    Cooperation coop(nbThreads, limitExport);

    coop.ctrl = 0;
    coop.deterministic_mode = true;
    INIParser parser(std::string("configuration.ini"));
    parser.parse();
    for (int t = 0; t < nbThreads; t++) {
        coop.solvers[t].initialize(&coop, t, parser);
        coop.solvers[t].threadId = t;
        coop.solvers[t].verbosity = 0;
        coop.solvers[t].deterministic_mode = true;
    }

    FILE* in = fopen(fileName, "rb");
    CPPUNIT_ASSERT(in != NULL);
    DimacsParser::parse_DIMACS(in, &coop);
    fclose(in);

    SolverLauncher * threads[nbThreads];
    for (int t = 0; t < nbThreads; t++) {
        threads[t] = new SolverLauncher(&(coop.solvers[t]), &coop);
        threads[t]->start();
    }

    bool solFound = false;
    bool sol = false;
    for (int t = 0; t < nbThreads; t++) {
        threads[t]->join();
        if (coop.answer(t) != l_Undef) {
            solFound = true;
            sol = (coop.answer(t) == l_True);
        }
        delete(threads[t]);
    }

    CPPUNIT_ASSERT(solFound);

    return sol;

}


