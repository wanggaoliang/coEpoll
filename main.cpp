#include "CoRo/Lazy.h"
#include "CoRo/Task.h"
#include "CoKernel/CoKernel.h"
#include <iostream>
#include <chrono>

template <typename Awaitable>
auto ttt_transform(Awaitable &&awaitable)
{
    return std::forward<Awaitable>(awaitable);
}

class A
{
public:
    A() :a(0)
    {
        std::cout << "couns A" << std::endl;
    }
    ~A()
    {
        std::cout << "~ A" << std::endl;
    }
    int a;
};
struct base
{
    base() { std::cout << "base" << std::endl; }
    ~base() { std::cout << "~base" << std::endl; }
};
struct RunInCore2:public base
{
    using ret_type = int;

    template <typename F>
    RunInCore2(F &&func, std::shared_ptr<A> &fq) : func_(func), fq_(fq)
    {
        a = 0;
        std::cout << "cRIC:" << fq_.use_count() << std::endl;
    }

    RunInCore2(const RunInCore2 &old) : func_(old.func_),  ret(old.ret), fq_(old.fq_)
    {
        a = 1;
        std::cout << "lRIC:" << fq_.use_count() << std::endl;
    }

    RunInCore2(RunInCore2 &&old) : func_(old.func_), ret(std::move(old.ret)), fq_(old.fq_)
    {
        a = 2;
        std::cout << "rRIC:" << fq_.use_count() << std::endl;
    }

    ~RunInCore2()
    {
        std::cout << "~RIC:" << a << ":" << fq_.use_count() << std::endl;
    }

    bool await_ready() noexcept
    {
        
            ret = func_();
            return true;
    }

    void await_suspend(std::coroutine_handle<> handle)
    {
    }

    ret_type await_resume() noexcept
    {
        std::cout << "resume" << std::endl;
        return ret;
    }

private:
    ret_type ret;
    int a;
    std::shared_ptr<A> &fq_;
    std::function<ret_type()> func_;
};

Task testwwwrrr()
{
    for (int i = 3;i < 5;i++)
    {
        auto ret = co_await CoKernel::getKernel()->updateIRQ(21, 0);
        std::cout << "ret:" << ret << std::endl;
    }
}
int main()
{
    std::cout << "1" << std::endl;
    CoKernel::INIT(3);
    testwwwrrr();
    std::cout << "3" << std::endl;
    CoKernel::getKernel()->start();
    std::cout << "2" << std::endl;
    return 0;
}