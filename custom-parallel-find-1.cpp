/************************************************************
 *          PARALLEL FIND ALGORITHM IMPLEMENTATION          *
 *                 USING STD::PACKAGED_TASK                 *
 ************************************************************/

/*!
 * @brief std::packaged_task is a class template that allows you
 *        to wrap and asynchronously call any callable object. 
 *        You will get the callable object's return value
 *        as a std:future.
 * 
 * See https://en.cppreference.com/w/cpp/thread/packaged_task
 * for more details.
 */

/*!
 * @note We are using std::packaged_task to perform a custom
 *       parallel implementation of the std::find that allows
 *       to perform a search in a given range.
 * 
 * See http://www.cplusplus.com/reference/algorithm/find/
 * for more details about std::find
 * 
 * Please note that C++17 provides its own implementation
 * of parallel algorithms that should of course be used 
 * instead of this basic example.
 * 
 * See https://en.cppreference.com/w/cpp/algorithm/find 
 * for more details.
 */

#include <iostream>
#include <iomanip>
#include <string>
#include <future>
#include <iterator>
#include <utility>
#include <vector>
#include <functional>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <random>

//////////////////////////////////////////////////////////////////////////////////////////
/*!
 * @brief coutWrapper
 *        Thread-safe cout wrapper.
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

//////////////////////////////////////////////////////////////////////////////////////////
/*
 * @brief A stopwatch class to perform measures
 */
template < typename Clock = std::chrono::high_resolution_clock >
class stopwatch
{
private:
    const typename Clock::time_point m_start;
    const std::string                m_title;

public:
    stopwatch(const std::string& p_title = "") : m_title(p_title), m_start( Clock::now() ) {}
    ~stopwatch() {
        std::cout << "Computation of " << m_title << " performed in "
                  << elapsed_time<unsigned int, std::chrono::milliseconds>() << " ms\n";
    }

    template < typename Rep   = typename Clock::duration::rep, 
               typename Units = typename Clock::duration >
    Rep elapsed_time(void) const
    {
        std::atomic_thread_fence(std::memory_order_relaxed);
        auto l_time = std::chrono::duration_cast<Units>(Clock::now() - m_start).count();
        std::atomic_thread_fence(std::memory_order_relaxed);

        return static_cast<Rep>(l_time);
    }
};

using precise_stopwatch   = stopwatch<>;
using system_stopwatch    = stopwatch<std::chrono::system_clock>;
using monotonic_stopwatch = stopwatch<std::chrono::steady_clock>;

//////////////////////////////////////////////////////////////////////////////////////////
/*!
 * @brief custom_find
 *        Custom parallel implementation of std::find
 *        using std::thread and std::promise, std::future.
 */
template < class It, class T >
It custom_find( It p_first, It p_last, const T& p_val ) 
{
    /*!
     * @brief finder
     *        Callable struct to perform the
     *        search for one thread.
     */
    class finder
    {
    public:
        void operator()( It                 p_first, 
                         It                 p_last, 
                         const T&           p_val,
                         std::promise<It>&  p_res,
                         std::atomic<bool>& p_done,
                         std::mutex&        p_mtx )
        {
            coutWrapper{} << "Thread " << std::this_thread::get_id() << " - launched.\n";
            try
            {
                It p_cur { p_first };

                for (; ( p_cur != p_last ) && !p_done.load(); ++p_cur )
                {
                    if ( *p_cur == p_val )
                    {
                        // /!\ Can be a problem if already set
                        // so we protect it with a mutex
                        std::lock_guard<std::mutex> lck(p_mtx);

                        if ( !p_done.load() )
                        {
                            p_done.store    ( true  );
                            p_res .set_value( p_cur );
                            coutWrapper{} << "Thread " << std::this_thread::get_id()
                                          << " - found the value!\n";
                        }
                        return;
                    }
                }
            }
            catch ( ... )
            {
                p_res .set_exception( std::current_exception() );
                p_done.store        ( true );
            }
        }
    };

    static const unsigned long threads_hw{ std::thread::hardware_concurrency() };

    const unsigned long length = static_cast< unsigned long >( std::distance(p_first, p_last) );
    if ( !length ) 
        return p_last; 

    const unsigned long min_per_thread { 25 };
    const unsigned long max_threads    { (length + min_per_thread -1) / min_per_thread };
    const unsigned long threads_nb     { std::min( threads_hw ? threads_hw : 2, max_threads ) };
    const unsigned long block_sz       { length / threads_nb };

    std::atomic <bool>         l_flag{false};
    std::promise<It>           l_res;
    std::mutex                 l_mtx;
    std::vector< std::thread > l_threads(threads_nb);

    It block_str { p_first };
    for ( unsigned long i = 0; i < threads_nb; i++ )
    {
        It block_end { block_str };
        std::advance( block_end, block_sz );

        l_threads[i] = std::thread( finder(), 
                                    block_str, 
                                    block_end, 
                                    std::ref(p_val), 
                                    std::ref(l_res), 
                                    std::ref(l_flag),
                                    std::ref(l_mtx) );

        block_str = block_end;
    }

    for ( auto& elm : l_threads ) { elm.join(); }

    return l_flag.load() ? l_res.get_future().get() : p_last;
}

#define ELEMENTS 1e7 // The number of elements in the vector
#define FIND_ELM 42  // The element to find

int main()
{
    // Generate random numbers for the vector
    std::uniform_int_distribution<int> distrib(0, 10*ELEMENTS);
    std::default_random_engine         random_engine;

    // Create a vector of random double elements to be sorted
    std::vector<int> myVec(ELEMENTS);
    for ( auto& v : myVec ) { v = distrib(random_engine); }

    std::cout << "----- INPUT SIZE : " << ELEMENTS << " -----\n";
    {
        stopwatch watch("CUSTOM PARALLEL_FIND");
        auto resIt = custom_find( std::begin(myVec), std::end(myVec), FIND_ELM );
    }

    {
        stopwatch watch("SEQUENTIAL STD::FIND");
        auto resIt = std::find( std::begin(myVec), std::end(myVec), FIND_ELM );
    }

    return EXIT_SUCCESS;
}

/*
----- INPUT SIZE : 1e+07 -----
Thread 2 - launched.
Thread 3 - launched.
Thread 4 - launched.
Thread 5 - launched.
Thread 4 - found the value!
Thread 6 - launched.
Thread 7 - launched.
Thread 8 - launched.
Thread 9 - launched.
Computation of CUSTOM PARALLEL_FIND performed in 4 ms
Computation of SEQUENTIAL STD::FIND performed in 12 ms
*/