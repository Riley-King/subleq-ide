#pragma once
#include <map>
#include <iostream>
#include <fstream>
#include <conio.h>
#include "subleq.h"


typedef int8_t cell_value_t;

enum BREAKPT_TYPE : uint8_t
{
	BREAK,
	COND_GT,
	COND_GEQ,
	COND_LT,
	COND_LEQ,
	COND_EQ,
	COND_NEQ,
};
struct BreakPoint
{
	BREAKPT_TYPE type = BREAK;
	cell_value_t meta = 0;
	int8_t addr_offset = 0;
	bool is_valid = true;
};

enum EditorMode : uint8_t
{
	RUNNING,			// The simulator is running until breakpoint or end
	STEP,				// The simulator runs for a single step
	MENU,				// Shows the menu that can allow running, stepping, adding breakpoints and editing
	ADD_BREAKPOINT,		// The simulator is paused (or potentially halted) and a breakpoint is being added
	EDIT_VALUES,		// The simulator is paused (or potentially halted) and being edited
	QUIT,				// Quit the editor and simulator
	END_OF_PROGRAM,		// The simulator has finished running
};
struct EditorState
{
	// Once the simulator has finished running, this will be set to false
	// The moment the simulator steps, this is set to true.
	bool sim_started = true;
	subleq<cell_value_t>* sim = nullptr;
	subleq<cell_value_t>* sim_initial = nullptr;
	size_t elements_per_row = -1;
	// Maps an address to a breakpoint
	std::map<size_t, BreakPoint> breakpoints;
	EditorMode mode = EditorMode::MENU;

	char* program_output = nullptr;
	size_t program_output_size = 0;
	size_t program_output_capacity = 0;

	uint16_t term_rows = -1;
	uint16_t term_cols = -1;
	size_t term_mem_cursor = 0;
	size_t element_width = 0;

	~EditorState()
	{
		destroy_subleq(this->sim);
		if (this->sim_initial != nullptr)
			destroy_subleq(this->sim_initial);
		free(program_output);
	}

	EditorState(){}

	EditorState(const EditorState&) = delete;
	EditorState& operator =(const EditorState&) = delete;

	EditorState& operator =(EditorState&& other) noexcept
	{
		this->elements_per_row = other.elements_per_row;
		this->mode = other.mode;
		this->program_output = other.program_output;
		this->program_output_capacity = other.program_output_capacity;
		this->program_output_size = other.program_output_size;
		this->sim = other.sim;
		this->sim_started = other.sim_started;
		this->term_cols = other.term_cols;
		this->term_mem_cursor = other.term_mem_cursor;
		this->term_rows = other.term_rows;
		this->breakpoints = std::move(other.breakpoints);

		other.program_output = nullptr;
		other.program_output_size = 0;
		other.program_output_capacity = 0;
		other.sim = nullptr;
		other.term_mem_cursor = -1;
		return *this;
	}
	EditorState(EditorState&& other) noexcept { *this = std::move(other); }

	static EditorState create(const size_t mem_size)
	{
		EditorState state;
		state.sim = create_subleq<cell_value_t>(mem_size);
		state.program_output = (char*)malloc(mem_size);
		state.program_output_capacity = mem_size;
		state.program_output_size = 0;
		memset(state.program_output, 0, state.program_output_capacity) ;
		
		state.mode = MENU;

		state.term_rows = 0;
		state.term_cols = 0;
		state.term_mem_cursor = 0;
		printf("\033[999;999H\033[6n");
		fprintf(stdout, "\033[6n");
		_getch_nolock(); _getch_nolock(); // Read "\033[" from response

		int c = 0;
		while (c != ';' && c != -1)
		{
			c = _getch_nolock();
			if (c >= 48 && c < 58)
				state.term_rows = state.term_rows * 10 + (c - 48);
		}
		c = 0;
		while (c != 'R' && c != -1)
		{
			c = _getch_nolock();
			if (c >= 48 && c < 58)
				state.term_cols = state.term_cols * 10 + (c - 48);
		}
		printf("\033[1;1H");

		state.element_width = 5;
		state.elements_per_row = state.term_cols / state.element_width;
		const size_t total_num_elements = mem_size;
		while (((total_num_elements / state.elements_per_row) * state.elements_per_row) != total_num_elements)
			state.elements_per_row--;
		if (state.elements_per_row == 1)
			throw std::exception("Failed to find elements per row");

		return state;
	}
};

