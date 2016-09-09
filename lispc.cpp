#include <string>
#include <iostream>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <stdexcept>
#include <map>
#include <climits>

typedef uintptr_t value;

const uintptr_t PAIR_TAG = 0;
const uintptr_t NUMBER_TAG = 1;
const uintptr_t SYMBOL_TAG = 2;
const uintptr_t FN_TAG = 3;

bool is_number(value v){
    return (v & 3) == NUMBER_TAG;
}

bool is_symbol(value v){
    return (v & 3) == SYMBOL_TAG;
}

bool is_pair(value v){
    return (v & 3) == PAIR_TAG;
}

bool is_fn(value v){
    return (v & 3) == FN_TAG;
}

value int2value(int64_t n){
    return (n << 2) | NUMBER_TAG;
}

int64_t value2int(value n){
    int64_t signd = n;
    return (signd >> 2);
}

std::vector<std::string> symbol_table;

value get_symbol(std::string name){
    auto itr = std::find(symbol_table.begin(), symbol_table.end(), name);
    int index;

    if(itr != symbol_table.end()){
        index = itr - symbol_table.begin(); 
    }
    else{
        index = symbol_table.size();
        symbol_table.push_back(name);
    }
    return (index << 2) | SYMBOL_TAG;
}

const value nil = get_symbol("nil");

std::string get_symbol_name(value symbol){
    return symbol_table[symbol >> 2]; 
}

value make_pair(value car, value cdr){
    value* ptr = (value*) malloc(2 * sizeof(value));
    ptr[0] = car;
    ptr[1] = cdr;
    return reinterpret_cast<value>(ptr);
}

value build_list(std::vector<value>& contents, value end = nil) {
    value list = end;
    for (auto iter = contents.rbegin(); iter != contents.rend(); iter++) {
        list = make_pair(*iter, list);
    }
    return list; 
}

value car(value pair){
    return reinterpret_cast<value*>(pair)[0];
}

value cdr(value pair){
    return reinterpret_cast<value*>(pair)[1];
}

int list_length(value v){ 
    int out = 0;
    while(1){
        if(v == nil)
            return out;
        else if(!is_pair(v)){
            return -1;
        }
        else{
            out++;
            v = cdr(v);
        }
    }
}

typedef value(*fn_ptr)(value);

struct Fn{
    fn_ptr f;
};

value make_fn(fn_ptr f){
    Fn* mem = (Fn*)malloc(sizeof(Fn));
    mem->f = f; 
    return reinterpret_cast<value>(mem) | FN_TAG;
}

value call_fn(value fn, value args){
    Fn* mem = reinterpret_cast<Fn*>(fn & (~3));
    return mem->f(args);
}

std::string value2string(value v){
    if(is_number(v)){
        return std::to_string(value2int(v));
    }
    else if(is_symbol(v)){
        return get_symbol_name(v);
    }
    else if(is_pair(v)){
        std::stringstream ss;
        ss << "(" << value2string(car(v));
        while(true){
            value vcdr = cdr(v);
            if(vcdr == nil) 
                break;
            if(!is_pair(vcdr)){
                ss << " . " << value2string(vcdr);
                break;
            }
            ss << " " << value2string(car(vcdr));
            v = vcdr;
        }
        ss << ")";
        return ss.str();
    } else if (is_fn(v)) {
        return "<fn>";
    }
}

void skip_space(const std::string& input, size_t& offset){
    while((offset < input.size()) && std::isspace(input[offset])){
        offset++;
    } 
}

char next_char(const std::string& input, size_t& offset) {
    skip_space(input, offset);
    if(offset == input.size()){
        throw std::runtime_error("unexpected end of input!");
    }
    return input[offset];
}

bool is_special(char c){
    return (c == '(') || (c == ')') || std::isspace(c);
}

value parse_expression(const std::string& input, size_t& offset){
    char next = next_char(input, offset); 
    if(next == '('){
        std::vector<value> contents;
        value end = nil;
        offset++;
        while(true){
            char next = next_char(input, offset);
            if(next == ')'){
                offset++;
                break;
            } 
            else if(next == '.'){
                if(contents.size() == 0)
                    throw std::runtime_error("leading . in list");
                offset++;
                end = parse_expression(input, offset);
                if(next_char(input, offset) != ')')
                    throw std::runtime_error("improper improper list");
                offset++;
                break; 
            }
            else{
                contents.push_back(parse_expression(input, offset));
            }
        }
        return build_list(contents, end);
    }
    else if(next == ')'){
        throw std::runtime_error("improper end to list");
    }
    else{
        bool saw_non_digit = false; 
        size_t start_ind = offset;
        while(offset < input.size() && !is_special(input[offset])){ 
            if(!std::isdigit(input[offset]))
                saw_non_digit = true;
            offset++;
        }
        std::string token = input.substr(start_ind, offset - start_ind);

        if(saw_non_digit){
            return get_symbol(token); 
        }
        else{
            return int2value(std::stoi(token));
        }
    }
}

