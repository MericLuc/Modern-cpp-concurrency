/************************************************************
 *              THREAD SAFE STD::COUT WRAPPER               *
 ************************************************************/

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <sstream>
#include <vector>

/*!
 * @note C++20 introduces a built-in class to
 *       perform synchronised std::cout wrapper
 *       with std::osyncstream.
 * 
 * See http://en.cppreference.com/w/cpp/io/basic_osyncstream 
 * for more details.
 */

/*!
 * @note Our wrapper object only lives for one line.
 *       The destructor is responsible for printing
 *       the underlying buffer content.
 */

/*!
 * @note coutWrapper is still a std::stringstream
 *       and can of course be used like one.
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

void thread_function(int id) {
    // Do the work
    // ...
    
    // Use std::cout to see the mess
    std::cout     << "(cout) In the thread " << id << "\n";
    // Then call our wrapper
    coutWrapper{} << "(wrap) In the thread " << id << "\n"; 
}

int main()
{
    std::vector<std::thread> myThreads;
    for ( int i = 0; i < 5; i++ )
    {
        myThreads.emplace_back( thread_function, i );
    }

    for ( auto& t : myThreads ) { t.join(); }

    coutWrapper{} << "It worked fine\n";

    return EXIT_SUCCESS;
}