inline void _editor_draw_sim_cell(EditorState& state, const size_t& i)
{
	if (state.term_mem_cursor == i && (state.mode == ADD_BREAKPOINT || state.mode == EDIT_VALUES)) printf("\033[7m");
	else if (state.sim->_ip == i) printf("\033[48;5;10m");
	else if (state.breakpoints.contains(i) &&
			 state.breakpoints[i].is_valid)
		printf("\033[48;5;9m");

	printf("% *d", state.element_width, (cell_value_t)state.sim->memory[i]);
	
	printf("\033[m");
}

inline void _editor_draw_sim(EditorState& state)
{
	// Clear and set cursor to 1,1
	printf("\033[2J\033[1;1H\033[m");
	for (uint16_t c = 0; c < state.term_cols; ++c)
		printf("%c", 220);
	for (size_t i = 0; i < state.sim->memsize; ++i)
	{
		if (i % state.elements_per_row == 0 && i != 0)
			printf("\n");
		_editor_draw_sim_cell(state, i);
	}
	printf("\n");
	for (uint16_t c = 0; c < state.term_cols; ++c)
		printf("%c", 223);
	printf("\n");
}

inline void _editor_reset(EditorState& state)
{
	if (state.sim_initial == nullptr)
	{
		memset(state.sim->memory, 0, state.sim->memsize * sizeof(cell_value_t));
		state.sim->_ip = 0;
		state.sim->running = false;
	}
	else
		memcpy(state.sim, state.sim_initial, sizeof(subleq<cell_value_t>)+sizeof(cell_value_t)*state.sim->memsize);
}

inline void _editor_load_asm(EditorState& state)
{
	// prompt filename

	// assemble assembley

}

inline void _editor_swap_byteorder(void* dst, void* src, const size_t element_size, const size_t num_elements)
{
	if (element_size == 1)
		return;
	char* d = (char*)dst;
	char* s = (char*)src;
	for (size_t i = 0; i < num_elements; ++i)
	{
		const size_t idx = i * element_size;
		if (d == s)
		{
			for (size_t j = 0; j < element_size / 2; ++j)
			{
				char tmp = d[idx + j];
				d[idx + j] = s[idx + element_size - j - 1];
				s[idx + element_size - j - 1] = tmp;
			}
		}
		else
			for (size_t j = 0; j < element_size; ++j)
				d[idx + j] = s[idx + element_size - j - 1];
	}
}

inline void _editor_load_bin(EditorState& state)
{
	// prompt filename
	std::ifstream f;
	const size_t fname_buf_size = 261;
	char fname[fname_buf_size]{ '\0' };
	do {
		printf("File Name: ");
		fgets(fname, fname_buf_size, stdin);
		fname[strlen(fname) - 1] = '\0';
		uint16_t x = 0;
		f.open(fname);
		if (!f.is_open())
			printf("\033[38;5;9mFile could not be created\033[m\n");
	} while (!f.is_open());


	std::streampos f_size = f.tellg();
	f.seekg(0, std::ios::end);
	f_size = f.tellg() - f_size;
	f.seekg(0, std::ios::beg);
	if (f_size < (2 + 1 + sizeof(size_t) + sizeof(cell_value_t)))
	{
		printf("\033[38;5;9mFile was too small to be a valid save!\033[m\nPress any key to continue...\n");
		_getch();
		return;
	}
	// read magic value, version, memsize and initial IP
	bool match_endian = true;
	uint16_t magic;
	f.read((char*)(&magic), 2);
	if (magic != 0x1337 && magic != 0x3713)
	{
		printf("\033[38;5;9mFile header was not valid!\033[m\nPress any key to continue...\n");
		_getch();
		return;
	}
	match_endian = memcmp(&magic, "\x13\x37", 2) != 0;

	uint8_t version;
	f.read((char*)(&version), 1);
	if (version != 1)
	{
		printf("\033[38;5;9mFile verrsion does not match version %d!\033[m\nPress any key to continue...\n", 1);
		_getch();
		return;
	}
	size_t memsize;
	f.read((char*)(&memsize), sizeof(size_t));
	if (!match_endian)
		_editor_swap_byteorder(&memsize, &memsize, sizeof(size_t), 1);
	destroy_subleq(state.sim);
	state.sim = create_subleq<cell_value_t>(memsize);
	f.read((char*)(&state.sim->_ip), sizeof(cell_value_t));
	if (!match_endian && sizeof(cell_value_t) != 1)
		_editor_swap_byteorder(&state.sim->_ip, &state.sim->_ip, sizeof(cell_value_t), 1);

	// copy into memory
	f.read((char*)&state.sim->memory, memsize*sizeof(cell_value_t));
	if (!match_endian && sizeof(cell_value_t) != 1)
		_editor_swap_byteorder(&state.sim->memory, &state.sim->memory, sizeof(cell_value_t), memsize);
	f.close();

	if (state.sim_initial == nullptr)
		state.sim_initial = create_subleq<cell_value_t>(state.sim->memsize);
	memcpy(state.sim_initial, state.sim, sizeof(subleq<cell_value_t>) + sizeof(cell_value_t) * state.sim->memsize);
}

