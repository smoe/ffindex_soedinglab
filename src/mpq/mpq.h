/*
 MPQ - MPi Queue

 Copyright (c) 2015 Milot Mirdita <milot@mirdita.de>
 All rights reserved.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.

 This library distributes jobs across MPI processes. One job packet contains 
 the start and end index of a complete task. The task is user defined function 
 in user space that matches the MPQ_Payload_t type.
 
 To setup a MPQ Queue use MPQ_Init.
 To tear it down again use MPQ_Finalize.
 
 MPQ_Main starts the main loop that either starts 
 a MPI master process or MPI worker processes.
 
 The worker processes call the function pointed to by MPQ_Payload.

 This library is multi threading unsafe. Only use from one main thread.
 */

#include <stddef.h>

#define MPQ_MASTER 0

extern int MPQ_rank;
extern int MPQ_size;

enum {
    MPQ_SUCCESS = 0,
    MPQ_ERROR_NO_WORKERS,
    MPQ_ERROR_TOO_MANY_WORKERS,
    MPQ_ERROR_REINIT,
    MPQ_ERROR_UNKNOWN
};

int MPQ_Init (int argc, char** argv, const size_t num_jobs);

void MPQ_Main (const size_t split_size);

void MPQ_Finalize ();

typedef int (*MPQ_Payload_t) (const size_t start, const size_t end);
MPQ_Payload_t MPQ_Payload;