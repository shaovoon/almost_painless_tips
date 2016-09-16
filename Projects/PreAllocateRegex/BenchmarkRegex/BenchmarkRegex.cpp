// BenchmarkRegex.cpp : Defines the entry point for the console application.
//

#include <vector>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>
#include <cmath>
#include <cassert>
#include <sstream>
#include <cstdlib>
#include <chrono>
#include <regex> 
#include <unordered_map>
#include <thread>
#include <mutex>
#include <memory> 

class timer
{
public: 
	timer() = default;
	void start_timing(const std::string& text_)
	{
		text = text_;
		begin = std::chrono::high_resolution_clock::now();
	}
	void stop_timing()
	{
		auto end = std::chrono::high_resolution_clock::now();
		auto dur = end - begin;
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
		std::cout << std::setw(16) << text << ":" << std::setw(5) << ms << "ms" << std::endl;
	}

private:
	std::string text;
	std::chrono::steady_clock::time_point begin;
};

#ifdef WIN32

#pragma optimize("", off)
template <class T>
void do_not_optimize_away(T* datum) {
	datum = datum;
}
#pragma optimize("", on)

#else
static void do_not_optimize_away(void* p) { 
	asm volatile("" : : "g"(p) : "memory");
}
#endif

struct singleton
{
public:
	typedef std::unordered_map<std::thread::id, std::unique_ptr<std::regex> > MapType;
	static void set(const std::string& reg_exp) { s_reg_exp = reg_exp; }
	static const std::regex& get()
	{
		std::lock_guard<std::mutex> lock(s_mutex);

		std::thread::id thread_id = std::this_thread::get_id();
		MapType::iterator it = s_map.find(thread_id);
		if (it == s_map.end())
		{
			std::unique_ptr<char[]> p = std::unique_ptr<char[]>(new char[1000]);
			s_map[thread_id] = std::unique_ptr<std::regex>(new std::regex(s_reg_exp));
			return *s_map[thread_id];
		}
		return *(it->second);
	}
private:
	static MapType s_map;
	static std::string s_reg_exp;
	static std::mutex s_mutex;
};
singleton::MapType singleton::s_map;
std::string singleton::s_reg_exp;
std::mutex singleton::s_mutex;

struct factory
{
public:
	static std::unique_ptr<std::regex> get(const std::string& reg_exp)
	{
		std::lock_guard<std::mutex> lock(s_mutex);
		std::unique_ptr<char[]> p = std::unique_ptr<char[]>(new char[1000]);
		return std::unique_ptr<std::regex>(new std::regex(reg_exp));
	}
private:
	static std::mutex s_mutex;
};
std::mutex factory::s_mutex;

const std::string local_match(const std::string& text);
const std::string static_match(const std::string& text); // not reentrant-safe
const std::string singleton_match(const std::string& text);
const std::string factory_match(const std::string& text, const std::regex& regex);

void parallel_invoke(int size, int threads, std::function<void(int, int)> func)
{
	typedef std::unique_ptr<std::thread> PtrType;
	std::vector< PtrType > vec;
	int each = size / threads;

	for (int i = 0; i < threads; ++i)
	{
		if (i == threads - 1) // last thread
		{
			vec.emplace_back(PtrType(new std::thread(func, each*i, each*(i + 1) + (size % threads))));
		}
		else
		{
			vec.emplace_back(PtrType(new std::thread(func, each*i, each*(i + 1) )));
		}
	}

	for (size_t i = 0; i < vec.size(); ++i)
	{
		vec[i]->join();
	}
}

const std::string REG_EXP = ".*PRICE:.*US\\$(\\d+\\.\\d+|[-+]*\\d+).*PER SHARE";

