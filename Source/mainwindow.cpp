#include <iostream>
#include <list>
#include "mainwindow.h"
#include <mutex>
#include <QMessageBox>
#include "ui_mainwindow.h"
#include "worker.h"


//---------------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------------
const int WORK_QUEUE_MONITOR_INTERVAL_MS = 500;
const QString JOB_STATUS[] = { "PENDING", "RUNNING", "COMPLETE" };

// Synchronization
static std::mutex g_workQueueMutex;
static std::mutex g_threadWorkMutex;
static std::mutex g_jobMutex;
static std::condition_variable jobAvailableConditionVar;



//---------------------------------------------------------------------------------
// MainWindow
//---------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Set focus to input parameter field
    ui->inpParam->setFocus();

    // Make sure when Return key is pressed, then value in input param is added
    // as a job to work queue
    connect(ui->inpParam, SIGNAL(returnPressed()), this, SLOT(on_btnAddJob_clicked()));

    // Work queue thread which continuosly monitors the list of available
    // jobs in the work queue.
    std::thread workQueueThread(&MainWindow::WorkQueueUpdate, this);
    workQueueThread.detach();

    // Start worker threads
    // Worker 1
    ThreadInfo_t* thread1_info = new ThreadInfo_t{};
    thread1_info->threadTable = ui->opTableJobs1;
    thread1_info->status = ui->opStatus1;
    thread1_info->jobsCompleted = ui->opJobsCompleted1;

    std::thread workerThread1(&MainWindow::WorkerThread, this, thread1_info, 1);
    m_threadInfo.push_back(thread1_info);
    workerThread1.detach();

    // Worker 2
    ThreadInfo_t* thread2_info = new ThreadInfo_t{};
    thread2_info->threadTable = ui->opTableJobs2;
    thread2_info->status = ui->opStatus2;
    thread2_info->jobsCompleted = ui->opJobsCompleted2;

    std::thread workerThread2(&MainWindow::WorkerThread, this, thread2_info, 2);
    m_threadInfo.push_back(thread2_info);
    workerThread2.detach();

    // Worker 3
    ThreadInfo_t* thread3_info = new ThreadInfo_t{};
    thread3_info->threadTable = ui->opTableJobs3;
    thread3_info->status = ui->opStatus3;
    thread3_info->jobsCompleted = ui->opJobsCompleted3;

    std::thread workerThread3(&MainWindow::WorkerThread, this, thread3_info, 3);
    m_threadInfo.push_back(thread3_info);
    workerThread3.detach();

    // Worker 4
    ThreadInfo_t* thread4_info = new ThreadInfo_t{};
    thread4_info->threadTable = ui->opTableJobs4;
    thread4_info->status = ui->opStatus4;
    thread4_info->jobsCompleted = ui->opJobsCompleted4;

    std::thread workerThread4(&MainWindow::WorkerThread, this, thread4_info, 4);
    m_threadInfo.push_back(thread4_info);
    workerThread4.detach();
}

MainWindow::~MainWindow()
{
    delete ui;
}

//---------------------------------------------------------------------------------
// @name                : WorkQueueUpdate
//
// @description         : This thread runs continuosly at predefined interval to
//                        monitor the job queue. Once it
//                        determines that there is a job waiting for execution, it
//                        notifies one of the waiting worker threads to process
//                        this job.
//---------------------------------------------------------------------------------
void MainWindow::WorkQueueUpdate()
{
    while (1 /* Run forever */)
    {
        {
            std::lock_guard<std::mutex> lck(g_workQueueMutex);
            auto queuedJobs = m_workQueue.size();
            if (queuedJobs > 0)
            {
                jobAvailableConditionVar.notify_one();
            }
        }

        UpdateTable(ui->opTableQueuedJobs, m_allJobs);
        Sleep(WORK_QUEUE_MONITOR_INTERVAL_MS);
    }
}

//---------------------------------------------------------------------------------
// @name                : WorkerThread
//
// @description         : Function for worker thread
//---------------------------------------------------------------------------------
void MainWindow::WorkerThread(ThreadInfo_t * threadInfo, int workerId)
{
    int threadId = workerId;
    threadInfo->workerThreadId = threadId;

    ui->statusBar->showMessage("Worker Thread #" + QString::number(threadId) + " started");
    threadInfo->status->setText("IDLE");

    time_t idleEnd = time(nullptr);

    while (1 /* Runs forever */)
    {
        time_t idleStart = time(nullptr);
        Job_t* job = RetrieveJobFromWorkQueue();
        if (job)
        {
            g_threadWorkMutex.lock();
            threadInfo->jobList.push_back(job);
            g_threadWorkMutex.unlock();

            QString msg = "Processing [" + job->inputParam + "] using Worker thread: " + QString::number(threadId);
            ui->statusBar->showMessage(msg);

            // Do work
            time_t workStart = time(nullptr);
            StartJob(job, threadInfo);
            time_t workEnd = time(nullptr);

            // Update thread info
            int jobsCompleted = threadInfo->jobsCompleted->text().toInt();
            threadInfo->jobsCompleted->setText(QString::number(jobsCompleted + 1));

            threadInfo->workingTime += workEnd - workStart;
        }
        else
        {
            idleEnd = time(nullptr);
            threadInfo->idleTime += idleEnd - idleStart;
        }

        // Update the table continuosly
        UpdateTable(threadInfo->threadTable, threadInfo->jobList);
    }
}


