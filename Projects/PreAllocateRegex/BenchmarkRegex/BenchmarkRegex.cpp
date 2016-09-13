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
#include <map> 
#include <thread>
#include <mutex>

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
	static void set(const std::regex* obj_ptr) { s_regex_ptr = obj_ptr; }
	static const std::regex* get()
	{
		std::lock_guard<std::mutex> lock(s_mutex);

		std::thread::id thread_id = std::this_thread::get_id();
		std::map<std::thread::id, const std::regex*>::iterator it = s_map.find(thread_id);
		if (it == s_map.end())
		{
			const std::regex* ptr = new std::regex(*s_regex_ptr);
			s_map[thread_id] = ptr;
			return ptr;
		}
		return it->second;
	}
private:
	static std::map<std::thread::id, const std::regex*> s_map;
	static const std::regex* s_regex_ptr;
	static std::mutex s_mutex;
};
std::map<std::thread::id, const std::regex*> singleton::s_map;
const std::regex* singleton::s_regex_ptr = nullptr;
std::mutex singleton::s_mutex;

const std::string local_match(const std::string& text);
const std::string static_match(const std::string& text); // not reentrant-safe
const std::string singleton_match(const std::string& text);

int main(int argc, char* argv[])
{
	const int LOOP = 100000;
	std::string str1 = "Zoomer PRICE: HK$1.23 PER SHARE";
	std::string str2 = "Boomer PRICE: HK$4.56 PER SHARE";

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

	const std::regex regex(".*PRICE:.*HK\\$(\\d+\\.\\d+|[-+]*\\d+).*PER SHARE");
	singleton::set(&regex);

	stopwatch.start_timing("singleton regex object");
	for (int j = 0; j < LOOP; ++j)
	{
		for (size_t i = 0; i < vec.size(); ++i)
		{
			do_not_optimize_away(singleton_match(vec[i]).c_str());
		}
	}
	stopwatch.stop_timing();

	/*
	std::cout << local_match(str1) << std::endl;
	std::cout << local_match(str2) << std::endl;
	std::cout << static_match(str1) << std::endl;
	std::cout << static_match(str2) << std::endl;
	std::cout << singleton_match(str1) << std::endl;
	std::cout << singleton_match(str2) << std::endl;
	*/

	return 0;
}

const std::string local_match(const std::string& text)
{
	std::string ipo_price = "";
	std::smatch what;
	const std::regex regex(".*PRICE:.*HK\\$(\\d+\\.\\d+|[-+]*\\d+).*PER SHARE");
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
	static const std::regex regex(".*PRICE:.*HK\\$(\\d+\\.\\d+|[-+]*\\d+).*PER SHARE");
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
	const std::regex* regex_ptr = singleton::get();
	if (std::regex_match(text, what, *regex_ptr))
	{
		ipo_price = what[1];
	}
	return ipo_price;
}