value parse(std::string input){
    size_t v = 0;
    return parse_expression(input, v);
}

std::vector<value> parse_body(std::string input){
    size_t offset = 0;
    std::vector<value> result;
    while(true) {
        skip_space(input, offset);
        if (offset == input.size()) return result;
        result.push_back(parse_expression(input, offset));
    }
}

void check_args(value args, int len_req){
    if(list_length(args) != len_req){
        throw std::runtime_error("args length incorrect");
    } 
}

std::map<value, value> env;
value plus(value args){
    int v = 0;
    while(args != nil){
        if(!is_number(car(args))){
            throw std::runtime_error("arg to plus not number"); 
        }
        v += value2int(car(args));
        args = cdr(args);
    }
    return int2value(v); 
}

value minus(value args){
    int v = 0;
    bool init = false;
    while(args != nil){
        if(!is_number(car(args))){
            throw std::runtime_error("arg to minus not number"); 
        }
        if(!init){
            init = true;
            v = value2int(car(args));
        }
        else{
            v -= value2int(car(args));
        }
        args = cdr(args);
    }
    return int2value(v); 
}

value mul(value args){
    int v = 1;
    while(args != nil){
        if(!is_number(car(args))){
            throw std::runtime_error("arg to mul not number"); 
        }
        v *= value2int(car(args));
        args = cdr(args);
    }
    return int2value(v); 
}

value div(value args){
    int v = 0;
    int nv;
    bool init = false;
    while(args != nil){
        if(!is_number(car(args))){
            throw std::runtime_error("arg to div not number"); 
        }
        if(!init){
            init = true;
            v = value2int(car(args));
            std::cout << v << std::endl;
        }
        else{
            nv = value2int(car(args)); 
            if(nv == 0){
                throw std::runtime_error("div by 0");
            }
            v = v / nv; 
        }
        args = cdr(args);
    }
    return int2value(v); 
}

void init_env() {
    env[get_symbol("+")] = make_fn(&plus);
    env[get_symbol("-")] = make_fn(&minus);
    env[get_symbol("*")] = make_fn(&mul);
    env[get_symbol("/")] = make_fn(&div);
    env[nil] = nil;
    env[get_symbol("t")] = get_symbol("t");
}

value sym_if = get_symbol("if");
value sym_define = get_symbol("define");

value nth(value v, size_t n){
    while(n > 0){
        v = cdr(v);
        n--;
    }
    return car(v);
}

value evaluate(value v){
    if(is_number(v) || is_fn(v))
        return v;
    else if(is_symbol(v)){
        auto itr = env.find(v);
        if(itr == env.end()){
            throw std::runtime_error("undefined variable "+get_symbol_name(v));
        }
        return itr->second;
    }
    else if(is_pair(v)){
        //car of this must be a function
        value head = car(v);
        value args = cdr(v);
        if(head == sym_if){
            check_args(args, 3);
            return evaluate(nth(args, evaluate(nth(args,0)) == nil ? 2 : 1));
        } 
        else if(head == sym_define){
            check_args(args, 2);
            if(!is_symbol(nth(args, 0))) 
                throw std::runtime_error("first argument of define not symbol!");
            env[nth(args, 0)] = evaluate(nth(args, 1)); 
            return nil;
        }
        else{
            value f = evaluate(head);
            if(!is_fn(f))
                throw std::runtime_error("calling a non-function");
            if(list_length(args) == -1)
                throw std::runtime_error("fn call with improper list");
            std::vector<value> arg_values;
            for (auto cur = args; cur != nil; cur = cdr(cur))
                arg_values.push_back(evaluate(car(cur)));
            return call_fn(f, build_list(arg_values));
        }
    }
    else{
        throw std::runtime_error("unknown value type");
    }
}


int main(){
    init_env();
    std::string buf;
    while(true){
        std::cout << ">  ";
        //std::getline(std::cin, buf, '\0');
        std::getline(std::cin, buf);
        if(buf == "exit"){
            std::cout << "exiting..." << std::endl;
            break;
        }      
        auto input = parse_body(buf);
        value result = nil;
        for (auto iter = input.begin(); iter != input.end(); iter++)
            result = evaluate(*iter);
        std::cout << value2string(result) << std::endl;
        //std::cin.ignore(INT_MAX);
    }
    return 0;

    //  std::cout << "begin" << std::endl;
    //  std::getline(std::cin, buf, '\0');
    //  std::cout << "entered command" << std::endl;
    //  auto input = parse_body(buf);
    //  value result = nil;
    //  for (auto iter = input.begin(); iter != input.end(); iter++)
    //    result = evaluate(*iter);
    //  std::cout << value2string(result) << std::endl;
    //  return 0;
}
