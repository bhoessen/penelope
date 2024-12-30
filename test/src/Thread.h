/*
Copyright (c) <2012> <B.Hoessen>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
 */

#ifndef THREAD_H
#define THREAD_H
#include <pthread.h>
#include "Exception.h"

namespace penelope {

    /**
     * This abstract class provide an easy implementation for threads.
     * To be able to create a thread, the user only needs to inherit from
     * this class and implement the run method.
     */
    class Thread {
    public:

        /** code representing the method that triggered an error in stop */
        static const unsigned int THREAD_STOP;
        /** code representing the method that triggered an error in join */
        static const unsigned int THREAD_JOIN;
        /** code representing the method that triggered an error in start */
        static const unsigned int THREAD_START;

        /**
         * Constructor
         */
        Thread();

        /**
         * Destructor
         */
        virtual ~Thread();

        /**
         * The function that will be run in the thread
         */
        virtual void run() = 0;

        /**
         * Start the thread
         */
        void start() throw (Exception);

        /**
         * Stop the thread
         */
        void stop() throw (Exception);

        /**
         * Wait for the end of execution of the thread
         */
        void join() throw (Exception);

    private:
        /** The related pthread */
        pthread_t thread;
        /** 
         * If true, it means that the value of thread was created through the
         * appropriate function
         */
        bool threadCreated;

    };

}

#endif  /* THREAD_H */
