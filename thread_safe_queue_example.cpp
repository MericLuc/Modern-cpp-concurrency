/************************************************************
 *                    THREAD SAFE QUEUE                     *
 ************************************************************/

#include <iostream>
#include <list>
#include <vector>
#include <map>
#include <optional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <variant>

/*!
 * @brief queueThreadSafe
 *        push() and pop() operations can only be performed if 
 *        the queue state is OPENED.
 *        These operations also have a settable timeout and will
 *        give an indication of success/failure:
 *              - through a bool for push()
 *              - through std::optional for pop()
 */

/*!
 * @note We will make use of some C++17 features
 *       such as std::variant for error-handling.
 * 
 * You can find more informations about std::variant
 * here https://en.cppreference.com/w/cpp/utility/variant
 * and some tests on it here 
 * https://github.com/MericLuc/Cpp17-Features-tests/std-variant
 */

template <typename T>
class queueThreadSafe
{
public:
    enum State { OPENED, CLOSED };

    enum class StatusCode { 
        ERR_NO    ,  // No error
        ERR_FULL  ,  // Capacity error - trying to push() on a queue that is full.
        ERR_EMPTY ,  // Capacity error - trying to get() on a queue that is empty.
        ERR_TIMOUT,  // Timeout error  - trying to get() or push() but timed out.
        ERR_ACCESS   // Access error   - trying to get() or push() on CLOSED queue.
    };

    explicit queueThreadSafe(size_t p_cap = 0) : m_state(OPENED), m_size(0), m_cap(p_cap) {}
    ~queueThreadSafe() { close(); }

    queueThreadSafe(const& queueThreadSafe) = delete;
    queueThreadSafe& operator=(const& queueThreadSafe) = delete;

    static std::string getStatus( StatusCode&& p_code ) { 
        return m_statusStr.at(p_code); 
    }

    static std::string getStatus( const std::variant<T, StatusCode>& p_code ) {
        return ( std::holds_alternative<StatusCode>(p_code) ) ? 
                m_statusStr.at(std::get<StatusCode>(p_code)) : 
                m_statusStr.at(StatusCode::ERR_NO);
    }

    void close() 
    { 
        std::unique_lock<std::mutex> lck(m_mtx);
        m_state = State::CLOSED;

        m_push.notify_all();
        m_pop .notify_all();
    }

    /*
     * @brief push
     *        Will block in case of full queue untill timeout
     *        or space appears.
     */
    [[maybe_unused]] StatusCode push( const T &  p_elm, 
                                      uint32_t&& p_ms = 2000 )
    {
        std::unique_lock<std::mutex> lck(m_mtx);

        // Wait untill "There is some place" OR "timeout"
        m_pop.wait_for(lck,
                       std::chrono::milliseconds(p_ms),
                       [this] { return (m_size < m_cap && m_state == State::OPENED ); });

        if ( m_size == m_cap ) 
            return StatusCode::ERR_FULL;

        if ( m_state == State::CLOSED )
            return StatusCode::ERR_ACCESS;

        ++m_size;
        m_data.push_back (p_elm );
        m_pop.notify_one();

        return StatusCode::ERR_NO;
    }

    [[maybe_unused]] StatusCode push(T &&p_elm,
                                     uint32_t &&p_ms = 2000 )
    {
        std::unique_lock<std::mutex> lck(m_mtx);

        // Wait untill "There is some place" OR "timeout"
        m_pop.wait_for(lck,
                       std::chrono::milliseconds(p_ms),
                       [this] { return (m_size < m_cap && m_state == State::OPENED); });

        if (m_size == m_cap)
            return StatusCode::ERR_FULL;

        if (m_state == State::CLOSED)
            return StatusCode::ERR_ACCESS;

        ++m_size;
        m_data.push_back( p_elm );
        m_pop.notify_one();

        return StatusCode::ERR_NO;
    }

    /*!
     * @brief pop
     *        Will return a std::variant that contains
     *        a value if possible, otherwise the corresponding StatusCode.
     */
    std::variant<T, StatusCode> pop( std::chrono::milliseconds &&p_ms = std::chrono::milliseconds(1) )
    {
        std::unique_lock<std::mutex> lck(m_mtx);

        // Wait untill "There is one item" OR "timeout"
        m_pop.wait_for(lck,
                       p_ms,
                       [this]{ return !m_data.empty() && m_state == State::OPENED; });
        if ( m_data.empty() )
            return StatusCode::ERR_EMPTY;

        if (m_state == State::CLOSED)
            return StatusCode::ERR_ACCESS;

        --m_size;
        T l_ret = m_data.front();
        m_data.pop_front();
        m_push.notify_one();

        return l_ret;
    }

private:
    State                   m_state; /*!< State of the queue               */
    size_t                  m_size;  /*!< Current size of the queue        */
    size_t                  m_cap;   /*!< Capacity of the queue            */
    std::mutex              m_mtx;   /*!< Mutex for operations             */
    std::list<T>            m_data;  /*!< Underlying container             */
    std::condition_variable m_push;  /*!< Condition variable for producers */
    std::condition_variable m_pop ;  /*!< Condition variable for consumers */

    inline static const std::map<StatusCode, std::string> m_statusStr =
        {
            { StatusCode::ERR_NO     , "OK!\n"},
            { StatusCode::ERR_FULL   , "Queue is full\n"},
            { StatusCode::ERR_EMPTY  , "Queue is empty\n"},
            { StatusCode::ERR_TIMOUT , "Timed out before end of operation\n"},
            { StatusCode::ERR_ACCESS , "Trying to access closed queue\n"}
    };
};

int main()
{
    const uint32_t THREADS_NB    { 5};
    const uint32_t OPERATIONS_NB { 4};
    const size_t   QUEUE_CAPACITY{10};

    std::mutex iomutex;
    std::vector<std::thread> producers, consumers;

    queueThreadSafe<int> myQueue(QUEUE_CAPACITY);

    // THREADS_NB Producers threads doing 
    // OPERATIONS_NB operations on the queue.
    for ( uint32_t id = 0; id < THREADS_NB; ++id )
    {
        producers.push_back(std::thread([&, id] {
            for ( uint32_t i = 0; i < OPERATIONS_NB; ++i )
            {
                {
                    std::lock_guard<std::mutex> lck(iomutex);
                    std::cout << "T" << id << ": trying to push " << (id * THREADS_NB + i) << "...";

                    std::cout << queueThreadSafe<int>::getStatus(myQueue.push(id * THREADS_NB + i));
                }
            }
        }));
    }

    // THREADS_NB Producers threads doing
    // OPERATIONS_NB operations on the queue.
    for ( uint32_t id = THREADS_NB; id < 2 * THREADS_NB; ++id )
    {
        consumers.push_back(std::thread([&, id] {
            auto maybe = myQueue.pop( std::chrono::milliseconds(5) );
            {
                std::lock_guard<std::mutex> lck(iomutex);
                std::cerr << "T" << id << ": poped - " << queueThreadSafe<int>::getStatus(maybe);
            }
        }));
    }

    for (auto &t : producers) t.join();
    for (auto &t : consumers) t.join();

    myQueue.close();

    std::cout << "ok\n";

    return EXIT_SUCCESS;
}
