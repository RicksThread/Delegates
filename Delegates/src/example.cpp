#include <iostream>
#include "MultiCastDelegate.hpp"

//Example use of the delegate system

class A
{
public:
	int x;
	A(int a) : x(a) {}

	void foo(int a)
	{
		std::cout << "A called! public var x: " << x << "; param. 'a' passed: " << a << "\n";
	}

	void fooNoParams()
	{
		std::cout << "A called! public var x: " << x << "\n";
	}
};

class B
{
public:
	int x;
	float o;
	B(int a, float b) : x(a), o(b) {}

	void foo(int c)
	{
		std::cout << "foo called (B)! public var 'x': << " << x << "; public var 'o': " << o << "\n";
	}

	void foo1(int c)
	{
		std::cout << "foo1 called (B)! public var 'x': << " << x << "; public var 'o': " << o << "; param 'c' passed: " << c << "\n";
	}
};

float Sum(float a, float b)
{
	return a + b;
}

void GlobalFunc(int a)
{
	std::cout << "global function called! Param passed: " << a << "\n";
}

int main()
{
	using namespace DelegateSystem;


	std::cout << "\n\n======DELEGATE GLOBAL FUNC======\n\n";
	//create a delegate (similar to the "func" of the function library)

	//first template parameter: class type
	//second template parameter: return type
	//third, fourth, fifth... template parameters: methods parameters
	Delegate<void, void, int> globalFuncInt(GlobalFunc);

	//call the delegate
	globalFuncInt.Invoke(5);


	std::cout << "\n\n======DELEGATE GLOBAL FUNC RETURN TYPE======\n\n";
	//delegate with return type different from void with two float parameters
	Delegate<void, float, float, float> sum(Sum);

	int x = sum.Invoke(3, 6);
	std::cout << "sum is: " << x << "\n";


	std::cout << "\n\n======DELEGATE MEMBER FUNC======\n\n";
	//delegate for member function example:
	A a(3);
	//delegate pointer to member function of class type classexample
	Delegate<A, void> memberFunc(&A::fooNoParams, a);

	memberFunc.Invoke();

	std::cout << "\n\n======MULTICAST DELEGATE======\n\n";
	B b(4, 5.31f);
	//multicastdelegate it accepts global and member functions but it does lack lambda support
	MultiCastDelegate<void, int> multiFuncs;
	multiFuncs.Add(&GlobalFunc);
	multiFuncs.Add(&A::foo,  a);
	multiFuncs.Add(&B::foo,  b);
	multiFuncs.Add(&B::foo1, b);

	multiFuncs.Invoke(8);

	//you can also remove a function from the multicastdelegate
	multiFuncs.Remove(&A::foo, a);
	std::cout << "\n\n======MULTICAST DELEGATE (minus a func.)======\n\n";
	multiFuncs.Invoke(53);

	//the return value is taken from the last method called

	return 0;
}