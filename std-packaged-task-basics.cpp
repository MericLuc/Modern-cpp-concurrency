/************************************************************
 *              BASICS OF STD::PACKAGED_TASK                *
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
 * @note When to use ?
 * To perform an tasks in background.
 * When you want to be able to :
 *       - Invoke the start of the task (contrary to std::async)
 *       - Move it to a std::thread (same)
 * 
 * In a nutshell, std::packaged_task is a std::function linked to a 
 * std::future and std::async wraps and calls a std::packaged_task 
 * (possibly in a different thread).
 * 
 * More resources here :
 *      - https://stackoverflow.com/questions/18143661/what-is-the-difference-between-packaged-task-and-async
 *      - http://scottmeyers.blogspot.com/2013/03/stdfutures-from-stdasync-arent-special.html
 *      - http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3451.pdf
 */

#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <future>
#include <iterator>
#include <utility>
#include <functional>
#include <algorithm>

/*!
 * @brief flip_map
 *        Helper function for task1.
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
 * @brief task1
 *        compute a sorted histogram of characters 
 *        in a given std::string
 */
std::multimap<size_t, char> task1(const std::string &p_input)
{
    std::map<char, size_t> l_ret;

    for ( const char& c : p_input ) { ++l_ret[std::tolower(c)]; }
    return flip_map(l_ret);
}

/*!
 * @brief task2
 *        Compute a sorted copy of an input string.
 */
std::string task2( const std::string& p_input ) 
{
    std::string l_ret;
    std::transform( std::begin(p_input), 
                    std::end  (p_input), 
                    std::back_inserter(l_ret),
                    [](const char& c){ return std::tolower(c); });

    std::sort( std::begin(l_ret), std::end(l_ret) );
    return l_ret;
}

size_t task3( const std::string& p_input ) 
{
    static char vowels[] {"aeiouy"};
    return std::count_if( std::begin(p_input), 
                          std::end  (p_input), 
                          [](const char& c){ 
                              return std::find( std::begin(vowels), 
                                                std::end  (vowels), 
                                                std::tolower(c) ) != std::end(vowels); } );
}

int main()
{
    std::string inputStr { "Hello beautiful World! Nice to meet you!" };

    // You can bind the callable object (in that case a function)
    // to its agruments
    std::packaged_task< std::multimap<size_t, char>( const std::string& ) > 
        ptask1 ( std::bind(task1, std::ref(inputStr)) );
    
    // Or just provide the callable object
    // and give the arguments at invoke call.
    std::packaged_task<std::string( const std::string & )>
        ptask2 ( task2 );
    std::packaged_task< size_t( const std::string& ) >       
        ptask3 ( task3 );
    // Note : we could also use a lambda

    /*
     * Use std::packaged_task<R(Args...)>::get_future
     * to get the std::future<R> that will get you the
     * return value when ready.
     * 
     * See https://en.cppreference.com/w/cpp/thread/packaged_task/get_future
     * for more informations.
     */
    auto mySortedHist = ptask1.get_future(); // std::future<std::multimap<size_t, char>>
    auto mySortedStr  = ptask2.get_future(); // std::future<std::string>
    auto myVowelsNb   = ptask3.get_future(); // std::future<size_t>

    /*
     * std::packaged_task<R(Args...)>::get_future
     * will throw std::future_error :
     *  - Of category std::future_already_retrived
     *    if the shared state has already been retrieved by a call 
     *    to std::packaged_task<R(Args...)>::get_future.
     *  - Of category std::no_state if *this has no shared state.
     */
    try
    {
        auto tmp = ptask1.get_future();
    }
    catch( const std::future_error& e )
    {
        std::cerr << "Could not call get_future() on ptask1 - " 
                  << e.what() << "\n";
    }
    

    /*
     * You need to start the tasks yourself
     */
    ptask1( ""       );
    ptask2( inputStr );
    ptask3( inputStr );

    /* 
     * If you call operator() twice, it will throw a 
     * std::future_error (error category is std::promise_already_satisfied)
     * 
     * See https://en.cppreference.com/w/cpp/thread/packaged_task/operator()
     * for more informations.
     */
    try {
        // Try to invoke ptask1 again
        ptask1( inputStr );
    }
    catch ( const std::future_error& e)
    {
        std::cout << "Could not invoke ptask1 - " << e.what() << "\n";
    }

    std::cout << "Sorted histogram\n";
    for (const auto &[c, nb] : mySortedHist.get() )
    {
        std::cout << c << " - " << nb << "\n";
    }

    std::cout << "Sorted string\n"    << mySortedStr.get() << "\n";
    std::cout << "Number of vowels: " << myVowelsNb .get() << "\n";

    return EXIT_SUCCESS;
}