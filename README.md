# scheduler
Simple Scheduler


example:

1. Create the scheduler with nThreads threads. The job should be a JobBase derived class/struct. The constructor kicks off the thread running.

    Scheduler<Job> scheduler(nThreads);

2. Get the queue from the scheduler.

    Queue<Job> &jobQ = scheduler.getQueue();

3. Populate the queue

    for (int i = 1; i <= nJobs; i++)
        jq.push(Job {i});
    jq.push(Job {-1});  //last job

4. If you want to wait for completion can do this. The statistics of each job run can be taken from scheduler.getCompletedJobs();
                            
    scheduler.wait();
