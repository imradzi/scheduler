#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <chrono>
#include <memory>
#include <iomanip>
#include <random>
using namespace std::chrono_literals;
namespace Random {
    template<typename T>
    class Picker {
        std::random_device rd;
        std::mt19937 gen;
        T rangeMin, rangeMax;

    public:
        Picker(T min, T max) : gen(rd()),
                               rangeMin(min),
                               rangeMax(max) {}
        T GetNextRandom() {
            std::uniform_int_distribution<T> r(rangeMin, rangeMax);
            return r(gen);
        }
    };
}

static std::mutex lock_cout;  // so that only one thread can use std::cout at one time
void printLine(const std::string &s) {
    std::lock_guard<std::mutex> __guard(lock_cout);
    std::cout << std::this_thread::get_id() << "> " << s << std::endl;
}

template<typename T>
class Queue {
    std::queue<std::shared_ptr<T>> queue;
    std::mutex _mtx;

public:
    Queue() {}
    void push(const T &j) {
        const std::lock_guard<std::mutex> _guard(_mtx);
        queue.emplace(std::make_shared<T>(j))->setQueueTime(std::chrono::steady_clock::now());
    }
    bool empty() {
        const std::lock_guard<std::mutex> _guard(_mtx);
        return queue.empty();
    }
    std::shared_ptr<T> pop() {
        const std::lock_guard<std::mutex> _guard(_mtx);
        if (queue.empty()) return nullptr;
        auto j = queue.front();
        queue.pop();
        return j;
    }
};

template<typename T>
class Scheduler {
    Queue<T> jobQueue;
    std::mutex mtxJC;
    std::vector<std::shared_ptr<T>> jobCompleted;
    std::atomic<bool> endOfJob {false};
    std::vector<std::thread> threadList;
    void workerThread() {
        while (!endOfJob) {
            auto j = jobQueue.pop();
            if (j != nullptr) {
                if (j->isLastJob())
                    endOfJob = true;
                else {
                    j->process();
                    mtxJC.lock();
                    jobCompleted.emplace_back(j);
                    mtxJC.unlock();
                }
            }
        }
    }

public:
    Scheduler(int nThreads) {
        for (int i = 0; i < nThreads; i++)
            threadList.emplace_back(&Scheduler::workerThread, this);
    }
    ~Scheduler() { wait(); }
    void wait() {
        for (auto &x : threadList) {
            x.join();
        }
        threadList.clear();
    }
    Queue<T> &getQueue() { return jobQueue; }
    auto &getCompletedJobs() { return jobCompleted; }
};

struct JobBase {
    std::chrono::time_point<std::chrono::steady_clock> createTime {std::chrono::steady_clock::now()};
    std::chrono::time_point<std::chrono::steady_clock> queueTime {std::chrono::steady_clock::now()};
    std::chrono::time_point<std::chrono::steady_clock> processStartTime {std::chrono::steady_clock::now()};
    std::chrono::time_point<std::chrono::steady_clock> processEndTime {std::chrono::steady_clock::now()};

public:
    JobBase() {
        createTime = std::chrono::steady_clock::now();
    }
    JobBase(bool) {
        createTime = std::chrono::steady_clock::now();
    }
    void setQueueTime(std::chrono::time_point<std::chrono::steady_clock> time) {
        queueTime = time;
    }
    auto getElapsed() const {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(processEndTime - createTime).count();
    }
    void process() {
        if (isEmpty()) return;
        processStartTime = std::chrono::steady_clock::now();
        doActualProcess();
        processEndTime = std::chrono::steady_clock::now();
        printLine(std::to_string(getId()) + " processed ");
    }

public:  // these three need to be overridden by derived class to change the behaviour.
    virtual bool isLastJob() = 0;
    virtual bool isEmpty() = 0;
    virtual void doActualProcess() = 0;
    virtual int64_t getId() = 0;
};

struct Job : public JobBase {
    int id {-1};
    static Random::Picker<int> rand;

public:
    Job(int i) : id(i),
                 JobBase(true) {}
    bool isLastJob() override { return id == -1; }
    int64_t getId() override { return id; }
    bool isEmpty() override { return id <= 0; }
    void doActualProcess() override {
        auto dur = rand.GetNextRandom();                              //rand.GetNextRandom();
        std::this_thread::sleep_for(std::chrono::milliseconds(dur));  // fictitously set to 500ms;
    }
};

Random::Picker<int> Job::rand(300, 900);

int main(int argc, char *argv[]) {
    int nThreads = 4;
    int nJobs = 100;
    if (argc > 1) nThreads = std::stoi(argv[1]);
    if (argc > 2) nJobs = std::stoi(argv[2]);

    Random::Picker<int> rand(100, 500);
    Scheduler<Job> scheduler(nThreads);
    Queue<Job> &jq = scheduler.getQueue();
    //simulating jobs on queue.
    for (int i = 1; i <= nJobs; i++) {
        auto sleep = rand.GetNextRandom();
        printLine("sleeping for " + std::to_string(sleep));
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
        jq.push(Job {i});
    }
    jq.push(Job {-1});  //last job

    scheduler.wait();

    double total = 0;
    double min = std::numeric_limits<double>::max();
    double max = 0;
    auto jobList = scheduler.getCompletedJobs();
    int n = jobList.size();
    printLine("No of jobs completed = " + std::to_string(n));
    for (auto &jc : jobList) {
        double dur = jc->getElapsed();
        lock_cout.lock();
        std::cout << std::setiosflags(std::ios::fixed) << std::setprecision(0) << "Job " << jc->getId() << " take " << dur / 1000 << " μs" << std::endl;
        lock_cout.unlock();
        total += dur;
        if (dur < min) min = dur;
        if (dur > max) max = dur;
    }
    lock_cout.lock();
    std::cout << "Min = " << std::setiosflags(std::ios::fixed) << std::setprecision(0) << std::setw(9) << min / 1000 << " μs" << std::endl;
    std::cout << "max = " << std::setiosflags(std::ios::fixed) << std::setprecision(0) << std::setw(9) << max / 1000 << " μs" << std::endl;
    std::cout << "avg = " << std::setiosflags(std::ios::fixed) << std::setprecision(0) << std::setw(9) << (total / n) / 1000 << " μs" << std::endl;
    std::cout << std::endl;
    lock_cout.unlock();
}
