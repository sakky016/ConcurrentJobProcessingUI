#ifndef WORKER_H
#define WORKER_H

#include "job_concurrency.h"
#include <Qthread>

//-----------------------------------------------------
// NOT USED CURRENTLY
//-----------------------------------------------------

//-----------------------------------------------------
// Worker thread
//-----------------------------------------------------
class Worker : public QThread
{
    Q_OBJECT

private:
    Job_t *m_job;

public:
    Worker(Job_t *job) : QThread()
    {
        m_job = job;
    }

    void run() override
    {
        emit JobComplete(m_job);
    }

signals:
    void JobComplete(Job_t* job);
};


#endif // WORKER_H
