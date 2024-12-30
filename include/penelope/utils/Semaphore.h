/* 
 * File:   Semaphore.h
 * Author: bhoessen
 *
 * Created on 4 novembre 2011, 13:39
 */

#ifndef SEMAPHORE_H
#define	SEMAPHORE_H

#ifdef WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <semaphore.h>
#endif /* WIN32 */

namespace penelope{
    
    /**
     * This class represents a semaphore
     */
    class Semaphore{
    private:
#ifdef WIN32
        /** The win32 semaphore */
        HANDLE ghSemaphore;
#else
        /** The posix semaphore related to this object */
        sem_t sema;
#endif

        
    public:
        
        /**
         * Creates a new Semaphore
         * @param multiProcessShared if set to true, the semaphore can be shared
         *        across different process, otherwise it can only be shared
         *        across the different threads of a same process
         * @param initialValue the initial value of the semaphore
         */
        Semaphore(bool multiProcessShared, int initialValue);
        
        /**
         * Destroy the semaphore
         */
        virtual ~Semaphore();
        
        /**
         * Perform a signal operation to this semaphore.
         * 
         * Let s be the value of the semaphore
         *  if s is null and the queue isn't empty, one of the waiting process
         *  will be taken out of the queue and will be able to continue. 
         *  Otherwise, the value of the semaphore will be incremented
         *    
         */
        void signal();
        
        /**
         * Perform the wait operation on this semaphore.
         * 
         * Let s be the value of the semaphore
         *  if s is greater than zero, the value is decremented. Otherwise, the
         *  process performing the wait operation will be added in the queue of
         *  this semaphore
         */
        void wait();
        
        /**
         * Try to perform a wait operation on this semaphore
         * Let s be the value of the semaphore
         *  if s is greater than zero, the value is decremented. Otherwise, 
         *  nothing will happen
         * @return true if we successfully decremented the semaphore, false
         *         otherwise
         */
        bool tryWait();
        
    };
    
}

#endif	/* SEMAPHORE_H */