//---------------------------------------------------------------------------------
// @name                : RetrieveJobFromWorkQueue
//
// @description         : Fetches and then removes job from work queue. This thread
//                        waits till notified by the main thread that monitors
//                        arrival on jobs in work queue.
//---------------------------------------------------------------------------------
Job_t * MainWindow::RetrieveJobFromWorkQueue()
{
    Job_t* job = nullptr;
    std::unique_lock<std::mutex> lck(g_workQueueMutex);
    jobAvailableConditionVar.wait(lck);

    if (!m_workQueue.empty())
    {
        job = m_workQueue.front();
        m_workQueue.pop_front();
    }

    return job;
}

//---------------------------------------------------------------------------------
// @name                : StartJob
//
// @description         : Start execution of job using thread identified byworkerThreadId.
//---------------------------------------------------------------------------------
void MainWindow::StartJob(Job_t* job, ThreadInfo_t* workerThreadInfo)
{
    //g_jobMutex.lock();

    // Update job status
    job->jobStatus = JOB_STATUS[RUNNING];

    // Update work status of this thread
    workerThreadInfo->status->setText("RUNNING");

    job->startTime = QTime::currentTime();
    job->workerThreadId = workerThreadInfo->workerThreadId;
    //g_jobMutex.unlock();

    // Update the table for this worker thread
    UpdateTable(workerThreadInfo->threadTable, workerThreadInfo->jobList);

    int runningJobs = ui->opRunningJobs->text().toInt();
    ui->opRunningJobs->setText(QString::number(runningJobs + 1));

    // Do some time consuming operation on this job.
    for (auto i = 0; i < job->inputParam.size(); i++)
    {
        job->jobResult += 10;
        Sleep(rand() % 1000);
    }

    // Update job status
    job->jobStatus = JOB_STATUS[COMPLETE];

    // Update work status of this thread
    workerThreadInfo->status->setText("IDLE");

    job->endTime = QTime::currentTime();

    // Update the table for this worker thread
    UpdateTable(workerThreadInfo->threadTable, workerThreadInfo->jobList);

    // Increment completed jobs
    int completedJobs = ui->opCompletedJobs->text().toInt();
    ui->opCompletedJobs->setText(QString::number(completedJobs + 1));

    // Decrement running jobs
    runningJobs = ui->opRunningJobs->text().toInt();
    ui->opRunningJobs->setText(QString::number(runningJobs - 1));
}

//---------------------------------------------------------------------------------
// @name                : UpdateTable
//
// @description         : Updates the specified table widget as per jobs assigned to it.
//---------------------------------------------------------------------------------
void MainWindow::UpdateTable(QTableWidget *table, std::list<Job_t*> jobList)
{
    table->clearContents();

    table->setRowCount(jobList.size());
    table->setColumnCount(COLUMN_MAX);

    size_t rowIndex = 0;
    for (auto it = jobList.begin(); it != jobList.end(); it++)
    {
        Job_t *job = *it;
        AddJobToTable(job, table, rowIndex);
        rowIndex++;
    }
}

