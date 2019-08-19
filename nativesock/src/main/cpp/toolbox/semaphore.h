#ifndef TESTPROJ_SEMAPHORE_H
#define TESTPROJ_SEMAPHORE_H

#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>

class CSemaphore
{
private:
    std::condition_variable condition;
    std::mutex mutex;
    int value;

public:
    explicit CSemaphore(int init) : value(init)
    {}

    void wait()
    {
        std::unique_lock<std::mutex> lock(mutex);
        condition.wait(lock, [&]()
        { return value >= 1; });
        value--;
    }

    bool try_wait()
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (value < 1)
            return false;
        value--;
        return true;
    }

    void post()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            value++;
        }
        condition.notify_one();
    }
};

class TestCSemaphore
{
public:
    static void execute()
    {
        CSemaphore semaph(2);
        auto fn = [&](int rand_milli) -> void
        {
            semaph.wait();
            std::this_thread::sleep_for(std::chrono::milliseconds(rand_milli));
            std::cout << "thread:" << std::this_thread::get_id() << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            semaph.post();
        };
        std::thread t1(fn, 0);
        std::thread t2(fn, 100);
        std::thread t3(fn, 200);
        std::thread t4(fn, 300);
        std::thread t5([&]() -> void
                       {
                           while (!semaph.try_wait())
                           {
                               std::this_thread::sleep_for(std::chrono::milliseconds(200));
                               std::cout<<"wait while()"<<std::endl;
                           }
                           std::cout << "thread:" << std::this_thread::get_id() << std::endl;
                       });


        t1.join();
        t2.join();
        t3.join();
        t4.join();
        t5.join();
    }
};

#endif //TESTPROJ_SEMAPHORE_H
