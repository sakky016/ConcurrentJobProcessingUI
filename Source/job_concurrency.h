#ifndef JOB_CONCURRENCY_H
#define JOB_CONCURRENCY_H

#include <string>
#include <QApplication>
#include <QTableWidget>
#include <QTime>
#include <list>
#include <Windows.h>

//---------------------------------------------------------------------------------
// Job Status
//---------------------------------------------------------------------------------
enum
{
    PENDING,
    RUNNING,
    COMPLETE
};

//---------------------------------------------------------------------------------
// Job structure
//---------------------------------------------------------------------------------
typedef struct Job_tag
{
    size_t jobId;

    // Job assigned to which worker thread
    int workerThreadId;

    // Job status
    QString jobStatus;

    // Job created, start and completion time
    QTime createTime;
    QTime startTime;
    QTime endTime;

    // Parameter to process for this job. This is just
    // for simulation. Actual params could be anything
    QString inputParam;

    // Job result stored here. This is just for simulation,
    // Actual result could be anything- string, int, custom object etc.
    unsigned int jobResult;
}Job_t;


//---------------------------------------------------------------------------------
// Structure storing information of its job execution details
//---------------------------------------------------------------------------------
typedef struct ThreadInfo_tag
{
    int workerThreadId;
    std::list<Job_t*> jobList;
    QTableWidget *threadTable;
    QLineEdit *status;
    QLineEdit *jobsCompleted;
    QLineEdit *workingTime;
    QLineEdit *idleTime;

}ThreadInfo_t;

Job_t* RetrieveJobFromWorkQueue();
void AddToWorkQueue(Job_t* job);
void ShowStatisticsThread();


#endif // JOB_CONCURRENCY_H
