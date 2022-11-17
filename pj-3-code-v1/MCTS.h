#pragma once
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <type_traits>
#include <random>
#include <algorithm>
#include <fstream>
#include <queue>
#include "board.h"
#include "action.h"

using namespace std;

class MCTS_node {
public:
    std::default_random_engine engine;
    board::piece_type who;
    board::piece_type me;
    bool terminal=0;
    unsigned int size;
    unsigned int number_of_simulations;
    double score;
    board* state;
    vector<MCTS_node*> *child;
    action::place* move;
    MCTS_node* parent;
    vector<action::place*> *untried_actions;
    const int starting_number_of_children = 32;

    MCTS_node(MCTS_node *parent, board* state, action::place* move, board::piece_type who, board::piece_type me)
         : parent(parent), state(state), score(0.0), move(move), number_of_simulations(0), size(0), who(who), me(me){
        untried_actions = new vector<action::place*>;
        for(int i=0;i < (board::size_x)*(board::size_y);i++){
            board::point mv(i);
            board after = *state;
            if(after.place(mv) == board::legal){
                action::place* tmp = new action::place(mv,who);
                untried_actions->push_back(tmp);
            }
        }
        if(untried_actions->size() == 0)terminal = 1;
        shuffle(untried_actions->begin(), untried_actions->end(), engine);
        child = new vector<MCTS_node*>;
    }
    ~MCTS_node(){
        delete state;
        delete move;
        for(auto *ch:*child) delete ch;
        delete child;
        while(!untried_actions->empty()) {
            delete untried_actions->back();
            untried_actions->pop_back();
        }
        delete untried_actions;
    }
    bool is_fully_expanded(){
        return terminal || untried_actions->empty();
    }
    void backpropagate(double w,int n){
        score += w;
        number_of_simulations += n;
        if(parent != NULL){
            parent->size++;
            parent->backpropagate(w,n);
        }
    }
    void expand(){
        if(terminal){
            return;
        }
        else if(is_fully_expanded()){
            return;
        }
        action::place* next_move = untried_actions->back();
        untried_actions->pop_back();
        
        board* next_state = new board(*state);
        next_state->place(next_move->position());
        
        MCTS_node* new_node = new MCTS_node(this, next_state, next_move, (who == board::black ? board::white : board::black), me);
        //cout << new_node->move->position() << endl;
        int value = simulate(*new_node->state, new_node->who);
        //cout << value << endl;
        new_node->backpropagate(value, 1);

        child->push_back(new_node);
    }

    int simulate(board b, board::piece_type op){
        action::place* mv = get_random_move(b, op);
        while(mv != NULL){
            b.place(mv->position());
            op = (op == board::black ? board::white : board::black);
            mv = get_random_move(b, op);
        }
        return op == me;
    }

    action::place* get_random_move(board b, board::piece_type op){
        vector<action::place*> move;
        for(int i=0;i < (board::size_x)*(board::size_y);i++){
            board::point mv(i);
            board after = b;
            if(after.place(mv) == board::legal){
                action::place* tmp = new action::place(mv,op);
                move.push_back(tmp);
            }
        }
        shuffle(move.begin(), move.end(), engine);
        if(move.empty()) return NULL;
        return move[0];
    }

    MCTS_node* select_best_child(double c){
        if(child->empty())return NULL;
        double uct, max = -1;
        MCTS_node* best = NULL;

        for(auto *ch:*child){
            if(ch->number_of_simulations == 0) return ch;

            double winrate = ch->score / (1.0 * ch->number_of_simulations);
            if(who == me) winrate = 1.0 - winrate;
            
            uct = winrate + sqrt(c*log(1.0*this->number_of_simulations) / (1.0 * ch->number_of_simulations));

            if(uct > max){
                max = uct;
                best = ch;
            }
        }
        return best;
    }
    MCTS_node* advance_tree(MCTS_node* b){
        MCTS_node *next = NULL;
        for(auto *ch:*child){
            if(ch == b){
                next = ch;
            }
            else {
                delete ch;
            }
        }
        this->child->clear();
        if(next == NULL) next = new MCTS_node(NULL, b->state, NULL, (who == board::black ? board::white : board::black), me);
        else next->parent = NULL;
        return next;
    }
};

class MCTS_tree{
    public:
    board::piece_type who;
    board::piece_type me;
    MCTS_node *root;
    MCTS_tree(board* starting, board::piece_type who){
        me = who;
        root = new MCTS_node(NULL, starting, NULL, who, me);
    }
    ~MCTS_tree() {
        delete root;
    }
    MCTS_node* select(double c=2){
        MCTS_node *node = root;

        while(!node->terminal){
            if(!node->is_fully_expanded()) return node;
            else node = node->select_best_child(c);
        }
        return node;
    }
    
    MCTS_node* select_best_child(){
        return root->select_best_child(0.0);
    }
    void grow(int maxiter, double max_time){
        MCTS_node* node;
        double dt;

        time_t start_t, now_t;
        time(&start_t);
        for(int i=0;i<maxiter;i++){
            node = select();
            node->expand();

            time(&now_t);
            dt = difftime(now_t, start_t);
            if(dt > max_time){
                cout << "Early stopping: Made " << (i+1) << "iterations in " << dt << " seconds." << endl;
                break;
            }
        }
        //time(&now_t);dt = difftime(now_t, start_t);cout << "Finished in " << dt << "seconds." <<endl;
    }
    void advance_tree(MCTS_node* next){
        MCTS_node* old = root;
        root = root->advance_tree(next);
        delete old;
    }
    unsigned int size(){return root->size;}
};