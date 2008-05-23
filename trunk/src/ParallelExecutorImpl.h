#ifndef SimTK_SimTKCOMMON_PARALLEL_EXECUTOR_IMPL_H_
#define SimTK_SimTKCOMMON_PARALLEL_EXECUTOR_IMPL_H_

/* -------------------------------------------------------------------------- *
 *                      SimTK Core: SimTK Simbody(tm)                         *
 * -------------------------------------------------------------------------- *
 * This is part of the SimTK Core biosimulation toolkit originating from      *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org.               *
 *                                                                            *
 * Portions copyright (c) 2008 Stanford University and the Authors.           *
 * Authors: Peter Eastman                                                     *
 * Contributors:                                                              *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,    *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR      *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE  *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                     *
 * -------------------------------------------------------------------------- */

#include "SimTKcommon/internal/ParallelExecutor.h"
#include <pthread.h>
#include <vector>

namespace SimTK {

class ParallelExecutorImpl;

/**
 * This class stores per-thread information used while executing a task.
 */

class ThreadInfo {
public:
    ThreadInfo(int index, ParallelExecutorImpl* executor) : index(index), executor(executor), running(false) {
    }
    const int index;
    ParallelExecutorImpl* const executor;
    bool running;
};

/**
 * This is the internal implementation class for ParallelExecutor.
 */

class ParallelExecutorImpl : public PIMPLImplementation<ParallelExecutor, ParallelExecutorImpl> {
public:
    ParallelExecutorImpl(int numThreads);
    ~ParallelExecutorImpl();
    ParallelExecutorImpl* clone() const;
    void execute(ParallelExecutor::Task& task, int times);
    int getThreadCount() {
        return threads.size();
    }
    ParallelExecutor::Task& getCurrentTask() {
        return *currentTask;
    }
    int getCurrentTaskCount() {
        return currentTaskCount;
    }
    bool isFinished() {
        return finished;
    }
    pthread_mutex_t* getLock() {
        return &runLock;
    }
    pthread_cond_t* getCondition() {
        return &runCondition;
    }
    void incrementWaitingThreads();
private:
    bool finished;
    pthread_mutex_t runLock, waitLock;
    pthread_cond_t runCondition, waitCondition;
    std::vector<pthread_t> threads;
    std::vector<ThreadInfo*> threadInfo;
    ParallelExecutor::Task* currentTask;
    int currentTaskCount;
    int waitingThreadCount;
};

} // namespace SimTK

#endif // SimTK_SimTKCOMMON_PARALLEL_EXECUTOR_IMPL_H_
