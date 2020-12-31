/************************************************************
 *            BACKGROUND TASKS USING STD::ASYNC             *
 ************************************************************/

/*!
 * @brief std::async allows you to run a function
 *        asynchronously and will return its value
 *        is a std:future.
 * 
 * See https://en.cppreference.com/w/cpp/thread/async
 * for more details.
 * 
 * NB : This code is inspired by the book
 *  "C++17 STL Cookbook" by Jacek Galowicz
 */

/*!
 * @note You can decide the launch policy
 *       very easily :
 *       - std::launch::async guarantees the function
 *         to be executed by another thread.
 *       - std::launch::deferred will execute the function
 *         in the same thread but later (lazy evaluation).
 *         The execution happens when get() or wait() is
 *         called on the std::future so if none of both
 *         happens, the function will not be called at all !
 *       - std::launch::async | std::launch::deferred (default)
 *         The STL's std::async is free to choose which policy
 *         shall be followed.
 */

/*!
 * @note Exceptions
 *      std::async can throw std::system_error with error
 *      condition std::errc::resource_unavailable_try_again
 *      if the launch policy is std::async but the impl was
 *      unable to launch a thread.
 */

/*!
 * @note When to use ?
 *       - To perform an tasks in background when it is not
 *         convenient to launch std::thread and join() them.
 *         For example, when you need to access the task 
 *         return value.
 */

#include <iostream>
#include <iomanip>
#include <map>
#include <string>
#include <future>
#include <iterator>
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
    auto        mySortedHist ( std::async( std::launch::async,
                                           task1,
                                           inputStr ) ); // std::future<std::multimap<size_t, char>>
    auto        mySortedStr  ( std::async( std::launch::async,
                                           task2,
                                           inputStr ) ); // std::future<std::string>
    auto        myVowelsNb   ( std::async( std::launch::deferred,
                                           task3,
                                           inputStr ) ); // std::future<size_t>
    /*
     * Do whatever you want as the tasks will perform
     * in the background and get the results when you need them.
     */

    std::cout << "Sorted histogram\n";
    for (const auto &[c, nb] : mySortedHist.get() )
    {
        std::cout << c << " - " << nb << "\n";
    }

    std::cout << "Sorted string\n"    << mySortedStr.get() << "\n";
    std::cout << "Number of vowels: " << myVowelsNb .get() << "\n";

    return EXIT_SUCCESS;
}