inline void _editor_save_bin(EditorState& state)
{
	// prompt filename
	std::ofstream f;
	const size_t fname_buf_size = 261;
	char fname[fname_buf_size]{'\0'};
	do {
		printf("File Name: ");
		fgets(fname, fname_buf_size, stdin);
		fname[strlen(fname) - 1] = '\0';
		uint16_t x = 0;
		f.open(fname);
		if (!f.is_open())
			printf("\033[38;5;9mFile could not be created\033[m\n");
	} while (!f.is_open());
	
	// write magic value, version, memsize and initial IP
	const uint16_t magic = 0x1337;
	const uint8_t version = 1;
	f.write((const char*)(&magic), 2);
	f.write((const char*)(&version), 1);
	f.write((const char*)(&state.sim->memsize), sizeof(size_t));
	f.write((const char*)(&state.sim->_ip), sizeof(cell_value_t));

	// write the memory values
	f.write((const char*)(&state.sim->memory), sizeof(cell_value_t) * state.sim->memsize);
	f.close();
	printf("Saved binary \"%s\"\nPress any key to continue...\n", fname);
	_getch();
}

inline EditorMode _editor_menu(EditorState& state)
{
	_editor_draw_sim(state);
	printf("%.*s\n", state.program_output_size, state.program_output);
	for (uint16_t c = 0; c < state.term_cols; ++c)
		printf("%c", 223);
	printf("\n");
	
	printf("[q]uit    [c]ontinue    [s]tep    [e]dit    [b]reakpoint\n");
	printf("[r]eset   [l]oad asm    [L]oad bin          [S]ave bin\n");
	while (true)
	{
		int keycode = _getch();
		if (keycode == 'q' || keycode == 'Q') return QUIT;
		else if (keycode == 'e' || keycode == 'E') return EDIT_VALUES;
		else if (keycode == 'b' || keycode == 'B') return ADD_BREAKPOINT;
		else if (keycode == 'c' || keycode == 'C') return RUNNING;
		else if (keycode == 's') return STEP;
		else if (keycode == 'r' || keycode == 'R') { _editor_reset(state); return MENU; }
		else if (keycode == 'l') { _editor_load_asm(state); return MENU; }
		else if (keycode == 'L') { _editor_load_bin(state); return MENU; }
		else if (keycode == 'S') { _editor_save_bin(state); return MENU; }
	}
	return EditorMode::QUIT;
}

