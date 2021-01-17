#include "ThreadPool.h"
#include "ThreadPool.h"
#include <windows.h>
#include <iostream>
#include <exception>

struct MyException : public std::exception {
	const char* what() const throw () {
		return "MyException";
	}
};

int ThreadFunc1()
{
	for (int i = 0; i < 5; ++i)
	{
		printf("THRDFNC1\n");
		fflush(stdout);
		Sleep(1000);
	}
	return 0;
}

int ThreadFunc2(const int intValue)
{
	for (int i = 0; i < 5; ++i)
	{
		printf("THRDFNC2, prms: %d\n", intValue);
		fflush(stdout);
		Sleep(1000);
	}
	return 0;
}

char ThreadFunc3(const int intValue, const std::string stringValue)
{
	for (int i = 0; i < 5; ++i)
	{
		printf("THRDFNC3, prms: %d %s\n", intValue, stringValue.c_str());
		fflush(stdout);
		Sleep(1000);
	}
	return 'h';
}
char ThreadFunc5(const int intValue, const std::string stringValue)
{
	for (int i = 0; i < 5; ++i)
	{
		printf("THRDFNC5, prms: %d %s\n", intValue, stringValue.c_str());
		fflush(stdout);
		Sleep(1000);
	}
	return 'h';
}
char ThreadFunc6(const int intValue, const std::string stringValue)
{
	for (int i = 0; i < 5; ++i)
	{
		printf("THRDFNC6, prms: %d %s\n", intValue, stringValue.c_str());
		fflush(stdout);
		Sleep(1000);
	}
	return 'h';
}
int ThreadFunc4(const int intValue)
{
	for (int i = 0; i < 5; ++i)
	{
		printf("THRDFNC4, prms: %d\n", intValue);
		fflush(stdout);
		Sleep(1000);
		if (i == 2)
			throw MyException();
	}
	return 0;
}

int main()
{
	ThreadPool* pool = new ThreadPool(4);

	auto res1 = pool->addTask(&ThreadFunc1);
	auto res2 = pool->addTask(&ThreadFunc2, 42);
	auto res3 = pool->addTask(&ThreadFunc3, 21, std::string("strthrd3"));
	auto res4 = pool->addTask(&ThreadFunc4, 21);
	auto res5 = pool->addTask(&ThreadFunc5, 21, std::string("strthrd5"));
	auto res6 = pool->addTask(&ThreadFunc6, 21, std::string("strthrd6"));
	try {
		printf("THRDFNC1 result = %d\n", res1.get());
	}
	catch (MyException& e) {
		std::cout << e.what() << std::endl;
	}
	catch (std::exception& e) {
		std::cout << "Task 1 was declined" << std::endl;
	}

	try {
		printf("THRDFNC2 result = %d\n", res2.get());
	}
	catch (MyException& e) {
		std::cout << e.what() << std::endl;
	}
	catch (std::exception& e) {
		std::cout << "Task 2 was declined" << std::endl;
	}

	try {
		printf("THRDFNC3 result = %d\n", res3.get());
	}
	catch (MyException& e) {
		std::cout << e.what() << std::endl;
	}
	catch (std::exception& e) {
		std::cout << "Task 3 was declined" << std::endl;
	}

	try {
		printf("THRDFNC6 result = %d\n", res6.get());
	}
	catch (MyException& e) {
		std::cout << e.what() << std::endl;
	}
	catch (std::exception& e) {
		std::cout << "Task 6 was declined" << std::endl;
	}

	try
	{
		printf("THRDFNC4 result = %d\n", res4.get());
	}
	catch (MyException& e) {
		std::cout << e.what() << std::endl;
	}
	catch (std::exception& e) {
		std::cout << "Task 4 was declined" << std::endl;
	}

	getchar();
	return 0;
}