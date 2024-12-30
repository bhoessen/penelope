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

#ifndef EXCEPTION_H
#define	EXCEPTION_H

namespace penelope {

    /**
     * This class represent an exception that occurred during the use of threads
     */
    class Exception {
    public:

        /** The maximum length authorized for the exception messages */
        static const unsigned int MSG_MAX_LENGTH;

        /**
         * Create a new thread related exception
         * @param anErrorCode the error code generated
         * @param anOrigin a code representing the function that triggered the 
         *        exception
         * @param aMsg the message related to this exception
         */
        Exception(int anErrorCode, unsigned int anOrigin, const char* aMsg = "");
        
        /**
         * Copy constructor
         * @param other the source of the copy
         */
        Exception(const Exception& other);
        
        /**
         * Copy assignment operator
         * @param other the source of the copy
         * @return the destination of the copy
         */
        Exception& operator=(const Exception& other);

        /**
         * Retrieve the error code of this exception
         * @return the error code of this exception
         */
        int getErrorCode() const {
            return errorCode;
        }

        /**
         * Retrieve a code representing the origin function
         * @return the origin code
         */
        unsigned int getOrigin() const {
            return origin;
        }
        
        /**
         * Retrieve the message related to this exception
         * @return the exception message
         */
        const char* getMessage() const{
            return msg;
        }

    private:
        /** The error code returned by the thread management system */
        int errorCode;
        /** The function that triggered the exception */
        unsigned int origin;
        /** A message */
        char msg[512];
    };
}

#endif	/* EXCEPTION_H */

