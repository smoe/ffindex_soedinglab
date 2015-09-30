#include "mpq.h"
#include <mpi.h>

#include <stdlib.h>
#include <sys/queue.h>

int MPQ_size;
int MPQ_rank;
int MPQ_num_jobs;
int MPQ_split_size;

enum {
    TAG_JOB,
    TAG_RESULT
};

enum {
    MSG_RELEASE,
    MSG_JOB,
    MSG_RESULT
};

int MPQ_Init (int argc, char** argv, const size_t num_jobs, const size_t split_size)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &MPQ_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &MPQ_size);

    MPQ_num_jobs = num_jobs;
    MPQ_split_size = split_size;

    int workers = MPQ_size - 1;
    if (workers < 1) {
        MPI_Finalize();
        return MPQ_ERROR_NO_WORKERS;
    }

    if (num_jobs < workers) {
        MPI_Finalize();
        return MPQ_ERROR_TOO_MANY_WORKERS;
    }

    return MPQ_SUCCESS;
}

void MPQ_Worker ()
{
    int message[3];
    while (1) {
        MPI_Recv(message, 3, MPI_INT, MPQ_MASTER, TAG_JOB, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (message[0] == MSG_RELEASE) {
            break;
        }

        int exit_status = MPQ_Payload(message[1], message[2]);

        message[0] = MSG_RESULT;
        message[1] = exit_status;

        MPI_Send(message, 2, MPI_INT, MPQ_MASTER, TAG_RESULT, MPI_COMM_WORLD);
    }
}

void MPQ_Master ()
{
    typedef struct idle_wokers_s idle_workers_t;

    struct idle_wokers_s {
        int rank;
        STAILQ_ENTRY(idle_wokers_s) entries;
    };

    STAILQ_HEAD(idle_workers, idle_wokers_s) idle_workers_head;
    STAILQ_INIT(&idle_workers_head);

    idle_workers_t* data = NULL;
    for (int i = 1; i < MPQ_size; i++)
    {
        data = malloc(sizeof(idle_workers_t));
        data->rank = i;
        STAILQ_INSERT_HEAD(&idle_workers_head, data, entries);
    }

    MPI_Status status;
    int message[3];

    size_t remaining_jobs = MPQ_num_jobs;
    size_t start_job = 0;

    while (remaining_jobs > 0 || !STAILQ_EMPTY(&idle_workers_head)) {
        if (remaining_jobs > 0 && !STAILQ_EMPTY(&idle_workers_head)) {
            size_t batch_size = (remaining_jobs < MPQ_split_size) ? remaining_jobs : MPQ_split_size;

            message[0] = MSG_JOB;
            message[1] = start_job;
            message[2] = start_job + batch_size;

            start_job += batch_size;
            remaining_jobs -= batch_size;

            idle_workers_t* idle_worker = STAILQ_FIRST(&idle_workers_head);
            MPI_Send(message, 3, MPI_INT, idle_worker->rank, TAG_JOB, MPI_COMM_WORLD);

            STAILQ_REMOVE_HEAD(&idle_workers_head, entries);
            free(idle_worker);
        } else {
            MPI_Recv(message, 2, MPI_INT, MPI_ANY_SOURCE, TAG_RESULT, MPI_COMM_WORLD, &status);
            switch (message[0]) {
                default:
                case MSG_RESULT: {
                    // TODO: Do something with the job status in message[1]
                    idle_workers_t* idle_worker = malloc(sizeof(idle_workers_t));
                    idle_worker->rank = status.MPI_SOURCE;
                    STAILQ_INSERT_HEAD(&idle_workers_head, idle_worker, entries);
                    break;
                }
            }
        }
    }

    while (!STAILQ_EMPTY(&idle_workers_head)) {
        data = STAILQ_FIRST(&idle_workers_head);
        STAILQ_REMOVE_HEAD(&idle_workers_head, entries);
        free(data);
    }

}

void MPQ_Main ()
{
    if (MPQ_rank != MPQ_MASTER) {
        MPQ_Worker();
        return;
    }

    MPQ_Master();
}

void MPQ_Release_Worker (const int worker_rank)
{
    int message[3];
    message[0] = MSG_RELEASE;
    message[1] = 0;
    message[2] = 0;
    MPI_Send(message, 3, MPI_INT, worker_rank, TAG_JOB, MPI_COMM_WORLD);
}

void MPQ_Finalize ()
{
    if (MPQ_rank == MPQ_MASTER)
    {
        for (int i = 1; i < MPQ_size; i++)
        {
            MPQ_Release_Worker(i);
        }
    }

    MPI_Finalize();
    MPQ_Payload = NULL;
}
