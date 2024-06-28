#pragma once
#include "editor.h"


int main(int argc, char** argv)
{
	//while (true) printf("%d ", _getch());
	EditorState state = EditorState::create(256);


	while (editor_tick(state));


	return 0;
}

