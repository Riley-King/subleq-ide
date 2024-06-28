#include "subleq.h"
#include <conio.h>
#include <map>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

const int8_t program[] = { 9, -1, 3, 10, -1, 6, 0, 0, -1, 72, 105, 0 };
const size_t program_length = sizeof(program) / sizeof(program[0]);

const uint8_t rows = 30, cols = 120;
const uint8_t element_width = 5; // (-127 )

enum BREAKPT_TYPE
{
	BREAK,
	COND_GT,
	COND_GEQ,
	COND_LT,
	COND_LEQ,
	COND_EQ,
	COND_NEQ,
};
struct breakpt
{
	BREAKPT_TYPE type;
	uint8_t cond_offset = 0;
	int8_t meta;
};

void getch_test()
{
	while (true)
	{
		printf("%d ", _getch());
	}
}

int old_main()
{
	getch_test();

	std::map<size_t, breakpt> breakpoints;
	subleq<uint8_t>* sim = create_subleq<uint8_t>(255);
	if (sim == nullptr)
		return -1;
	memcpy(sim->memory, program, program_length);

	bool adding_breakpts = false;
	bool is_stepping = false;
	bool started = false;
	do {
		if (started)
			started = false;
		if (!sim->running && !adding_breakpts && sim->_ip != 0)
			sim->_ip = 0;
		// If it is not the first iteration and there is no breakpoint, quit
		if (sim->running && !is_stepping && !adding_breakpts)
		{
			// Check breakpoints
			if (breakpoints.contains(sim->_ip))
			{
				breakpt b = breakpoints.at(sim->_ip);
				is_stepping |= (b.type == BREAKPT_TYPE::BREAK ||
					b.type == BREAKPT_TYPE::COND_EQ && b.meta == sim->memory[sim->_ip + b.cond_offset] ||
					b.type == BREAKPT_TYPE::COND_GEQ && b.meta >= sim->memory[sim->_ip + b.cond_offset] ||
					b.type == BREAKPT_TYPE::COND_GT && b.meta > sim->memory[sim->_ip + b.cond_offset] ||
					b.type == BREAKPT_TYPE::COND_LEQ && b.meta <= sim->memory[sim->_ip + b.cond_offset] ||
					b.type == BREAKPT_TYPE::COND_LT && b.meta < sim->memory[sim->_ip + b.cond_offset] ||
					b.type == BREAKPT_TYPE::COND_NEQ && b.meta != sim->memory[sim->_ip + b.cond_offset]
					);
			}
			if (!is_stepping)
			{
				subleq_step(sim);
				continue;
			}
			else
				printf("\n========================================================\n");
		}

		printf("\033[m\033[2J\033[1;1H");
		printf("Instruction Pointer: %llu\n", (size_t)sim->_ip);
		for (size_t i = 0; i < sim->memsize; ++i)
		{
			if (i == sim->_ip) printf("\033[7m");											// Set the currently executing instruction to be black/white
			else if (breakpoints.contains(i)) printf("\033[48;5;9m\033[38;5;15m");	// Set breakpoints to white/red
			printf("% *i", element_width, (int8_t)sim->memory[i]);							// Print the value at the address
			if (i == sim->_ip || breakpoints.contains(i)) printf("\033[m");			// Reset colour to prevent state leak
			if ((i + 1) % (cols / element_width) == 0) printf("\n");
		}
		printf("\n\n");

		if (sim->running && is_stepping && !adding_breakpts)
		{
			is_stepping = false;
			printf("==================== Program Output ====================\n");
			subleq_step(sim);
			printf("\n========================================================\n");
		}

		if (!adding_breakpts)
			printf("[Q]uit  [S]tep  [C]ontinue  [B]reakpt\n");
		else if (adding_breakpts)
			printf("[c]ancel  Navigate Up/Down/Left/Right  Return to select\n");
		int keypress = -1;
		while (keypress == -1)
		{
			keypress = _getch();
			if ((keypress == 'q' || keypress == 'Q') && !adding_breakpts)
			{
				sim->running = false;
				sim->_ip = sim->memsize + 1;
				printf("Quitting\n");
			}
			else if (keypress == 'c' || keypress == 'C')
			{
				if (!adding_breakpts)
				{
					if (sim->running == false)
						started = true;
					sim->running = true;
					adding_breakpts = false;
					printf("==================== Program Output ====================\n");
				}
				else
					adding_breakpts = false;
			}
			else if ((keypress == 's' || keypress == 'S') && !adding_breakpts)
			{
				if (sim->running == false)
					started = true;
				sim->running = true;
				is_stepping = true;
				adding_breakpts = false;
			}
			else if (keypress == 'b' || keypress == 'B' || adding_breakpts)
			{
				if (keypress == 224)
				{
					int keypress2 = _getch();
					if (keypress2 == 75 && sim->_ip >= 1) sim->_ip--;
					else if (keypress2 == 72 && sim->_ip >= (cols / element_width)) sim->_ip -= (cols / element_width);
					else if (keypress2 == 77 && (sim->_ip + 1) < sim->memsize) sim->_ip++;
					else if (keypress2 == 80 && (sim->_ip + cols / element_width) < sim->memsize) sim->_ip += (cols / element_width);
				}
				else if (keypress == 13)
				{
					if (breakpoints.contains(sim->_ip))
						breakpoints.erase(sim->_ip);
					else
					{
						BREAKPT_TYPE type = BREAKPT_TYPE::BREAK;
						int64_t meta = 1;

						// Read breakpoint type
						if (adding_breakpts)
							printf("[c]ancel  [b]reak  [g]t  [G]eq  [l]t  [L]eq  [e]q  [n]eq\n");
						int code = -1;
						while (code == -1)
						{
							code = _getch();
							if (code == 'c' || code == 'C')
								meta = 0;
							else if (code == 'B' || code == 'b') type = BREAKPT_TYPE::BREAK;
							else if (code == 'g') type = BREAKPT_TYPE::COND_GT;
							else if (code == 'G') type = BREAKPT_TYPE::COND_GEQ;
							else if (code == 'l') type = BREAKPT_TYPE::COND_LT;
							else if (code == 'L') type = BREAKPT_TYPE::COND_LEQ;
							else if (code == 'e' || code == 'E') type = BREAKPT_TYPE::COND_EQ;
							else if (code == 'n' || code == 'N') type = BREAKPT_TYPE::COND_NEQ;
							else code = -1;
						}

						if (meta == 0)
						{
							adding_breakpts = false;
							keypress = -1;
							break;
						}

						// Read breakpoint meta (if needed)
						if (type != BREAKPT_TYPE::BREAK && meta != 0)
						{
							printf("Cond. Value: ");
							while (scanf_s("%lld", &meta));
							printf("\n");
						}

						breakpoints.emplace(sim->_ip, breakpt{
							.type = type,
							.meta = (int8_t)meta
							});

						adding_breakpts = false;
						keypress = -1;
						std::this_thread::sleep_for(100ms);
					}
				}
				if (!adding_breakpts)
					adding_breakpts = true;
			}
			else
				keypress = -1;
		}
	} while (sim->_ip < sim->memsize);
	if (!sim->running && !is_stepping && sim->_ip != (sim->memsize + 1))
		printf("\n========================================================\n");

	destroy_subleq(sim);
	return 0;
}