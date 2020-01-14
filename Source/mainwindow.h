#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "job_concurrency.h"
#include <QMainWindow>
#include <QTableWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

//---------------------------------------------------------------------------------
// MainWindow class
//---------------------------------------------------------------------------------
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    Job_t* CreateJob(const QString & param);
    void AddToWorkQueue(Job_t* job);
    void UpdateTable(QTableWidget *table, std::list<Job_t*> jobList);
    void AddJobToTable(Job_t *job, QTableWidget *table, size_t rowIndex);

    void WorkQueueUpdate();
    void WorkerThread(ThreadInfo_t * threadInfo, int workerId);
    void StartJob(Job_t* job, ThreadInfo_t* workerThreadInfo);
    Job_t * RetrieveJobFromWorkQueue();

private slots:
    void on_btnAddJob_clicked();

private:
    Ui::MainWindow *ui;

    // Job queues
    std::list<Job_t*> m_workQueue;
    std::list<Job_t*> m_allJobs;

    std::list<ThreadInfo_t*> m_threadInfo;
};

//---------------------------------------------------------------------------------
// Functions
//---------------------------------------------------------------------------------


//---------------------------------------------------------------------------------
// Table column indices
//---------------------------------------------------------------------------------
enum
{
    COLUMN_JOB_ID = 0,
    COLUMN_ASSIGNED_WORKER,
    COLUMN_JOB_STATUS,
    COLUMN_CREATE_TIME,
    COLUMN_START_TIME,
    COLUMN_COMPLETE_TIME,
    COLUMN_INPUT_PARAM,
    COLUMN_RESULT,
    COLUMN_MAX
};
#endif // MAINWINDOW_H
