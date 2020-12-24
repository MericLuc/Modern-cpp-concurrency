/************************************************************
 *            BASIC COMMUNICATION BETWEEN THREADS           *
 ************************************************************/

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

/*!
 * @brief We will use condition variables
 *        to perform basic communication
 *        between threads.
 * 
 * @note  A condition variable is an object able to block 
 *        the calling thread until notified to resume.
 * 
 * More infos here
 * http://www.cplusplus.com/reference/condition_variable/condition_variable/
 */

std::mutex              mtx;
std::condition_variable cv;
std::string             data;
bool                    isReady{false};
bool                    isDone {false};

void workImpl()
{
    std::unique_lock<std::mutex> lck(mtx);
    cv.wait( lck, []{ return isReady; } );

    std::cout << "Producer thread is processing data\n";

    // Do your work
    // ...
    std::this_thread::sleep_for( std::chrono::milliseconds(2000) );
    data   = "Processing result";
    isDone = true;

    std::cout << "Producer thread completed!\n";

    lck.unlock();
    cv.notify_one();
}

void aFunction()
{
    std::cout << "Consumer thread waiting for producer to complete its task...\n";
    {
        std::unique_lock<std::mutex> lck(mtx);
        cv.wait( lck, []{ return isDone; } );
    }
    std::cout << "Consumer thread knows producer is done\n";
    // Do anything you want
    // ...
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    std::cout << "Consumer thread done!\n";
}

int main()
{
    std::thread producerThread( workImpl  );
    std::thread consumerThread( aFunction );

    {
        std::lock_guard<std::mutex> lck(mtx);
        isReady = true;

        std::cout << "Main sending signal to producer\n";
    }
    cv.notify_one();

    producerThread.join();
    consumerThread.join();

    return EXIT_SUCCESS;
}

/*
 * Main sending signal to producer
 * Consumer thread waiting for producer to complete its task...
 * Producer thread is processing data
 * Producer thread completed!
 * Consumer thread knows producer is done
 * Consumer thread done!
*/