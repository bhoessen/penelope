#include "Thread.h"

using namespace penelope;

const unsigned int Thread::THREAD_STOP = 0;
const unsigned int Thread::THREAD_JOIN = 1;
const unsigned int Thread::THREAD_START = 2;

/**
 * This function is the one used to launch the pthreads
 */
void* launchPThread(void* data);

void* launchPThread(void* data) {
    Thread* t = NULL;
    t = static_cast<Thread*> (data);
    if(t == NULL){
        throw Exception(-1, 2, "Couldn't make the cast");
    }
    t->run();
    return (void*) t;
}

Thread::Thread() : thread(0), threadCreated(false) {

}

Thread::~Thread() {

}

void Thread::start() throw (Exception) {
    if (!threadCreated) {
        int code = pthread_create(&thread, NULL, launchPThread, (void*) this);
        if (code != 0) {
            throw Exception(code, THREAD_START, "Unable to create thread");
        } else {
            threadCreated = true;
        }
    }else{
        throw Exception(-1, THREAD_START, "A thread was already created");
    }
}

void Thread::stop() throw (Exception) {
    if (threadCreated) {
        int code = pthread_cancel(thread);
        if (code != 0) {
            throw Exception(code, THREAD_STOP, "Unable to stop the thread");
        } else {
            threadCreated = false;
            thread = 0;
        }
    }else{
        throw Exception(-1, THREAD_STOP, "No thread were created");
    }
}

void Thread::join() throw (Exception) {
    if (threadCreated) {
        void* r = NULL;
        int code = pthread_join(thread, &r);
        if (code != 0) {
            throw Exception(code, THREAD_JOIN, "Unable to stop the thread");
        } else {
            threadCreated = false;
            thread = 0;
        }
    }else{
        throw Exception(-1, THREAD_JOIN, "No thread were created");
    }
}
