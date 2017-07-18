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

void debug(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

typedef uint8_t word;

int start_address = -1;
std::vector<word> memory;
std::vector<word> stack;
std::vector<word> rstack;

std::vector<word> program;

size_t pc = 0;

std::unordered_map<uint32_t, uint32_t> jump_targets;

typedef std::function<void(void)> vv;

std::unordered_map<word, vv> run_time_dispatch;

word current_byte() {
    if (pc >= program.size()) {
        debug("Out of bounds in current_byte");
    }

    return program[pc];
}

word eat_byte() {
    return program[pc++];
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
    if (isdigit(rv) || rv == '\'') advance_to_whitespace();
    return rv;
}

void eat_comment() {
    size_t comment_start = pc;
    while (eat_byte() != ')') continue;
    jump_targets[comment_start] = pc;
}

vv push_dataspace_label(uint32_t n) {
    return [=]() { stack.push_back(n); };
}

void define(word name, vv action) {
    assert(run_time_dispatch.count(name) == 0);
    run_time_dispatch[name] = action;
}

void dataspace_label() {
    word name = get_token();
    define(name, push_dataspace_label(memory.size()));
}

vv call_function(size_t n) {
    return [=]() {
        rstack.push_back(pc);
        pc = n;
    };
}

void define_function() {
    word name = get_token();
    define(name, call_function(pc));
}

size_t read_number() {
    std::string intbuf;
    char c;
    while (isdigit(c = eat_byte())) {
        intbuf += c;
    }
    return std::stoi(intbuf,nullptr,10);
}

void literal_byte() {
    advance_past_whitespace();
    memory.push_back(read_number());
}

std::vector<word> as_bytes(uint32_t n) {
    std::vector<word> v {n & 255, (n >> 8) & 255, (n >> 16) & 255, (n >> 24) & 255};
    return v;
}

void literal_word() {
    advance_past_whitespace();
    std::vector<words> bytes = as_bytes(read_number());
    memory.push_back(bytes[0]);
    memory.push_back(bytes[1]);
    memory.push_back(bytes[2]);
    memory.push_back(bytes[3]);
}

void allocate_space() {
    advance_past_whitespace();
    size_t n = read_number();
    memory.reserve(memory.size() + n);
}

void set_start_address() {
    start_address = pc;
}

void nop() {}

void skip_literal_byte() {
    eat_byte(); // '
    eat_byte(); // char
}

int main() {
    std::cout << "stoneknifeforth, yo" << std::endl;
}
