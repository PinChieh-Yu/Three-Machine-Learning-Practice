/**
 * Framework for 2048 & 2048-like Games (C++ 11)
 * use 'g++ -std=c++11 -O3 -g -o 2048 2048.cpp' to compile the source
 *
 * Author: Hung Guei (moporgic)
 *         Computer Games and Intelligence (CGI) Lab, NCTU, Taiwan
 *         http://www.aigames.nctu.edu.tw
 */

#pragma once
#include <iostream>
#include <unordered_map>
#include <vector>
#include <utility>

class weight {
public:
	weight() {}
	//weight(size_t len) : {}
	//weight(weight&& f) : value(std::move(f.value)) {}
	weight(const weight& f) = default;

	weight& operator =(const weight& f) = default;
	//double& operator[] (size_t i) { return table.count(i) > 0 ? table.find(i)->second : 0.0; }
	//const double& operator[] (size_t i) const { return table.count(i) > 0 ? table.find(i)->second : 0.0; }
	size_t size() const { return table.size(); }

public:
	void update(int key, double value) {
		if (table.count(key) > 0) {
			table.find(key)->second += value;
		} else {
			table.emplace(key, value);
		}
	}

	double getvalue(int key) {
		return table.count(key) > 0 ? table.find(key)->second : 0.0;
	}

public:
	friend std::ostream& operator <<(std::ostream& out, const weight& w) {
		//auto& value = w.value;
		auto& table = w.table;
		//uint64_t size = value.size();
		uint64_t size = table.size();
		//out.write(reinterpret_cast<const char*>(&size), sizeof(uint64_t));
		//out.write(reinterpret_cast<const char*>(value.data()), sizeof(float) * size);
		out.write(reinterpret_cast<const char*>(&size), sizeof(uint64_t));
		for (auto it = table.begin(); it != table.end(); ++it) {
			out.write(reinterpret_cast<const char*>(&(it->first)), sizeof(int));
    		out.write(reinterpret_cast<const char*>(&(it->second)), sizeof(double));
		}
		return out;
	}
	friend std::istream& operator >>(std::istream& in, weight& w) {
		//auto& value = w.value;
		auto& table = w.table;
		uint64_t size = 0;
		in.read(reinterpret_cast<char*>(&size), sizeof(uint64_t));
		
		//in.read(reinterpret_cast<char*>(value.data()), sizeof(float) * size);
		int key; double value;
		for (uint64_t i = 0; i < size; ++i) {
			in.read(reinterpret_cast<char*>(&key), sizeof(int));
    		in.read(reinterpret_cast<char*>(&value), sizeof(double));
    		table.emplace(key, value);
		}
		return in;
	}

protected:
	std::unordered_map<int, double> table;
};