#include"signal.hpp"
#include"TestObject.hpp"

#include<iostream>
#include<memory>
#include<vector>

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

int main()
{
	{
		ObservableVector<my::TestObject> vec;
		Observer<my::TestObject> obs;

		vec.onAdd.connect(&obs, &Observer<my::TestObject>::on_add);
		vec.onRemove.connect(&obs, &Observer<my::TestObject>::on_remove);

		my::TestObject obj1;
		vec.Add(obj1);
		vec.Remove(obj1);
	}
	system("pause");
	return 0;
}