inline void _editor_add_breakpoints(EditorState& state)
{
	while (true)
	{
		_editor_draw_sim(state);
		printf("[c]ancel    [return/space] toggle breakpt    [e] Edit breakpt\n");
		if (state.breakpoints.contains(state.term_mem_cursor) &&
			state.breakpoints[state.term_mem_cursor].is_valid)
		{
			const BreakPoint& pt = state.breakpoints[state.term_mem_cursor];
			printf("Breakpoint    ");
			if (pt.type == BREAKPT_TYPE::COND_EQ) printf("x == %d", pt.meta);
			else if (pt.type == BREAKPT_TYPE::COND_NEQ) printf("x != %d", pt.meta);
			else if (pt.type == BREAKPT_TYPE::COND_GT) printf("x > %d", pt.meta);
			else if (pt.type == BREAKPT_TYPE::COND_GEQ) printf("x >= %d", pt.meta);
			else if (pt.type == BREAKPT_TYPE::COND_LT) printf("x < %d", pt.meta);
			else if (pt.type == BREAKPT_TYPE::COND_LEQ) printf("x <= %d", pt.meta);

			if (pt.addr_offset != 0) printf(", x = [ip%+d]", pt.addr_offset);
		}
		else printf("Memory Cell    [%llu] = %d\n", state.term_mem_cursor, (cell_value_t)state.sim->memory[state.term_mem_cursor]);

		int keycode = _getch();
		if (keycode == 'c')
			break;
		else if (keycode == 'e')
		{
			printf("[c]ancel    edit [m]ode    edit [v]alue    edit [o]ffset\n");
		}
		else if (keycode == 224)
		{
			const size_t& max_per_row = state.elements_per_row;
			const size_t& max = state.sim->memsize;
			size_t& new_cur = state.term_mem_cursor;
			int dir = _getch();
			if		(dir == 72 && (new_cur- max_per_row) <= new_cur) new_cur -= max_per_row;
			else if (dir == 80 && (new_cur+ max_per_row) < max) new_cur += max_per_row;
			else if (dir == 75 && (new_cur-1) <= new_cur) new_cur--;
			else if (dir == 77 && (new_cur+1) < max) new_cur++;
		}
		else if (keycode == '\r' || keycode == ' ')
		{
			if (state.breakpoints.contains(state.term_mem_cursor) && state.breakpoints[state.term_mem_cursor].is_valid)
				state.breakpoints.erase(state.term_mem_cursor);
			else
			{
				BreakPoint bk;
				bk.is_valid = true;
				bk.addr_offset = 0;
				bk.type = BREAKPT_TYPE::BREAK;
				bk.meta = 0;
				
				if (keycode == ' ')
				{
					printf("Address Offset ] ");
					int r = -1;
					while (!scanf_s("%d", &r));
					bk.addr_offset = r;

					printf("Cmp Op ] ");
					char buf[9]{'\1'};
					buf[8] = '\0';
					do {
						fgets(buf, 8, stdin);
					} while(buf[0] != '\0' && 
							buf[0] != '>' && 
							buf[0] != '<' && 
							buf[0] != '!' && 
							buf[0] != '=');

					if		(buf[0] == '\0')					bk.type = BREAKPT_TYPE::BREAK;
					else if (buf[0] == '=' && buf[1] == '=')	bk.type = BREAKPT_TYPE::COND_EQ;
					else if (buf[0] == '!' && buf[1] == '=')	bk.type = BREAKPT_TYPE::COND_NEQ;
					else if (buf[0] == '>' && buf[1] == '\n')	bk.type = BREAKPT_TYPE::COND_GT;
					else if (buf[0] == '>' && buf[1] == '=')	bk.type = BREAKPT_TYPE::COND_GEQ;
					else if (buf[0] == '<' && buf[1] == '\n')	bk.type = BREAKPT_TYPE::COND_LT;
					else if (buf[0] == '<' && buf[1] == '=')	bk.type = BREAKPT_TYPE::COND_LEQ;

					if (bk.type != BREAKPT_TYPE::BREAK)
					{
						printf("Cmp RHS ] ");
						int r = -1;
						while (!scanf_s("%d", &r));
						bk.meta = r;
					}
				}

				bk.is_valid = true;
				state.breakpoints.emplace(state.term_mem_cursor, bk);
			}
		}
	}
	state.term_mem_cursor = 0;
	if (state.mode == ADD_BREAKPOINT)
		state.mode = MENU;
}

