// Implementation of the thread_pool class by osuka_
// https://codereview.stackexchange.com/questions/221626/c17-thread-pool

#include <iostream>
#include <vector>
#include <threadpool.h>

int multiply(int x, int y)
{
    return x * y;
}

int main()
{
    thread_pool                   pool   ;
    std::vector<std::future<int>> futures;

    for ( const int &x : {2, 4, 7, 13} )
    {
        futures.push_back( pool.execute(multiply, x, 2) );
    }

    for (auto &fut : futures)
    {
        std::cout << fut.get() << std::endl;
    }

    return 0;
}