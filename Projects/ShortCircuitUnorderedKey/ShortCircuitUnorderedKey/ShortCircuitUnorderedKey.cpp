// ShortCircuitUnorderedKey.cpp : Defines the entry point for the console application.
//

#include <vector>
#include <iostream>
#include <iomanip>
#include <string>
#include <cassert>
#include <chrono>
#include <unordered_map>
#include <random>
#include <numeric>
#include <algorithm>

using namespace std;

class timer
{
public:
	timer() = default;
	void start_timing(const string& text_)
	{
		text = text_;
		begin = chrono::high_resolution_clock::now();
	}
	void stop_timing()
	{
		auto end = chrono::high_resolution_clock::now();
		auto dur = end - begin;
		auto ms = chrono::duration_cast<chrono::milliseconds>(dur).count();
		cout << setw(38) << text << ":" << setw(5) << ms << "ms" << endl;
	}

private:
	string text;
	chrono::steady_clock::time_point begin;
};

struct KeyHasher
{
	size_t operator()(const size_t& k) const
	{
		return k;
	}
};

typedef unordered_map<size_t, size_t> uint_map_type;
typedef unordered_map<size_t, size_t, KeyHasher> kuint_map_type;

void init_vector(vector<size_t>& v, const size_t start_number)
{
	iota(v.begin(), v.end(), start_number);

	random_device rd;
	mt19937 g(rd());

	std::shuffle(v.begin(), v.end(), g);

	//std::copy(v.begin(), v.end(), std::ostream_iterator<int>(std::cout, " "));
	//std::cout << "\n";
}

void test_hash_fn()
{
	uint_map_type uint_map;

	uint_map_type::hasher uint_map_fn = uint_map.hash_function();

	std::cout << "uint_map_fn(1234)  returns " << uint_map_fn(1234) << std::endl;

	KeyHasher kuint_map_fn;

	std::cout << "kuint_map_fn(1234) returns " << kuint_map_fn(1234) << std::endl;
}

void benchmark();

int main()
{
	//test_hash_fn();
	benchmark();

	return 0;
}

void benchmark()
{
	constexpr size_t COUNT = 1000000;
	constexpr size_t START = 128569;
	vector<size_t> v(COUNT, 0);
	init_vector(v, START);

	uint_map_type uint_map;
	kuint_map_type kuint_map;

	timer stopwatch;

	stopwatch.start_timing("std hash:insertion");
	for (size_t n : v)
	{
		auto res = uint_map.insert(std::make_pair(n, n));
		assert(res.second);
	}
	stopwatch.stop_timing();

	stopwatch.start_timing("custom hash:insertion");
	for (size_t n : v)
	{
		auto res = kuint_map.insert(std::make_pair(n, n));
		assert(res.second);
	}
	stopwatch.stop_timing();

	size_t val = 0;

	stopwatch.start_timing("std hash:lookup");
	for (size_t n : v)
	{
		 val = uint_map[n];
		 assert(val==n);
	}
	stopwatch.stop_timing();

	val = 0;

	stopwatch.start_timing("custom hash:lookup");
	for (size_t n : v)
	{
		val = kuint_map[n] = n;
		assert(val == n);
	}
	stopwatch.stop_timing();
}

