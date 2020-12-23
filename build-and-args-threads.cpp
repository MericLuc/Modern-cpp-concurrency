/************************************************************
 *               PASSING ARGUMENTS TO THREADS               *
 ************************************************************/

#include <thread>
#include <iostream>
#include <chrono>

/*!
 * From https://en.cppreference.com/w/cpp/thread/thread/thread
 * template< class Function, class... Args >
 * explicit thread( Function&& f, Args&&... args );
 */

// ----- 1 : Call by reference ----- //
void doTheJobByRef(int& a) { 
    for ( int i = 0; i< 5; i++ ) 
    {
        std::cout << "Doing the job by reference... " << ++a << "\n";
        std::this_thread::sleep_for( std::chrono::milliseconds(5) );
    }
}

// ----- 2 : Call by value ----- //
void doTheJobByVal(int a) {
    for (int i = 0; i < 5; i++)
    {
        std::cout << "Doing the job by value... " << ++a << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// ----- 3 : Call a method of
//           an instance of a class. ----- //
class AClass
{
public:
    void AMethod() {
        for (int i = 0; i < 5; i++)
        {
            std::cout << "Doing the job in AClass::AMethod()... " << i << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
};

// ----- 4 : Call the operator() on a
//           callable object. ----- //
class ACallableClass
{
public:
    void operator()() {
        for (int i = 0; i < 5; i++)
        {
            std::cout << "Doing the job in AClass()... " << i << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
    }
};

int main()
{
    int            var{0};
    AClass         myClass;
    ACallableClass myCallableClass;

    std::cout << "Before the thread : " << var << "\n";

    std::thread t1( doTheJobByRef, std::ref(var) ); // Pass by reference
    std::thread t2( doTheJobByVal, var           ); // Pass by value
    std::thread t3( std::move(t2)                ); // t3 is now running t2
    std::thread t4( &AClass::AMethod, &myClass   ); // Runs AClass::AMethod() on myClass
    std::thread t5( myCallableClass              ); // Runs operator() on myCallableClass

    // t2 is not a thread anymore
    std::cout << "t2 is " << ( t2.joinable() ? "joinable\n" : "not joinable\n" );
                                                
    t1.join();
    t3.join();
    t4.join();
    t5.join();

    std::cout << "After the thread : " << var << std::endl;

    return EXIT_SUCCESS;
}

/*
    Before the thread : 0
    Doing the job by reference... 1
    Doing the job by value... 1
    Doing the job in AClass::AMethod()... 0
    Doing the job in AClass()... 0
    t2 is not joinable
    Doing the job in AClass::AMethod()... 1
    Doing the job in AClass::AMethod()... 2
    Doing the job in AClass()... 1
    Doing the job in AClass::AMethod()... 3
    Doing the job by reference... 2
    Doing the job in AClass::AMethod()... 4
    Doing the job in AClass()... 2
    Doing the job in AClass()... 3
    Doing the job by value... 2
    Doing the job by reference... 3
    Doing the job in AClass()... 4
    Doing the job by reference... 4
    Doing the job by reference... 5
    Doing the job by value... 3
    Doing the job by value... 4
    Doing the job by value... 5
    After the thread : 5
*/