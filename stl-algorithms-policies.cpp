/*!
 * @RESSOURCES
 *
 * A very interesting article from Bartłomiej Filipek
 * with discussion on gains and benchmark for policies 
 * STL algorithms
 *     -> https://www.bfilipek.com/2018/11/parallel-alg-perf.html 
 *
 * The book "C++17 in detail" by Bartlomiej Filipek.
 * 
 * A benchmark with working visual studio 2019 solutions
 *     -> https://github.com/MericLuc/ParallelAlgorithms 
 */

/*!
 * @NOTES
 *
 * Not every compilers support the "Standardization of Parallelism TS"
 * feature. 
 * At mentioned in https://en.cppreference.com/w/cpp/compiler_support
 * here are the only compiliers that may run this program at
 * the moment :
 *     - GCC libstdc++
 *     - MSVC 
 *     - Intel C++
 */

#include <algorithm>
#include <execution>
#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <atomic>

// ------------------------ Utility ------------------------ //

/*
 * @brief A stopwatch class to perform measures
 */
template < typename Clock = std::chrono::high_resolution_clock >
class stopwatch
{
private:
    const typename Clock::time_point m_start;

public:
    stopwatch() : m_start( Clock::now() ) {}
    ~stopwatch() {
        std::cout << "Computation performed in "
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

// ------------------------ Main ------------------------ //

#define ELEMENTS 1e6 // The number of elements in the vector
int main()
{
    // Generate random numbers for the vector
    std::uniform_real_distribution<double> distrib(0, 100);
    std::default_random_engine             random_engine;

    // Create a vector of random double elements to be sorted
    std::vector<double> myVec(ELEMENTS);
    for ( auto& v : myVec ) { v = distrib(random_engine); }

    {
        // ----- Test 1 ----- //
        // - sequential sort
        std::cout << "Test 1 - Sequential sort over " << ELEMENTS << " elements\n";
        stopwatch watch;
        std::sort( std::execution::seq, std::begin(myVec), std::end(myVec));
    }

    {
        // ----- Test 2 ----- //
        // - parallel sort
        std::cout << "Test 1 - Parallel sort over " << ELEMENTS << " elements\n";
        stopwatch watch;
        std::sort(std::execution::par, std::begin(myVec), std::end(myVec));
    }

    {
        // ----- Test 3 ----- //
        // - parallel and vectorized sort
        std::cout << "Test 1 - Parallel and vectorized sort over " << ELEMENTS << " elements\n";
        stopwatch watch;
        std::sort(std::execution::par_unseq, std::begin(myVec), std::end(myVec));
    }

    return EXIT_SUCCESS;
}