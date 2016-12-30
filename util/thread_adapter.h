/**
 */

#ifndef  _THREAD_ADAPTER_H_
#define  _THREAD_ADAPTER_H_

namespace poseidon{
namespace util{

template<class T, class A>
class Adapter
{
public:

	Adapter(T t, A param):t_(t), param_(param){}

    void operator () ()
    {
    	t_(param_);
    }
private:
    T * t_;
    A param_;
};

template<class T, class A, class B>
class Adapter2
{
public:

	Adapter2(T t, A param1, B param2):t_(t), param1_(param1), param2_(param2){}

    void operator () ()
    {
    	t_(param1_, param2_);
    }
private:

    T * t_;
    A param1_;
    B param2_;
};

#if 0

#include <iostream>

void f(int a)
{
	std::cout<<a<<std::endl;
	return;
}

typedef void F(int);
int main()
{
	Adapter<F, int>(f, 10)();
	return 0;
}
#endif

}
}

#endif   /* ----- #ifndef CN_API_THREAD_ADAPTER_H_  ----- */

