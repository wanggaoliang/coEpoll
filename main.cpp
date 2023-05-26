#include "Lazy.h"
#include "Task.h"
#include <iostream>
Lazy<int> test2(int a)
{
    std::cout << "start Lazy test2" << std::endl;
    co_return a + 10;
}


Lazy<int> test1(int a)
{
    std::cout << "start Lazy test1" << std::endl;
    co_return co_await test2(a + 100);
}

Task start_sync(int c)
{
    std::cout << "start Task" << std::endl;
    auto ret = co_await test1(c);
    std::cout << "end Task:"<<ret << std::endl;
}
int main()
{
    std::cout << "start main" << std::endl;
    for (int i = 0;i < 10;i++)
    {
        start_sync(i);
    }
    
}