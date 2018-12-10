#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include "board.h"
#include "action.h"
#include "weight.h"
#include <fstream>

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b, std::string last_op) { return action(); }
	virtual bool check_for_win(const board& b) { return false; }

public:
	virtual std::string property(const std::string& key) const { return meta.at(key); }
	virtual void notify(const std::string& msg) { meta[msg.substr(0, msg.find('='))] = { msg.substr(msg.find('=') + 1) }; }
	virtual std::string name() const { return property("name"); }
	virtual std::string role() const { return property("role"); }

protected:
	typedef std::string key;
	struct value {
		std::string value;
		operator std::string() const { return value; }
		template<typename numeric, typename = typename std::enable_if<std::is_arithmetic<numeric>::value, numeric>::type>
		operator numeric() const { return numeric(std::stod(value)); }
	};
	std::map<key, value> meta;
};

class random_agent : public agent {
public:
	random_agent(const std::string& args = "") : agent(args) {
		if (meta.find("seed") != meta.end())
			engine.seed(int(meta["seed"]));
	}
	virtual ~random_agent() {}

protected:
	std::default_random_engine engine;
};

/**
 * base agent for agents with weight tables
 */
class weight_agent : public agent {
public:
	weight_agent(const std::string& args = "") : agent(args), alpha(0.1f) {
		if (meta.find("init") != meta.end()) // pass init=... to initialize the weight
			init_weights(meta["init"]);
		if (meta.find("load") != meta.end()) // pass load=... to load from a specific file
			load_weights(meta["load"]);
		if (meta.find("alpha") != meta.end())
			alpha = float(meta["alpha"]);
	}
	virtual ~weight_agent() {
		if (meta.find("save") != meta.end()) // pass save=... to save to a specific file
			save_weights(meta["save"]);
	}

protected:
	virtual void init_weights(const std::string& info) {
		net.emplace_back(0xFFFFFF+1); // create an empty weight table with size 65536
		net.emplace_back(0xFFFFFF+1); // create an empty weight table with size 65536
		net.emplace_back(0xFFFFFF+1); // create an empty weight table with size 65536
		net.emplace_back(0xFFFFFF+1); // create an empty weight table with size 65536
	}
	virtual void load_weights(const std::string& path) {
		std::ifstream in(path, std::ios::in | std::ios::binary);
		if (!in.is_open()) std::exit(-1);
		uint32_t size;
		in.read(reinterpret_cast<char*>(&size), sizeof(size));
		net.resize(size);
		for (weight& w : net) in >> w;
		in.close();
	}
	virtual void save_weights(const std::string& path) {
		std::ofstream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
		if (!out.is_open()) std::exit(-1);
		uint32_t size = net.size();
		out.write(reinterpret_cast<char*>(&size), sizeof(size));
		for (weight& w : net) out << w;
		out.close();
	}

protected:
	std::vector<weight> net;
	float alpha;
};

/**
 * base agent for agents with a learning rate
 */
class learning_agent : public agent {
public:
	learning_agent(const std::string& args = "") : agent(args), alpha(0.1f) {
		if (meta.find("alpha") != meta.end())
			alpha = float(meta["alpha"]);
	}
	virtual ~learning_agent() {}

protected:
	float alpha;
};

class player : public weight_agent {
public:
	player(const std::string& args = "") : weight_agent("name=weight role=player " + args),
		opcode({ 0, 1, 2, 3 }), tuples({{
		{{{0,1,2,3,4,5},{4,5,6,7,8,9},{7,6,5,11,10,9},{15,14,13,11,10,9}}},
		{{{3,7,11,15,2,6},{2,6,10,14,1,5},{14,10,6,13,9,5},{12,8,4,13,9,5}}},
		{{{15,14,13,12,11,10},{11,10,9,8,7,6},{8,9,10,4,5,6},{0,1,2,4,5,6}}},
		{{{12,8,4,0,13,9},{13,9,5,1,14,10},{1,5,9,2,6,10},{3,7,11,2,6,10}}},
		{{{3,2,1,0,7,6},{7,6,5,4,11,10},{4,5,6,8,9,10},{12,13,14,8,9,10}}},
		{{{15,11,7,3,14,10},{14,10,6,2,13,9},{2,6,10,1,5,9},{0,4,8,1,5,9}}},
		{{{12,13,14,15,8,9},{8,9,10,11,4,5},{11,10,9,7,6,5},{3,2,1,7,6,5}}},
		{{{0,4,8,12,1,5},{1,5,9,13,2,6},{13,9,5,14,10,6},{15,11,7,14,10,6}}}}}) {}

