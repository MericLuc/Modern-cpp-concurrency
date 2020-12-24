/************************************************************
 *           BASIC EXAMPLE OF A THREAD SAFE STACK           *
 ************************************************************/

#include <iostream>
#include <string>
#include <optional>
#include <thread>
#include <mutex>
#include <stack>
#include <vector>

/*!
 * @brief This (almost) achieves thread-safety 
 *        but limits parallelism alot because
 *        only one thread can operate on the structure
 *        at a given time !
 */

/*!
 * @note There are still race conditions
 *       that are inherited from interface !
 * 
 *       - Between empty() and top()
 *       - Between top()   and pop()
 * 
 * The method maybe_pop_top() has been introduced
 * to fight these race conditions.
 */

template < typename T >
class stackThreadSafe {
public:
       stackThreadSafe() : m_data(), m_mutx() {}
       stackThreadSafe(const stackThreadSafe& p_other) :
            m_data(p_other.m_data), m_mutx() {}
        stackThreadSafe& operator=(const stackThreadSafe& p_other) {
            if ( &p_other == this )
                return *this;
            m_data = p_other.m_data();

            return *this;
        }

        void push( T&& p_val ) {
            const std::lock_guard<std::mutex> l_lck(m_mutx);
            m_data.push(p_val);
        }

        void pop() {
            const std::lock_guard<std::mutex> l_lck(m_mutx);
            m_data.pop();
        }

        std::size_t size() const {
            const std::lock_guard<std::mutex> l_lck(m_mutx);
            return m_data.size();
        }

        T& top() {
            const std::lock_guard<std::mutex> l_lck(m_mutx);
            return m_data.top();
        }

        std::optional<T> maybe_pop_top() {
            const std::lock_guard<std::mutex> l_lck(m_mutx);
            
            if ( m_data.empty() ) return std::nullopt;

            std::optional<T> l_ret{ m_data.top() };
            m_data.pop();
            return l_ret;
        }

        const T& top() const {
            const std::lock_guard<std::mutex> l_lck(m_mutx);
            return m_data.top();
        }

private:
    std::stack<T> m_data;
    std::mutex    m_mutx;
};

int main()
{
    uint32_t THREADS_NB{3}, PUSH_NB{5}, POP_NB{5};

    stackThreadSafe<int>     myStack;
    std::vector<std::thread> producers, consumers;
    std::mutex               iomutex;

    // THREADS_NB Producers pushing PUSH_NB times to the stack
    for ( uint32_t id = 0; id < THREADS_NB; id++ )
    {
        producers.push_back( std::thread( [&, id]() {
            for ( int i = 0; i < PUSH_NB; i++ ) 
            {
                // Locked I/O for output clarity
                {
                    const std::lock_guard<std::mutex> l_lck(iomutex);
                    std::cout << "T" << id << ": pushed " << ( id * THREADS_NB + i ) << "\n";
                }
                myStack.push( id * THREADS_NB + i ); // push a unique val
            }
        } ) );
    }

    // THREADS_NB consumers poping POP_NB times to the stack
    for (uint32_t id = THREADS_NB; id < (THREADS_NB << 1); id++)
    {
        consumers.push_back( std::thread([&, id]() {
            for (int i = 0; i < POP_NB; i++)
            {
                auto curTop = myStack.maybe_pop_top();
                // Locked I/O for output clarity
                {
                    const std::lock_guard<std::mutex> l_lck(iomutex);
                    std::cout << "T" << id << ": popped " << ( curTop ? std::to_string(*curTop) : "nothing" ) << "\n";
                }
            }
        }));
    }

    for ( auto& t : producers )
        t.join();
    
    for ( auto& t : consumers )
        t.join();

    return EXIT_SUCCESS;
}