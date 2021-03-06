#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#include <iostream>
#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <stack>

typedef uint8_t word;
typedef std::function<void(void)> vv;

word current_byte();
word eat_byte();
bool is_whitespace(word c);
void advance_past_whitespace();
void advance_to_whitespace();
word get_token();
void eat_comment();
vv push_dataspace_label(uint32_t n);
void define(word name, vv action);
void dataspace_label();
vv call_function(uint32_t n);
void define_function();
uint32_t read_number();
void literal_byte_compile();
std::vector<word> as_bytes(uint32_t n);
void literal_word();
void allocate_space();
void set_start_address();
void nop();
void skip_literal_byte();
void start_conditional();
void end_conditional();
const vv start_loop = start_conditional;
void end_loop();
void tbfcompile();
void write_out();
void quit();
void subtract();
void push_literal();
int32_t decode(std::vector<word> bytes);
void fetch();
void extend_memory(uint32_t addr);
void store();
void store_byte();
void less_than();
void return_from_function();
void read_byte();
void jump();
void conditional();
void loop();
void literal_byte_run();
void tbfrun();

void debug(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

template<typename T>
T pop(std::stack<T> &v) {
    assert(!v.empty());
    T val = v.top();
    v.pop();
    return val;
}

template<typename K, typename V>
bool intable(K &key, std::unordered_map<K, V> &table) {
    /* These are both correct but I can't decide which is more readable. */
    return table.count(key) > 0;
    return table.find(key) != table.end();
}

std::vector<word> memory;
std::stack<uint32_t> stack;
std::stack<uint32_t> rstack;

std::vector<word> program;
uint32_t pc = 0;
int64_t start_address = -1;

std::unordered_map<uint32_t, uint32_t> jump_targets;

std::unordered_map<word,vv> compile_time_dispatch = {
    {'(', eat_comment},
    {'v', dataspace_label},
    {':', define_function},
    {'b', literal_byte_compile},
    {'#', literal_word},
    {'*', allocate_space},
    {'^', set_start_address},
    {'[', start_conditional}, {']', end_conditional},
    {'{', start_loop}, {'}', end_loop},
    {' ', eat_byte}, {'\n', eat_byte},
    {'\'', skip_literal_byte},
};

std::unordered_map<word, vv> run_time_dispatch = {
    {'(', jump},
    {'W', write_out},
    {'G', read_byte},
    {'Q', quit},
    {'-', subtract},
    {'<', less_than},
    {'@', fetch},
    {'!', store},
    // {'f', fetch_byte}, not yet needed
    {'s', store_byte},
    {';', return_from_function},
    {'[', conditional}, {']', nop},
    {'{', nop}, {'}', loop},
    {' ', nop}, {'\n', nop},
    {'\'', literal_byte_run},
};

word current_byte() {
    return program.at(pc);
}

word eat_byte() {
    return program.at(pc++);
}

bool is_whitespace(word c) {
    return c == ' ' || c == '\n' || c == '\t';
}

void advance_past_whitespace() {
    while (pc < program.size() && is_whitespace(current_byte())) {
        eat_byte();
    }
}

void advance_to_whitespace() {
    while (pc < program.size() && !is_whitespace(current_byte())) {
        eat_byte();
    }
}

word get_token() {
    advance_past_whitespace();
    word rv = current_byte();
    if (!isdigit(rv) && rv != '\'') advance_to_whitespace();
    return rv;
}

void eat_comment() {
    uint32_t comment_start = pc;
    while (eat_byte() != ')') ;
    jump_targets[comment_start] = pc;
}

vv push_dataspace_label(uint32_t n) {
    return [=]() { stack.push(n); };
}

void define(word name, vv action) {
    assert(!intable(name, run_time_dispatch));
    run_time_dispatch[name] = action;
}

void dataspace_label() {
    word name = get_token();
    define(name, push_dataspace_label(memory.size()));
}

vv call_function(uint32_t n) {
    return [=]() {
        rstack.push(pc);
        pc = n;
    };
}

void define_function() {
    word name = get_token();
    define(name, call_function(pc));
}

uint32_t read_number() {
    std::string intbuf;
    char c;
    while (isdigit(c = eat_byte())) {
        intbuf += c;
    }
    return std::stoi(intbuf,nullptr,10);
}

void literal_byte_compile() {
    advance_past_whitespace();
    memory.push_back(read_number());
}

std::vector<word> as_bytes(uint32_t n) {
    std::vector<word> v {(word)n, (word)(n >> 8), (word)(n >> 16), (word)(n >> 24)};
    return v;
}

void literal_word() {
    advance_past_whitespace();
    std::vector<word> bytes = as_bytes(read_number());
    assert(bytes.size() == 4);
    memory.push_back(bytes[0]);
    memory.push_back(bytes[1]);
    memory.push_back(bytes[2]);
    memory.push_back(bytes[3]);
}

void allocate_space() {
    advance_past_whitespace();
    uint32_t n = read_number();
    memory.resize(memory.size() + n, 0);
}

void set_start_address() {
    start_address = pc;
}

void nop() {}

void skip_literal_byte() {
    eat_byte(); // '
    eat_byte(); // char
}

/* */

void start_conditional() {
    stack.push(pc);
}

void end_conditional() {
    jump_targets[pop(stack)] = pc;
}

void end_loop() {
    jump_targets[pc] = pop(stack);
}

void tbfcompile() {
    while (pc < program.size()) {
        word token = get_token();
        if (intable(token, compile_time_dispatch)) {
            compile_time_dispatch[token]();
        }
        else if (intable(token, run_time_dispatch)) {
            ; // ignore things from run-time for now
        }
        else {
            std::string tok = "` '";
            tok[1] = token;
            std::string errmsg = "Illegal instruction encountered at compile time: " + tok;
            debug(errmsg.c_str());
            abort();
        }

        advance_past_whitespace();
    }
}

/* */

void write_out() {
    uint32_t count = pop(stack);
    uint32_t address = pop(stack);
    std::cerr << "writing address " << address << ", count " << count << std::endl;
    std::string buf;
    for (uint32_t i = address; i < address+count; i++) {
        buf += memory[i];
    }
    std::cout << buf;
}

void quit() {
    exit(EXIT_SUCCESS);
}

void subtract() {
    int32_t x = pop(stack);
    stack.push(pop(stack) - x);
}

void push_literal() {
    stack.push(read_number());
}

int32_t decode(std::vector<word> bytes) {
    assert(bytes.size() == 4);
    uint32_t rv = bytes[0] | bytes[1] << 8 | bytes[2] << 16 | bytes[3] << 24;
    return rv;
}

void fetch() {
    uint32_t addr = pop(stack);
    std::vector<word> bytes;
    bytes.push_back(memory[addr+0]);
    bytes.push_back(memory[addr+1]);
    bytes.push_back(memory[addr+2]);
    bytes.push_back(memory[addr+3]);
    stack.push(decode(bytes));
}

void extend_memory(uint32_t addr) {
    /* Address >100k are probably a bug */
    if (memory.size() < addr + 1 && addr < 100000) {
        memory.resize(addr+1, 0);
    }
}

void store() {
    uint32_t addr = pop(stack);
    extend_memory(addr+3); // extend already +1
    std::vector<word> bytes = as_bytes(pop(stack));
    assert(bytes.size() == 4);
    memory[addr+0] = bytes[0];
    memory[addr+1] = bytes[1];
    memory[addr+2] = bytes[2];
    memory[addr+3] = bytes[3];
}

void store_byte() {
    uint32_t addr = pop(stack);
    extend_memory(addr);
    memory[addr] = (word)pop(stack);
}

void less_than() {
    int32_t b = pop(stack);
    int32_t a = pop(stack);
    stack.push((uint32_t) (a<b));
}

void return_from_function() {
    pc = pop(rstack);
}

void read_byte() {
    int byte = getchar();
    if (byte == EOF) {
        debug("EOF");
        stack.push(-1);
    }
    else {
        stack.push(byte);
    }
}

void jump() {
    pc = jump_targets[pc];
}

void conditional() {
    if (pop(stack)) return;
    jump();
}

void loop() {
    if (!pop(stack)) return;
    jump();
}

void literal_byte_run() {
    eat_byte();
    stack.push(eat_byte());
}

void tbfrun() {
    assert(start_address != -1);
    pc = start_address;
    while (true) {
        word token = get_token();
        // std::cout << token << std::endl;
        if (intable(token, run_time_dispatch)) {
            run_time_dispatch[token]();
        }
        else {
            std::string tok = "` '";
            tok[1] = token;
            std::string errmsg = "Illegal instruction encountered at run time: " + tok;
            debug(errmsg.c_str());
            abort();
        }
    }
}

int main(int argc, char **argv) {
    for (char c = '0'; c <= '9'; c++) {
        compile_time_dispatch[c] = read_number;
        run_time_dispatch[c] = push_literal;
    }

    if (argc != 2) {
        debug("Wrong number of arguments");
        return EXIT_FAILURE;
    }

    FILE *fp = fopen(argv[1], "r");
    if (fp == NULL) {
        debug("Could not open file for reading");
        return EXIT_FAILURE;
    }

    int c;
    while ((c = fgetc(fp)) != EOF) {
        program.push_back(c);
    }

    fclose(fp);

    tbfcompile();
    tbfrun();
    abort(); // tbfrun returned -- should exit
}