//---------------------------------------------------------------------------------
// @name                : AddJobToTable
//
// @description         : Adds a job to the Table widget for display.
//---------------------------------------------------------------------------------
void MainWindow::AddJobToTable(Job_t *job, QTableWidget *table, size_t rowIndex)
{
    // Job ID
    QTableWidgetItem *jobIdItem = new QTableWidgetItem();
    jobIdItem->setFlags(jobIdItem->flags() ^ Qt::ItemIsEditable);
    jobIdItem->setText(QString::number(job->jobId));
    table->setItem(static_cast<int>(rowIndex), COLUMN_JOB_ID, jobIdItem);

    // Assigned worker
    QTableWidgetItem *assignedWorkerItem = new QTableWidgetItem();
    assignedWorkerItem->setFlags(assignedWorkerItem->flags() ^ Qt::ItemIsEditable);
    QString assignedWorkerId = "-";
    if (job->jobStatus != JOB_STATUS[PENDING])
    {
        assignedWorkerId = QString::number(job->workerThreadId);
    }
    assignedWorkerItem->setText(assignedWorkerId);
    table->setItem(static_cast<int>(rowIndex), COLUMN_ASSIGNED_WORKER, assignedWorkerItem);

    // Job status
    QTableWidgetItem *jobStatusItem = new QTableWidgetItem();
    jobStatusItem->setFlags(jobStatusItem->flags() ^ Qt::ItemIsEditable);
    jobStatusItem->setText(job->jobStatus);
    table->setItem(static_cast<int>(rowIndex), COLUMN_JOB_STATUS, jobStatusItem);

    // Create Time
    QTableWidgetItem *createTimeItem = new QTableWidgetItem();
    createTimeItem->setFlags(createTimeItem->flags() ^ Qt::ItemIsEditable);
    QString cTime = job->createTime.isNull() ? " - " : job->createTime.toString("HH:mm:ss.zzz");
    createTimeItem->setText(cTime);
    table->setItem(static_cast<int>(rowIndex), COLUMN_CREATE_TIME, createTimeItem);

    // Start Time
    QTableWidgetItem *startTimeItem = new QTableWidgetItem();
    startTimeItem->setFlags(startTimeItem->flags() ^ Qt::ItemIsEditable);
    QString sTime = job->startTime.isNull() ? " - " : job->startTime.toString("HH:mm:ss.zzz");
    startTimeItem->setText(sTime);
    table->setItem(static_cast<int>(rowIndex), COLUMN_START_TIME, startTimeItem);

    // Complete Time
    QTableWidgetItem *completeTimeItem = new QTableWidgetItem();
    completeTimeItem->setFlags(completeTimeItem->flags() ^ Qt::ItemIsEditable);
    QString eTime = job->endTime.isNull() ? " - " : job->endTime.toString("HH:mm:ss.zzz");
    completeTimeItem->setText(eTime);
    table->setItem(static_cast<int>(rowIndex), COLUMN_COMPLETE_TIME, completeTimeItem);

    // Input parameter
    QTableWidgetItem *inputParamItem = new QTableWidgetItem();
    inputParamItem->setFlags(inputParamItem->flags() ^ Qt::ItemIsEditable);
    inputParamItem->setText(job->inputParam);
    table->setItem(static_cast<int>(rowIndex), COLUMN_INPUT_PARAM, inputParamItem);

    // Result
    QTableWidgetItem *resultItem = new QTableWidgetItem();
    resultItem->setFlags(resultItem->flags() ^ Qt::ItemIsEditable);
    QString res = "-";
    if (job->jobStatus == JOB_STATUS[COMPLETE])
    {
        res = QString::number(job->jobResult);
    }
    resultItem->setText(res);
    table->setItem(static_cast<int>(rowIndex), COLUMN_RESULT, resultItem);
}

//---------------------------------------------------------------------------------
// @name                : AddToWorkQueue
//
// @description         : Adds a job to the work queue and update the job table
//---------------------------------------------------------------------------------
void MainWindow::AddToWorkQueue(Job_t* job)
{
    g_workQueueMutex.lock();
    m_workQueue.push_back(job);
    auto queuedJobs = m_workQueue.size();
    g_workQueueMutex.unlock();

    // Update table jobs table
    UpdateTable(ui->opTableQueuedJobs, m_allJobs);

    ui->opTotalJobsAdded->setText(QString::number(m_allJobs.size()));
    ui->opPendingJobs->setText(QString::number(queuedJobs));

    QString msg = "[Job #" + QString::number(job->jobId) + "] added to work queue. Queued jobs: " + QString::number(queuedJobs);
    ui->statusBar->showMessage(msg);
}

//---------------------------------------------------------------------------------
// @name                : CreateJob
//
// @description         : Creates a new job with the specified user parameter.
//                        Specifies
//---------------------------------------------------------------------------------
Job_t* MainWindow::CreateJob(const QString & param)
{
    Job_t* job = nullptr;
    if (param.size())
    {
        job = new Job_t{};
        job->jobId = m_allJobs.size() + 1;
        job->createTime = QTime::currentTime();
        job->jobStatus = JOB_STATUS[PENDING];
        job->inputParam = param;

        // Add this to the list of all jobs
        g_jobMutex.lock();
        m_allJobs.push_back(job);
        g_jobMutex.unlock();
    }
    else
    {
        ui->statusBar->showMessage("Input parameter empty.");
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setWindowTitle("Input parameter empty");
        msgBox.setText("Nothing specified in input parameter. Specify a value to create a valid job.");
        msgBox.exec();
    }

    return job;
}

//---------------------------------------------------------------------------------
// Button Clicked: AddJob
//---------------------------------------------------------------------------------
void MainWindow::on_btnAddJob_clicked()
{
    QString userInput = ui->inpParam->text();
    Job_t *job = CreateJob(userInput);
    if (job)
    {
        AddToWorkQueue(job);
    }
}
