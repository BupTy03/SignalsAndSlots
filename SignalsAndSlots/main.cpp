#include "slot.hpp"
#include "signal.hpp"
#include "TestObject.hpp"

#include <iostream>
#include <memory>
#include <vector>

void print_sum(int a, int b)
{
	std::cout << "(print_sum) Sum: " << a + b << std::endl;
}

struct SumPrinter
{
	void print_sum(int a, int b)
	{
		std::cout << "(&SumPrinter::print_sum) Sum: " << a + b << std::endl;
	}
};

struct Functor
{
	void operator()(int a, int b) const
	{
		std::cout << "(&Functor::operator()) Sum: " << a + b << std::endl;
	}
};

int main(int argc, char* argv[])
{
	// slots
	my::slot<int, int> sl(print_sum);
	sl(2, 2);

	SumPrinter sp;
	my::slot<int, int> sl2(&sp, &SumPrinter::print_sum);
	sl2(2, 2);

	my::slot<int, int> sl3([](int a, int b) {
		std::cout << "(lambda) Sum: " << a + b << std::endl;
	});
	sl3(3, 4);

	my::slot<int, int> sl4(Functor{});
	sl4(2, 2);

	auto cp_sl = sl;
	cp_sl(2, 2);


	// signals
	my::signal<int, int> sign;
	auto conn = sign.connect(print_sum);
	auto conn2 = sign.connect(&sp, &SumPrinter::print_sum);
	auto conn3 = sign.connect([](int a, int b) {
		std::cout << "(Lambda) Sum: " << a + b << std::endl;
	});

	//conn.disconnect();
	//conn2.disconnect();
	conn3.disconnect();

	sign(2, 3);

	system("pause");
	return 0;
}