int main(int argc, char* argv[])
{
	const int LOOP = 1000000;
	std::string str1 = "Zoomer PRICE: US$1.23 PER SHARE";
	std::string str2 = "Boomer PRICE: US$4.56 PER SHARE";
	
	std::vector<std::string> vec;
	vec.push_back(str1);
	vec.push_back(str2);
	
	timer stopwatch;

	stopwatch.start_timing("local regex object");
	for(int j = 0; j < LOOP; ++j)
	{
		for(size_t i = 0; i < vec.size(); ++i)
		{
			do_not_optimize_away(local_match(vec[i]).c_str());
		}
	}
	stopwatch.stop_timing();

	stopwatch.start_timing("static regex object");
	for(int j = 0; j < LOOP; ++j)
	{
		for(size_t i = 0; i < vec.size(); ++i)
		{
			do_not_optimize_away(static_match(vec[i]).c_str());
		}
	}
	stopwatch.stop_timing();

	singleton::set(REG_EXP);
	stopwatch.start_timing("singleton regex object");
	for (int j = 0; j < LOOP; ++j)
	{
		for (size_t i = 0; i < vec.size(); ++i)
		{
			do_not_optimize_away(singleton_match(vec[i]).c_str());
		}
	}
	stopwatch.stop_timing();

	stopwatch.start_timing("local regex object(4 threads)");
	parallel_invoke(LOOP, 4, [&vec](int start, int end) {
		for (int j = start; j < end; ++j)
		{
			for (size_t i = 0; i < vec.size(); ++i)
			{
				do_not_optimize_away(local_match(vec[i]).c_str());
			}
		}
	});
	stopwatch.stop_timing();

	stopwatch.start_timing("singleton regex object(4 threads)");
	parallel_invoke(LOOP, 4, [&vec] (int start, int end) {
		for (int j = start; j < end; ++j)
		{
			for (size_t i = 0; i < vec.size(); ++i)
			{
				do_not_optimize_away(singleton_match(vec[i]).c_str());
			}
		}
	});
	stopwatch.stop_timing();

	stopwatch.start_timing("factory regex object(4 threads)");
	parallel_invoke(LOOP, 4, [&vec](int start, int end) {
		std::unique_ptr<std::regex> ptr = factory::get(REG_EXP);
		const std::regex& regex = *ptr;

		for (int j = start; j < end; ++j)
		{
			for (size_t i = 0; i < vec.size(); ++i)
			{
				do_not_optimize_away(factory_match(vec[i], regex).c_str());
			}
		}
	});
	stopwatch.stop_timing();
	
	/*
	std::cout << local_match(str1) << std::endl;
	std::cout << local_match(str2) << std::endl;
	std::cout << static_match(str1) << std::endl;
	std::cout << static_match(str2) << std::endl;
	singleton::set(REG_EXP);
	std::cout << singleton_match(str1) << std::endl;
	std::cout << singleton_match(str2) << std::endl;
	const std::regex regex(REG_EXP);
	std::cout << factory_match(str1, regex) << std::endl;
	std::cout << factory_match(str2, regex) << std::endl;
	*/

	return 0;
}

const std::string local_match(const std::string& text)
{
	std::string ipo_price = "";
	std::smatch what;
	const std::regex regex(REG_EXP);
	if (std::regex_match(text, what, regex))
	{
		ipo_price = what[1];
	}
	return ipo_price;
}

const std::string static_match(const std::string& text) // not reentrant-safe
{
	std::string ipo_price = "";
	std::smatch what;
	static const std::regex regex(REG_EXP);
	if (std::regex_match(text, what, regex))
	{
		ipo_price = what[1];
	}
	return ipo_price;
}

const std::string singleton_match(const std::string& text)
{
	std::string ipo_price = "";
	std::smatch what;
	const std::regex& regex = singleton::get();
	if (std::regex_match(text, what, regex))
	{
		ipo_price = what[1];
	}
	return ipo_price;
}

const std::string factory_match(const std::string& text, const std::regex& regex)
{
	std::string ipo_price = "";
	std::smatch what;
	if (std::regex_match(text, what, regex))
	{
		ipo_price = what[1];
	}
	return ipo_price;
}
