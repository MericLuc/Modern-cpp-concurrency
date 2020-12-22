/*!
 * A RAII thread wrapper to provide safe threads
 * Inspired from Tom Scott lecture available here
 * https://channel9.msdn.com/Events/GoingNative/2013/An-Effective-Cpp11-14-Sampler
 * 
 * Here is also a very interesting discussion on the problem
 * and possible solutions :
 * https://isocpp.org/files/papers/p0206r0.html 
 */

#include <iostream>
#include <utility>
#include <thread>

class ThreadWrapper {
public:
    typedef void (std::thread::*RAIIAction)();

    explicit ThreadWrapper(std::thread&& p_thread, RAIIAction p_action) : 
        m_thread(std::move(p_thread)),
        m_action(p_action) 
        {}
    
    // No copy or assignement allowed
    ThreadWrapper(const ThreadWrapper& ) = delete;
    ThreadWrapper& operator=(const ThreadWrapper& ) = delete;

    // We can provide a getter for the underlying std::thread in order
    // to work with it.
    std::thread& get(void) { return m_thread; }

    // RAII - The destructor calls for joinable or detach
    // if the std::thread is joinable so that the program
    // is safe.
    ~ThreadWrapper() { if ( m_thread.joinable() ) (m_thread.*m_action)(); }

private : 
    std::thread m_thread;
    RAIIAction  m_action;
};

void doTheJob(void) { std::cout << "Doing the job...\n"; }

void doTheBrokenJob(void) { 
    std::cout << "Doing a broken job...\n"; 
    throw std::runtime_error("A runtime error");
}

#define T1 1 // perform test1
#define T2 0 // perform test2
#define T3 0 // perform test3

int main()
{
    if ( T1 )
    {
        // This thread will call join on destruction
        // OK !
        std::cout << "Test 1\n";
        ThreadWrapper t( std::thread( doTheJob ), &std::thread::join );
    }

    if ( T2 )
    {
        // This thread will enter its destructor while
        // joinable, resulting in an unsafe program ( calling 
        // std::terminate() )
        std::cout << "Test 2\n";
        std::thread t( doTheJob );
    }

    if ( T3 )
    {
        // This thread will never be able to call
        // joinable or detach because of the exception
        // in the doTheBrokenJob() function.
        // It will therefore also result in an unsage program.
        std::cout << "Test 3\n";
        std::thread t( doTheBrokenJob );
        t.join();
    }

    std::cout << "Done !\n";

    return EXIT_SUCCESS;
}