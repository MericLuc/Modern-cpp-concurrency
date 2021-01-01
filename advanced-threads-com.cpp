/************************************************************
 *           COMMUNICATIONS BETWEEN THREADS USING           *
 *               STD::PROMISE AND STD::FUTURE               *
 ************************************************************/

/*!
 * @brief 
 * 
 * std::promise allows you to store a value (or exception)
 * that can be later acquired asynchronously using std::future.
 * 
 * A std::promise contains a shared state that can be :
 *      - Not evaluated yet.
 *      - Evaluated to a value.
 *      - Evaluated to an exception.
 * 
 * A std::promise can do 3 things with its shared state :
 *      - make ready : stores the result/exception into the shared state,
 *      make it ready and unblocks any thread waiting on a std::future
 *      associated with the shared state.
 *      - release : gives up its reference to the shared state.
 *      If it was the last reference, the shared state is destroyed.
 *      - abandon : stores an exception of type std::future_error
 *      withh type std::future_errc::broken_promise and make its
 *      shared state ready, and then release it.
 * 
 * See https://en.cppreference.com/w/cpp/thread/promise
 * and https://en.cppreference.com/w/cpp/thread/future
 * for more details.
 */

/*!
 * @note When to use ?
 * 
 */

#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <future>
#include <thread>
#include <iterator>
#include <utility>
#include <functional>
#include <algorithm>
#include <chrono>

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

/*!
 * @brief flip_map
 *        Helper function for T1.
 */
template <typename A, typename B>
std::multimap<B, A> flip_map(const std::map<A, B> &src)
{
    std::multimap<B, A> l_ret;
    std::transform(src.begin(), src.end(), std::inserter(l_ret, l_ret.begin()),
                   [](const auto &p) { return std::make_pair(p.second, p.first); });
    return l_ret;
}

/*!
 * @brief T1_job
 *        compute a sorted histogram of characters 
 *        in a given shared std::string.
 */
std::multimap<size_t, char> T1_job( std::shared_future<std::string>& p_input )
{
    coutWrapper{} << "T1 - Waiting for T2 to compute the value...\n";

    std::map<char, size_t> l_ret;

    // future::get() will wait until the future has
    // a valid result and retrieves it.
    if ( !p_input.valid() ) 
    {
        throw std::future_errc::no_state;
    }
    auto l_input = p_input.get();

    coutWrapper{} << "T1 - Now has access to the value!\n";

    for ( const char& c : l_input ) { ++l_ret[std::tolower(c)]; }

    coutWrapper{} << "T1 - ended.\n";

    return flip_map(l_ret);
}

/*!
 * @brief T2_job
 *        Compute a sorted copy of an input string.
 */
void T2_job( const std::string&        p_input, 
             std::promise<std::string> p_promise ) 
{
    coutWrapper{} << "T2 - Computing the shared-state value...\n";
    // Adding artificial delay for clarity
    std::this_thread::sleep_for( std::chrono::milliseconds(2000) );

    std::string l_ret;
    std::transform( std::begin(p_input), 
                    std::end  (p_input), 
                    std::back_inserter(l_ret),
                    [](const char& c){ return std::tolower(c); });

    std::sort( std::begin(l_ret), std::end(l_ret) );

    coutWrapper{} << "T2 - set the value of the promise to '" 
                  << l_ret << "'\n";

    // This will notify the associated std::shared_future
    // and the threads waiting for it to be ready.
    p_promise.set_value(l_ret);

    // You can keep doing some work
    std::this_thread::sleep_for( std::chrono::milliseconds(1000) );
    coutWrapper{} << "T2 - ended.\n";
}

/*!
 * @brief T3_job
 *        Compute the number of vowels in an input string
 *        computed by another thread using std::shared_future.
 */
size_t T3_job( std::shared_future<std::string>& p_input ) 
{
    static char vowels[] {"aeiouy"};

    coutWrapper{} << "T3 - Waiting for T2 to compute the value...\n";

    // future::get() will wait until the future has 
    // a valid result and retrieves it.
    if (!p_input.valid())
    {
        throw std::future_errc::no_state;
    }
    auto l_input = p_input.get();

    coutWrapper{} << "T3 - Now has access to the value!\n";

    size_t l_ret = std::count_if( std::begin(l_input), 
                                  std::end  (l_input), 
                          [](const char& c){ 
                              return std::find( std::begin(vowels), 
                                                std::end  (vowels), 
                                                std::tolower(c) ) != std::end(vowels); } );

    coutWrapper{} << "T3 - ended.\n";

    return l_ret;
}

int main()
{
    std::string inputStr { "Hello beautiful World! Nice to meet you!" };

    std::promise       <std::string> T2_promise;
    std::shared_future <std::string> T2_shared_future = T2_promise.get_future();

    auto T1(std::async(std::launch::async,
                       T1_job,
                       std::ref(T2_shared_future))); // std::future<std::multimap<size_t, char>>
    std::thread T2( T2_job, std::ref(inputStr), std::move(T2_promise) );
    std::thread T3( T3_job, std::ref(T2_shared_future) );

    T2.join();
    T3.join();

    coutWrapper{} << "T1 is the result of a std::task, but it "
                     "works exctly the same as std::thread T3\n";
    // Blocking untill T1 is available.
    // Note that can lead to deadlocks (for example if we put this
    // before joining T2...)
    for (const auto &[c, nb] :  T1.get() )
    {
        coutWrapper{} << c << nb;
    }

    return EXIT_SUCCESS;
}

/*
T2 - Computing the shared-state value...
T1 - Waiting for T2 to compute the value...
T3 - Waiting for T2 to compute the value...
T2 - set the value of the promise to '      !!abcdeeeeefhiillllmnoooortttuuuwy'
T1 - Now has access to the value!
T3 - Now has access to the value!
T1 - ended.
T3 - ended.
T2 - ended.
T1 is the result of a std::task, but it works exctly the same as std::thread T3
1a1b1c1d1f1h1m1n1r1w1y2i2!3t3u4o4l5e6
*/