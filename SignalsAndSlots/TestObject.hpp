#pragma once
#ifndef TEST_OBJECT_HPP
#define TEST_OBJECT_HPP

#include<iostream>
#include<string>
#include<vector>

namespace my
{
	class TestObject
	{
	public:
		TestObject() : id{ ++counter }, val{ id }
		{
			std::cout << "[object " << id << "] " << "TestObject::TestObject() (default constructor)" << std::endl;
			status = "constructed with default constructor";
			history.push_back(status);
		}

		TestObject(int v) : id{ ++counter }, val{ v }
		{
			std::cout << "[object " << id << "] " << "TestObject::TestObject() (constructor with value)" << std::endl;
			status = "constructed with value ";
			status += std::to_string(v);
			history.push_back(status);
		}

		TestObject(const TestObject& other) : id{ ++counter }, parent_id{ other.id }, val{ other.val }
		{
			std::cout << "[object " << id << "] " << "TestObject::TestObject(const TestObject& other) (copy constructor)" << std::endl;
			status = "constructed with copy constructor from object with id: ";
			status += std::to_string(other.id);
			history.push_back(status);
		}
		TestObject& operator=(const TestObject& other)
		{
			if (this == &other) return *this;
			std::cout << "[object " << id << "] " << "TestObject& TestObject::operator=(const TestObject& other) (copy assignment operator)" << std::endl;
			status = "parameters of this object is assigned with parameters of object with id: ";
			status += std::to_string(other.id);
			history.push_back(status);
			val = other.val;
			return *this;
		}

		TestObject(TestObject&& other) : id{ ++counter }, parent_id{ other.id }
		{
			std::cout << "[object " << id << "] " << "TestObject::TestObject(TestObject&& other) (move constructor)" << std::endl;
			status = "constructed with move constructor from object with id: ";
			status += std::to_string(other.id);
			other.status = "is moved to new object with id: ";
			other.status += std::to_string(id);
			other.history.push_back(other.status);
			val = other.val;
			other.val = 0;
			history.push_back(status);
		}
		TestObject& operator=(TestObject&& other)
		{
			if (this == &other) return *this;

			std::cout << "TestObject& TestObject::operator=(TestObject&& other) (move assignment operator)" << std::endl;
			other.status = "this object is moved to object with id: ";
			other.status += std::to_string(this->id);
			other.history.push_back(other.status);
			history.push_back(status);
			val = other.val;
			other.val = 0;
			return *this;
		}

		virtual ~TestObject()
		{
			std::cout << "[object " << id << "] " << "virtual TestObject::~TestObject() (destructor)" << std::endl;
			status = "this object is destructed";
			history.push_back(status);
		}

		friend std::ostream& operator<<(std::ostream& os, const TestObject& obj)
		{
			os << "TestObject{id: " << obj.id << ", val: " << obj.val << " }";
			return os;
		}

		inline bool operator==(const TestObject& other) const noexcept { return val == other.val; }
		inline bool operator!=(const TestObject& other) const noexcept { return val != other.val; }
		inline bool operator< (const TestObject& other) const noexcept { return val < other.val; }
		inline bool operator> (const TestObject& other) const noexcept { return val > other.val; }
		inline bool operator<=(const TestObject& other) const noexcept { return val <= other.val; }
		inline bool operator>=(const TestObject& other) const noexcept { return val >= other.val; }

		inline const std::vector<std::string>& getHistory() const noexcept { return history; }
		inline const std::string& getStatus() const noexcept { return status; }
		inline int getId() const noexcept { return id; }
		inline int getParentId() const noexcept { return parent_id; }
		inline int getValue() const noexcept { return val; }

	protected:
		static int counter;
		int id{};
		int parent_id{};
		int val{};
		std::string status;
		std::vector<std::string> history;
	};

	int TestObject::counter = 0;
}

#endif // TEST_OBJECT_HPP
