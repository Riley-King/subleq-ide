#pragma once
#include <memory.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>

template <typename T>
struct subleq
{
	T _ip;
	size_t memsize;
	bool running;
	uint8_t memory[0];
};

template <typename T>
subleq<T>* create_subleq(size_t memory_size=255)
{
	subleq<T>* x = (subleq<T>*)malloc(sizeof(subleq<T>) + memory_size);
	if (x == nullptr)
		return nullptr;
	x->_ip = 0;
	x->memsize = memory_size;
	x->running = false;
	memset(x->memory, 0, memory_size);
	return x;
}

template <typename T>
void destroy_subleq(subleq<T>* state)
{ free(state); }


template <typename T>
bool subleq_step(subleq<T>* state, void (*FN_OnOutput)(subleq<T>* state, T& value, const T& current_ip, void* userarg)=nullptr, void* userarg=nullptr)
{
	state->running = state->_ip < state->memsize;
	if (!state->running)
		return false;
	const T& a = *(T*)(state->memory+state->_ip);
	T& b = *(T*)(state->memory+state->_ip + sizeof(T));
	const T& c = *(T*)(state->memory + state->_ip + sizeof(T) * 2);

	if (b == ((T)(-1)))
	{
		if (FN_OnOutput != nullptr)
			FN_OnOutput(state, *(T*)(state->memory+a), state->_ip, userarg);
		printf("%c", (char)state->memory[a]);
	}
	else b = b - a;

	if (b <= 0) state->_ip = c;
	else state->_ip += sizeof(T) * 3;

	state->running = state->_ip < state->memsize;
	return state->_ip < state->memsize;
}

template <typename T>
void subleq_printmem(subleq<T>* state, size_t mem_addr, size_t length=1, FILE* f=stdout)
{
	if ((mem_addr+length) >= state->memsize)
	{
		printf(stderr, "Error: mem_addr(%llu) + length(%llu) was >= memsize(%llu)\n", mem_addr, length, state->memsize);
		return;
	}
	printf("%.*s", length, state->memory+mem_addr);
}