
/* Copyright (c) 2005, Stefan Eilemann <eile@equalizergraphics.com> 
   All rights reserved. */

#ifndef EQBASE_THREAD_H
#define EQBASE_THREAD_H

#include "base.h"

namespace eqBase
{
    /**
     * An abstraction to create a new execution thread.
     * 
     * Depending on the type, a different implementation is used to create the
     * thread. Please note that certain implementations, e.g., fork, create the
     * working unit in another address space. 
     */
    class Thread {

    public:
        /** The thread types. */
        enum Type
        {
            PTHREAD,
            FORK
        };

        Thread( const Type type );
        virtual ~Thread(){}

        /** 
         * Starts the thread using the specified entry function.
         * 
         * @return <code>true</code> if the thread was launched,
         *         <code>false</code> otherwise.
         */
        bool start();

        /** 
         * The entry function for the child thread.
         * 
         * @return the return value of the child thread.
         */
        virtual int run() = 0;

        /** 
         * Waits for the exit of the child thread.
         * 
         * The actual size of the return value is thread-type dependent and may
         * be as low as 8 bits.
         *
         * @param retVal output value for the return value of the child, can be
         *               <code>NULL</code>.
         * @return <code>true</code> if the thread was joined,
         *         <code>false</code> otherwise.
         */
        bool join( size_t* retVal=NULL );

    private:
        /** The current state of this thread. */
        enum State
        {
            STATE_STOPPED,
            STATE_STARTING, // start() in progress
            STATE_RUNNING,
            STATE_STOPPING  // child no longer active, join() not yet called
        };

        Type  _type;
        State _threadState;
        
        union ThreadID
        {
            pthread_t pthread;
            pid_t     fork;
        } _threadID;

        static void* runChild( void* arg );
        void        _runChild();

        ThreadID _getLocalThreadID();
    };
}

#endif //EQBASE_THREAD_H