	virtual action take_action(const board& before, std::string last_op) {

		board after;
		int hash; 
		int reward, final_reward = 0;
		int final_op = -1; 
		float value, highest_value = -2147483648;

		for (int op : opcode) { // four direction
			after = board(before);
			reward = after.slide(op);
			if (reward != -1) {
				// hash the tuple
				value = reward;
				for(int i = 0; i < 8; i++){
					for(int j = 0; j < 4; j++){
						hash = 0;
						for(int k = 0; k < 6; k++){
							hash = hash * 16 + after(tuples[i][j][k]);
						}
						value += net[j][hash];
					}
				}
				if (highest_value < value) {
					final_op = op;
					final_reward = reward;
					highest_value = value;
				}
			}
		}

		if(final_op != -1){
			after = board(before);
			after.slide(final_op);
			board_records.push_back(after);
			reward_records.push_back(final_reward);
			return action::slide(final_op);
		}

		return action();
	}

	void backward_train() {
		int hash;
		board cur, prev;
		float cur_value, pre_value, result;

		//update end board
		prev = board_records.back();
		pre_value = 0;
		for(int i = 0; i < 8; i++){
			for(int j = 0; j < 4; j++){
				hash = 0;
				for(int k = 0; k < 6; k++){
					hash = hash * 16 + prev(tuples[i][j][k]);
				}
				pre_value += net[j][hash];
			}
		}
		result = (0 - pre_value) * alpha / (8 * 4 * 6);
		for(int i = 0; i < 8; i++){
			for(int j = 0; j < 4; j++){
				hash = 0;
				for(int k = 0; k < 6; k++){
					hash = hash * 16 + prev(tuples[i][j][k]);
				}
				net[j][hash] += result;
			}
		}

		//start backward train
		while (board_records.size() > 1) {
			cur = board_records.back();
			board_records.pop_back();
			prev = board_records.back();

			cur_value = 0;
			for(int i = 0; i < 8; i++){
				for(int j = 0; j < 4; j++){
					hash = 0;
					for(int k = 0; k < 6; k++){
						hash = hash * 16 + cur(tuples[i][j][k]);
					}
					cur_value += net[j][hash];
				}
			}

			cur_value += reward_records.back();
			reward_records.pop_back();

			pre_value = 0;
			for(int i = 0; i < 8; i++){
				for(int j = 0; j < 4; j++){
					hash = 0;
					for(int k = 0; k < 6; k++){
						hash = hash * 16 + prev(tuples[i][j][k]);
					}
					pre_value += net[j][hash];
				}
			}

			result = (cur_value - pre_value) * alpha / (8 * 4 * 6);

			for(int i = 0; i < 8; i++){
				for(int j = 0; j < 4; j++){
					hash = 0;
					for(int k = 0; k < 6; k++){
						hash = hash * 16 + prev(tuples[i][j][k]);
					}
					net[j][hash] += result;
				}
			}
		}

		reward_records.clear();
		board_records.clear();
	}

private:
	std::array<int, 4> opcode;
	std::vector<int> reward_records;
	std::vector<board> board_records;
	std::array<std::array<std::array<int, 6>, 4>, 8> tuples;
};
/**
 * random environment
 * add a new random tile to an empty cell
 * 2-tile: 90%
 * 4-tile: 10%
 */
class rndenv : public random_agent {
public:
	rndenv(const std::string& args = "") : random_agent("name=random role=environment " + args),
		space({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 }), left_edge({0, 4, 8, 12}), right_edge({3, 7, 11, 15}), 
		up_edge({0, 1, 2, 3}), down_edge({12, 13, 14, 15}) {}

	virtual action take_action(const board& after, std::string last_op) {
		int* data;
		int length;
		if (last_op == "#U") {
			data = down_edge.data();
			length = 4;
		} else if (last_op == "#D") {
			data = up_edge.data();
			length = 4;
		} else if (last_op == "#R") {
			data = left_edge.data();
			length = 4;
		} else if (last_op == "#L") {
			data = right_edge.data();
			length = 4;
		} else {
			data = space.data();
			length = 16;
		}

		std::shuffle(data, data+length, engine);

		for (int i = 0; i < length; i++) {
			int pos = data[i];
			if (after(pos) != 0) continue;
			if (bag.empty()) {
				bag.push_back(1);
				bag.push_back(2);
				bag.push_back(3);
				std::shuffle(bag.begin(), bag.end(), engine);
			}
			board::cell tile = bag.back();
			bag.pop_back();
			return action::place(pos, tile);
		}
		return action();
	}

	void reset_bag(){
		bag.clear();
	}

private:
	std::array<int, 16> space;
	std::array<int, 4> left_edge;
	std::array<int, 4> right_edge;
	std::array<int, 4> up_edge;
	std::array<int, 4> down_edge;
	std::vector<int> bag;
};
