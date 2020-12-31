/************************************************************
 *     ENSURE SINGLE EXECUTION IN MULTITHREADED CONTEXT     *
 ************************************************************/

/*!
 * @brief std::call_one allows to execute a callable object
 *        exactly once, even in multithreaded contexts,
 *        when called from different threads.
 * 
 * See https://en.cppreference.com/w/cpp/thread/call_once
 * for more details.
 */

/*!
 * @note This is a C++11 feature.
 */

/*!
 * @note When to use ?
 *       - To perform an initialisation task
 *         that has to happen before any thread
 *         to be able to work but should not be repeated.
 */

#include <iostream>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>
#include <vector>

/*!
 * @brief coutWrapper
 *        Thread-safe wrapper to ensure std::cout
 *        coherence in multithreaded context.
 */
class coutWrapper : public std::stringstream {
public:
    coutWrapper() = default;
    ~coutWrapper() {
        std::lock_guard<std::mutex> l_lck(m_mtx);
        std::cout << std::stringstream::rdbuf();
        std::cout.flush();
    }

private:
    static inline std::mutex m_mtx;
};

void init_once( std::once_flag& p_flag ) {
    // Every thread can read there
    coutWrapper{} << "T" << std::this_thread::get_id() << ": in init_once()\n";

    // But only one will call this
    std::call_once( p_flag, 
            [](){ coutWrapper{} << "Init step called only once by T"
                                << std::this_thread::get_id() <<"\n"; } );
}

void thread_work( std::once_flag& p_flag ) {
    coutWrapper{} << "T" << std::this_thread::get_id() << " work\n";
    
    // Perform initialisation once
    init_once( p_flag );
    
    // Do the work
    // ...
}

#define THREADS_NB 5
int main()
{
    std::once_flag flag;

    std::vector<std::thread> myThreads;
    for ( int i = 0; i < THREADS_NB; i++ )
    {
        myThreads.emplace_back( std::thread(thread_work, std::ref(flag)) );
    }

    for ( auto& t : myThreads ) { t.join(); }

    coutWrapper{} << "The End.\n";

    return EXIT_SUCCESS;
}