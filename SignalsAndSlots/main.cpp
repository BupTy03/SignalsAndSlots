#include"signal.hpp"
#include"TestObject.hpp"

#include<iostream>
#include<memory>
#include<vector>
#include<functional>
#include<type_traits>

template<class T, class A = std::allocator<T>>
struct ObservableVector
{
	ObservableVector() 
		: onAdd{ onAdd_ }, onRemove{ onRemove_ }{}

	void Add(const T& val)
	{
		vec_.push_back(val);
		onAdd_(val);
	}

	void Remove(const T& val)
	{
		auto it = std::find(std::begin(vec_), std::end(vec_), val);
		if (it == std::end(vec_)) return;
		vec_.erase(it);
		onRemove_(val);
	}

	const my::signal<const T&>& onAdd;
	const my::signal<const T&>& onRemove;

private:
	my::signal<const T&> onAdd_;
	my::signal<const T&> onRemove_;
	std::vector<T, A> vec_;
};

template<class T>
struct Observer
{
	void on_add(const T& val)
	{
		std::cout << val << " was added\n";
	}

	void on_remove(const T& val)
	{
		std::cout << val << " was removed\n";
	}
};

int main()
{
	ObservableVector<my::TestObject> vec;
	Observer<my::TestObject> obs;

	vec.onAdd.connect(&obs, &Observer<my::TestObject>::on_add);
	vec.onRemove.connect(&obs, &Observer<my::TestObject>::on_remove);

	my::TestObject obj1;
	my::TestObject obj2;

	vec.Add(obj1);
	vec.Add(obj2);
	vec.Add(my::TestObject());
	vec.Add(my::TestObject());

	vec.Remove(obj1);
	vec.Remove(obj2);

	system("pause");
	return 0;
}