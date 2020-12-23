/************************************************************
 *              TRANSFER OWNERSHIP OF THREADS               *
 ************************************************************/

#include <thread>
#include <iostream>
#include <chrono>

/*!
 * From https://en.cppreference.com/w/cpp/thread/thread/thread
 * 
 * Creates a new thread object which is not a thread
 *      - thread() noexcept;
 * No copy-constructor
 *      - thread( const thread& ) = delete;
 * Move constructor
 *      - thread( thread&& other ) noexcept;
 *      This is the one we are interested in to transfer
 *      ownership of a std::thread.
 */

void doTheJob(int a) { 
    for ( int i = 0; i< 5; i++ ) 
    {
        std::cout << std::this_thread::get_id() << " is doing the job... " << ++a << "\n";
        std::this_thread::sleep_for( std::chrono::milliseconds(5) );
    }
}

int main()
{
    int var{0};

    std::thread t1( doTheJob, var );
    std::cout << "t1 id is " << t1.get_id() << "\n";

    std::thread t2( std::move(t1)           ); // t2 is now running t1
    std::cout << "t2 id is " << t2.get_id() << " and t1 id is " << t1.get_id() << "\n";

    // t1 is not a thread anymore
    std::cout << "t1 is " << ( t1.joinable() ? "joinable\n" : "not joinable\n" );

    // Assign a new thread to t1
    // with implicit std::move call
    t1 = std::thread( doTheJob, var );

    std::cout << "t1 id is " << t1.get_id() << "\n";

    t1.join();                                 
    t2.join();

    return EXIT_SUCCESS;
}