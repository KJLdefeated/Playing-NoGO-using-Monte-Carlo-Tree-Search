#pragma once
#include <string>
#include <random>
#include <set>
#include <sstream>
#include <map>
#include <type_traits>
#include <random>
#include <algorithm>
#include <fstream>
#include <thread>
#include <ctime>
#include <queue>
#include "board.h"
#include "action.h"

using namespace std;

default_random_engine engine;

class MCTS_node {
public:
    board::piece_type who;
    board::piece_type me;
    bool terminal=0;
    unsigned int size;
    unsigned int number_of_simulations;
    unsigned int rave_number_of_simulations;
    double rave_score;
    double score;
    board* state;
    vector<MCTS_node*> *child;
    vector<MCTS_node*> Map_Action2Child;
    action::place* move;
    MCTS_node* parent;
    vector<action::place*> *untried_actions;

    MCTS_node(MCTS_node *parent, board* state, action::place* move, board::piece_type who, board::piece_type me)
         : parent(parent), state(state), score(0.0), move(move), number_of_simulations(0), size(0), who(who), me(me) 
         ,rave_number_of_simulations(40), rave_score(32.0){
        untried_actions = new vector<action::place*>;
        Map_Action2Child.resize((board::size_x)*(board::size_y), NULL);
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

    void backpropagate(double w,int n, set<int>* history){

        number_of_simulations += n;
        score += w;

        for(auto i:*history){
            if(Map_Action2Child[i]!=NULL){
                Map_Action2Child[i]->rave_number_of_simulations+=n;
                Map_Action2Child[i]->rave_score+=w;
            }
        }

        if(parent != NULL){
            parent->size++;
            //history->insert(move->position().i);
            parent->backpropagate(w,n,history);
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
        
        MCTS_node* new_node = new MCTS_node(this, next_state, next_move, swt(who), me);
        
        Map_Action2Child[next_move->position().i] = new_node;
        
        set<int>* travelhistory = new set<int>;

        int value = simulate(*new_node->state, new_node->who);

        new_node->backpropagate(value, 1, travelhistory);
        
        travelhistory->clear();
        delete travelhistory;

        child->push_back(new_node);
    }

    //Use to do leaf parallelization
    int rollout(MCTS_node* node, int parallel){
        vector<int> result(parallel,0);
        //simulate(*node->state, node->who);
        int total=0;
        for(auto it:result){
            total+=it;
        }
        return total;
    }

    int simulate(board b, board::piece_type op){
        action::place* mv = get_random_move(b, op);
        while(mv != NULL){
            //s->insert(mv->position().i);
            b.place(mv->position());
            op = swt(op);
            mv = get_random_move(b, op);
        }
        return (op == me);
    }

    action::place* get_random_move(board b, board::piece_type op){
        vector<action::place*> move;
        for(int i = 0 ; i < (board::size_x) * (board::size_y) ; i++){
            board::point mv(i);
            board after = b;
            if(after.place(mv) == board::legal){
                action::place* tmp = new action::place(mv,op);
                move.push_back(tmp);
            }
        }
        shuffle(move.begin(), move.end(), engine);
        if(move.empty()) return NULL;
        for(int i=1;i<move.size();i++)delete move[i];
        return move[0];
    }

    double uct_value(MCTS_node* node, double c, bool RAVE){
        double b = 0.025;
        double beta = 1.0*node->rave_number_of_simulations / (1.0 * node->number_of_simulations + 1.0 * node->rave_number_of_simulations + 4.0 * node->number_of_simulations * node->rave_number_of_simulations * b * b);
        //cout << beta << endl;
        if(!RAVE) beta = 0;
        double winrate = node->score / (1.0 * node->number_of_simulations+1);
        double rave_winrate = node->rave_score / (1.0 * node->rave_number_of_simulations+1);
        double exploitation = (who != me ? (1-beta) * winrate + beta * rave_winrate : (1-beta) * (1-winrate) + beta * (1-rave_winrate));
        double exploration = sqrt(c * log(this->number_of_simulations+1) / (1.0 * node->number_of_simulations + 1));
        return exploitation + exploration;
    }

    MCTS_node* select_best_child(double c, bool RAVE){
        if(child->empty())return NULL;
        double uct, max = -1;
        MCTS_node* best = NULL;

        for(auto *ch:*child){
            if(ch->number_of_simulations == 0) return ch;
            
            uct = uct_value(ch, c, RAVE);
            
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
            if(*ch->state == *b->state){
                next = ch;
            }
            else {
                delete ch;
            }
        }
        this->child->clear();
        if(next == NULL) next = b;
        else next->parent = NULL;
        
        return next;
    }

    board::piece_type swt(board::piece_type a){ return board::piece_type(board::hollow - a); }
};

class MCTS_tree{
public:
    MCTS_tree(){};
    MCTS_tree(board* starting, board::piece_type who, int t, bool rave){
        me = who;
        max_time = t;
        RAVE = rave;
        root = new MCTS_node(NULL, starting, NULL, who, me);
    }
    ~MCTS_tree() {
        delete root;
    }
    MCTS_node* select(double c=2){
        MCTS_node *node = root;

        while(!node->terminal){
            if(!node->is_fully_expanded()) return node;
            else node = node->select_best_child(c, RAVE);
        }
        return node;
    }
    
    MCTS_node* select_best_child(){
        return root->select_best_child(0.0, false);
    }
    int grow(int maxiter, int max_t, double p_stop){
        MCTS_node* node;
        int dt;

        time_t start_t, now_t;
        time(&start_t);
        for(int i=0;i<maxiter;i++){
            
            node = select();

            node->expand();

            int max1=0, max2=0;
            for(auto *ch:*root->child){
                
                int cnt = ch->number_of_simulations;
                
                if(cnt > max1){
                    max2=max1;
                    max1=cnt;
                }
                else if(cnt > max2){
                    max2 = cnt;
                }
            }

            time(&now_t);
            dt = difftime(now_t, start_t);
            
            if(dt > max_t){
                cout << "Early stopping: Made " << (i+1) << "iterations in " << dt << " seconds." << endl;
                break;
            }
        }
        time(&now_t);
        dt = difftime(now_t, start_t);
        return min(dt, max_t);
    }
    void advance_tree(MCTS_node* next){
        MCTS_node* old = root;
        root = root->advance_tree(next);
        delete old;
    }

    int get_simulation_cnt(int i){
        if(root->Map_Action2Child[i]==NULL) return 0;
        return root->Map_Action2Child[i]->number_of_simulations;
    }

    double get_winrate(int i){
        if(root->Map_Action2Child[i]==NULL) return 0;
        return root->uct_value(root->Map_Action2Child[i], 0, false);
    }

public:
    board::piece_type who;
    board::piece_type me;
    MCTS_node *root=NULL;
    int max_time;
    bool RAVE;
};