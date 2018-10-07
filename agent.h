#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <vector>
#include <type_traits>
#include <algorithm>
#include "board.h"
#include "action.h"

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

/**
 * dummy player
 * select a legal action randomly
 */
class player : public random_agent {
public:
	player(const std::string& args = "") : random_agent("name=dummy role=player " + args),
		opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before, std::string last_op) {
		std::shuffle(opcode.begin(), opcode.end(), engine);
		for (int op : opcode) {
			board::reward reward = board(before).slide(op);
			if (reward != -1) return action::slide(op);
		}
		return action();
	}

private:
	std::array<int, 4> opcode;
};

class smarter_player : public random_agent {
public:
	smarter_player(const std::string& args = "") : random_agent("name=smarter role=player " + args),
		opcode({ 2, 1, 3, 0 }) {}

	virtual action take_action(const board& before, std::string last_op) {
		for (int op : opcode) {
			board::reward reward = board(before).slide(op);
			if (reward != -1) return action::slide(op);
		}
		return action();
	}

private:
	std::array<int, 4> opcode;
};

class greedy_player : public random_agent {
public:
	greedy_player(const std::string& args = "") : random_agent("name=greedy role=player " + args),
		opcode({ 0, 1, 2, 3 }) {}

	virtual action take_action(const board& before, std::string last_op) {
		int best_op;
		board::reward best_reward = -1, reward;
		for (int op : opcode) {
			reward = board(before).slide(op);
			if (reward > best_reward) {
				best_op = op;
				best_reward = reward;
			}
		}
		return best_reward != -1 ? action::slide(best_op) : action();
	}

private:
	std::array<int, 4> opcode;
};