/**
 * Framework for NoGo and similar games (C++ 11)
 * agent.h: Define the behavior of variants of the player
 *
 * Author: Theory of Computer Games
 *         Computer Games and Intelligence (CGI) Lab, NYCU, Taiwan
 *         https://cgilab.nctu.edu.tw/
 */

#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <algorithm>
#include <fstream>
#include "board.h"
#include "action.h"
#include "MCTS.h"

class agent {
public:
	agent(const std::string& args = "") {
		std::stringstream ss("name=unknown role=unknown " + args);
		for (std::string pair; ss >> pair; ) {
			std::string key = pair.substr(0, pair.find('='));
			std::string value = pair.substr(pair.find('=') + 1);
			meta[key] = { value };
			//cout << key << ' ' << value << endl;
		}
	}
	virtual ~agent() {}
	virtual void open_episode(const std::string& flag = "") {}
	virtual void close_episode(const std::string& flag = "") {}
	virtual action take_action(const board& b) { return action(); }
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

/**
 * base agent for agents with randomness
 */
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
 * random player for both side
 * put a legal piece randomly
 */
class player : public random_agent {
public:
	player(const std::string& args = "") : random_agent("name=random role=unknown " + args),
		space(board::size_x * board::size_y), who(board::empty) {
		if (name().find_first_of("[]():; ") != std::string::npos)
			throw std::invalid_argument("invalid name: " + name());
		if (role() == "black") who = board::black;
		if (role() == "white") who = board::white;
		if (meta.count("mcts")) search_algo = "mcts";
		if (meta.count("simu")) max_iter = meta["simu"];
		if (meta.count("time")) max_sec = meta["time"];
		if (who == board::empty)
			throw std::invalid_argument("invalid role: " + role());
		for (size_t i = 0; i < space.size(); i++)
			space[i] = action::place(i, who);
	}

	virtual action take_action(const board& state) {
		if (search_algo == "mcts") {
		    return mcts_action(state);
		} else {
		    return random_action(state);
		}
	}

	action random_action(const board& state){
		std::shuffle(space.begin(), space.end(), engine);
		for (const action::place& move : space) {
			board after = state;
			if (move.apply(after) == board::legal)
				return move;
		}
		return action();
	}

	action mcts_action(const board& st){
		board* b = new board(st);
		MCTS_node* tmp = new MCTS_node(NULL, b, NULL, who, who);

		if(!b->check_empty() && tree != NULL) tree->advance_tree(tmp);
		else tree = new MCTS_tree(b, who);

		if(tree->root->terminal) return action();
		//cout << "_______________" << endl;cout << "Growing tree..." << endl;
		tree->grow(max_iter, max_sec);
		//cout << "Tree size:" << tree->size() << endl;cout << "_______________" << endl;
		MCTS_node *best_child = tree->select_best_child();
		if(best_child == NULL){
			cout << "Warning: Tree root has no children! Possibly terminal node!" << endl;
			return action();
		}
		tree->advance_tree(best_child);
		return *best_child->move;
	}

private:
	MCTS_tree* tree = NULL;
	string search_algo="";
	int max_iter=100, max_sec=40;
	std::vector<action::place> space;
	board::piece_type who;
};

