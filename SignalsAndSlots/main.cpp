#include "slot.hpp"
#include "signal.hpp"
#include "TestObject.hpp"

#include <iostream>
#include <memory>
#include <vector>

#if 0
template<class T, class A = std::allocator<T>>
struct ObservableVector
{
	ObservableVector() 
		: onAdd{ onAdd_ }, onRemove{ onRemove_ }{}

	void Add(const T& val)
	{
		vec_.push_back(val);
		onAdd_(val, this);
	}

	void Remove(const T& val)
	{
		auto it = std::find(std::begin(vec_), std::end(vec_), val);
		if (it == std::end(vec_)) return;
		vec_.erase(it);
		onRemove_(val, this);
	}

	const my::signal<const T&, ObservableVector*>& onAdd;
	const my::signal<const T&, ObservableVector*>& onRemove;

private:
	my::signal<const T&, ObservableVector*> onAdd_;
	my::signal<const T&, ObservableVector*> onRemove_;
	std::vector<T, A> vec_;
};

template<class T>
struct Observer
{
	void on_add(const T& val, ObservableVector<T>* vec)
	{
		std::cout << val << " was added\n";
		vec->onAdd.disconnect(this, &Observer::on_add);
	}

	void on_remove(const T& val, ObservableVector<T>* vec)
	{
		std::cout << val << " was removed\n";
		vec->onRemove.disconnect(this, &Observer::on_remove);
	}
};
#endif

void print_sum(int a, int b)
{
	std::cout << "Sum: " << a + b << std::endl;
}

struct SumPrinter
{
	void print_sum(int a, int b)
	{
		std::cout << "Sum: " << a + b << std::endl;
	}
};

int main(int argc, char* argv[])
{
	// slots
	my::slot<int, int> sl(print_sum);
	if (sl.type() == my::slot_type::FUNCTION) {
		std::cout << "Slot type: FUNCTION" << std::endl;
	}
	std::cout << "Slot id: " << sl.id() << std::endl;
	sl(2, 2);

	SumPrinter sp;
	my::slot<int, int> sl2(&sp, &SumPrinter::print_sum);
	if (sl2.type() == my::slot_type::MEMBER_FUNCTION) {
		std::cout << "Slot type: MEMBER_FUNCTION" << std::endl;
	}
	std::cout << "Slot id: " << sl2.id() << std::endl;
	sl2(2, 2);

	my::slot<int, int> sl3([](int a, int b) {
		std::cout << "Sum: " << a + b << std::endl;
	});
	if (sl3.type() == my::slot_type::FUNCTOR) {
		std::cout << "Slot type: FUNCTOR" << std::endl;
	}
	std::cout << "Slot id: " << sl3.id() << std::endl;
	sl3(2, 2);

	auto cp_sl = sl;
	cp_sl(2, 2);


	// signals
	my::signal<int, int> sign;
	auto conn = sign.connect(print_sum);
	auto conn2 = sign.connect(&sp, &SumPrinter::print_sum);
	auto conn3 = sign.connect([](int a, int b) {
		std::cout << "Sum: " << a + b << std::endl;
	});

	conn.disconnect();
	//conn2.disconnect();
	//conn3.disconnect();

	std::cout << "Signal called!\n";
	sign(2, 3);

#if 0
	{
		ObservableVector<my::TestObject> vec;
		Observer<my::TestObject> obs;

		vec.onAdd.connect(&obs, &Observer<my::TestObject>::on_add);
		vec.onRemove.connect(&obs, &Observer<my::TestObject>::on_remove);

		my::TestObject obj1;
		vec.Add(obj1);
		vec.Remove(obj1);

		vec.Add(my::TestObject());
	}
#endif

	system("pause");
	return 0;
}