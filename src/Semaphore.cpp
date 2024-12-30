#include "penelope/utils/Semaphore.h"

using namespace penelope;

#ifdef WIN32
#include <limits.h>

Semaphore::Semaphore(bool multiProcessShared, int initialValue){
	ghSemaphore = CreateSemaphore(NULL, initialValue, INT_MAX, NULL);
}

Semaphore::~Semaphore(){
    CloseHandle(ghSemaphore);
}

void Semaphore::signal(){
    ReleaseSemaphore(ghSemaphore, 1, NULL);
}

void Semaphore::wait(){
    WaitForSingleObject(ghSemaphore, 0L);
}

bool Semaphore::tryWait(){
    return WaitForSingleObject(ghSemaphore, 1L) == WAIT_TIMEOUT;
}

#else

Semaphore::Semaphore(bool multiProcessShared, int initialValue){
    sem_init(&sema,multiProcessShared? 0 : 1, initialValue);
}

Semaphore::~Semaphore(){
    sem_destroy(&sema);
}

void Semaphore::signal(){
    sem_post(&sema);
}

void Semaphore::wait(){
    sem_wait(&sema);
}

bool Semaphore::tryWait(){
    return sem_trywait(&sema) == 0;
}
#endif /* WIN32 */