inline void _editor_edit(EditorState& state)
{
	bool sign = false;
	cell_value_t new_val = 0;
	cell_value_t* prev_vals = (cell_value_t*)malloc(state.sim->memsize*sizeof(cell_value_t));
	memcpy(prev_vals, state.sim->memory, state.sim->memsize * sizeof(cell_value_t));

	while (true)
	{
		_editor_draw_sim(state);
		printf("Memory Cell    [%llu] = %d    New Value: %d\n", 
			state.term_mem_cursor, (cell_value_t)state.sim->memory[state.term_mem_cursor], 
			new_val*!sign - new_val*sign
		);
		printf("[c]ancel    [s]ave    [del]ete new value    [0-9\\-\\+] Type number    [return] Set value    [j]ump\n");
		int keycode = _getch();

		if (keycode == 224)
		{
			const size_t& max_per_row = state.elements_per_row;
			const size_t& max = state.sim->memsize;
			size_t& new_cur = state.term_mem_cursor;
			int dir = _getch();
			if (dir == 72 && (new_cur - max_per_row) <= new_cur) new_cur -= max_per_row;
			else if (dir == 80 && (new_cur + max_per_row) < max) new_cur += max_per_row;
			else if (dir == 75 && (new_cur - 1) <= new_cur) new_cur--;
			else if (dir == 77 && (new_cur + 1) < max) new_cur++;
			else if (dir == 83)
			{
				new_val = 0;
				sign = false;
			}
		}
		else if (keycode == 8)
			new_val /= 10;
		else if (keycode == '\r')
			state.sim->memory[state.term_mem_cursor] = new_val * !sign - new_val * sign;
		else if (keycode == '+')
			sign = false;
		else if (keycode == '-')
			sign = true;
		else if (keycode >= 48 && keycode < 58)
			new_val = new_val * 10 + keycode - 48;
		else if (keycode == 'j')
			state.sim->_ip = state.term_mem_cursor;
		else if (keycode == 's')
			break;
		else if (keycode == 'c')
		{
			memcpy(state.sim->memory, prev_vals, state.sim->memsize);
			break;
		}
	}
	free(prev_vals);
	state.term_mem_cursor = 0;
	if (state.mode == EDIT_VALUES)
		state.mode = MENU;
}

inline bool _breakpoint_breaks(EditorState& state, const BreakPoint& bk)
{
	const cell_value_t& x = state.sim->memory[state.sim->_ip + bk.addr_offset];
	switch (bk.type)
	{
	case BREAKPT_TYPE::BREAK:		return true;
	case BREAKPT_TYPE::COND_EQ:		return x == bk.meta;
	case BREAKPT_TYPE::COND_NEQ:	return x != bk.meta;
	case BREAKPT_TYPE::COND_GT:		return x > bk.meta;
	case BREAKPT_TYPE::COND_GEQ:	return x >= bk.meta;
	case BREAKPT_TYPE::COND_LT:		return x < bk.meta;
	case BREAKPT_TYPE::COND_LEQ:	return x <= bk.meta;
	default:						return false;
	}
}

template <typename T>
void _editor_on_sim_out(subleq<T>* sim, T& outval, const T& ip, void* userarg)
{
	EditorState& state = *(EditorState*)userarg;
	if ((state.program_output_size + 1) >= state.program_output_capacity)
	{
		char* new_output = (char*)malloc(state.program_output_capacity*2);
		if (new_output == nullptr || new_output == 0)
			throw std::exception("Failed to reallocate program output buffer");
		memcpy(new_output, state.program_output, state.program_output_size);
		free(state.program_output);
		state.program_output = new_output;
		state.program_output_capacity *= 2;
	}
	state.program_output[state.program_output_size] = (char)outval;
	state.program_output_size++;
}

inline bool editor_tick(EditorState& state)
{
	if (!state.sim_started)
		state.mode = END_OF_PROGRAM;

	if ( (state.mode == RUNNING || state.mode == STEP) && state.sim_started)
	{
		if (state.breakpoints.contains(state.sim->_ip) &&
			state.breakpoints.at(state.sim->_ip).is_valid &&
			_breakpoint_breaks(state, state.breakpoints.at(state.sim->_ip)))
		{
			state.mode = MENU;
		}
		else
		{
			state.sim_started = subleq_step<cell_value_t>(state.sim, _editor_on_sim_out<cell_value_t>, &state);
			if (state.mode == STEP)
				state.mode = MENU;
			if (state.mode != MENU && _getch_nolock() == 'p')
				state.mode = MENU;
		}
	}

	if (state.mode == MENU || state.mode == END_OF_PROGRAM)
	{
		state.mode = _editor_menu(state);
		if (state.mode == RUNNING)
			printf("[p]ause\n");
	}

	if (state.mode == ADD_BREAKPOINT)
		_editor_add_breakpoints(state);
	else if (state.mode == EDIT_VALUES)
		_editor_edit(state);

	return state.mode != QUIT;
}

