/************************************************************
 *            SOME USEFUL FUNCTIONS ON THREADS              *
 ************************************************************/

#include <thread>
#include <iostream>
#include <chrono>

/*!
 * More infos on https://en.cppreference.com/w/cpp/thread/thread/thread
 */

void doTheJob() { 
    std::cout << "Thread " << std::this_thread::get_id() << " is doing the job...\n";
}

void sleepForExample() {
    static uint32_t sleepTime{2000};

    std::cout << "The current thread is gonna sleep for "
              << sleepTime << "ms...";

    std::this_thread::sleep_for( std::chrono::milliseconds(sleepTime) );

    std::cout << "Done !\n";
}

// "busy sleep" while suggesting that other threads run
// for a small amount of time
void little_sleep(std::chrono::microseconds us)
{
    auto start = std::chrono::high_resolution_clock::now();
    auto end   = start + us;
    do
    {
        std::this_thread::yield();
    } while ( std::chrono::high_resolution_clock::now() < end );
}

int main()
{
    /*!
     * ---------------------------------------------
     *  Get the ID of a thread 
     * ---------------------------------------------
     * As a public member function
     *      std::thread::id get_id() const noexcept;
     * As a function
     *      std::thread::id get_id() noexcept;
     */
    std::thread t1;
    std::cout << "t1 is not a thread so its id is " << t1.get_id() << "\n";
    
    std::thread t2( doTheJob );
    std::cout << "t2 id is " << t2.get_id() << "\n";
    t2.join();

    /*!
     * ---------------------------------------------
     * Get the number of concurrent threads 
     * supported by th implementation
     * ---------------------------------------------
     *
     *      static unsigned int hardware_concurrency() noexcept;   
     */
    std::cout << std::thread::hardware_concurrency() 
              << " concurrent threads are supported.\n";

    /*!
     * ---------------------------------------------
     * Reschedule the execution of a thread,
     * allowing other threads to run.
     * ---------------------------------------------
     *
     *      void yield() noexcept;
     */
    auto start = std::chrono::high_resolution_clock::now();

    little_sleep(std::chrono::microseconds(100));
    
    auto elapsed = std::chrono::high_resolution_clock::now() - start;
    std::cout << "waited for "
              << std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count()
              << " microseconds\n";

    /*!
     * ---------------------------------------------
     * Block the execution of a thread for at least
     * the specified amount of time specified.
     * ---------------------------------------------
     * 
     *      template< class Rep, class Period >
     *      void sleep_for( const std::chrono::duration<Rep, Period>& sleep_duration );
     */
    t1 = std::thread(sleepForExample); 
    t1.join();

    return EXIT_SUCCESS;
}