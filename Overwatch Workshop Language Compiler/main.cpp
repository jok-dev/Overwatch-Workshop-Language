#include <stdlib.h>
#include <stdio.h>

#include "lexer.h"
#include "parser.h"
#include "file.h"

#include <Windows.h>

#include <vector>

#include "DXGICaptureSample.h"

// @Todo: we could add a printing of the parser intention stack on error?

std::vector<WORD> key_buffer;

u32 text_sleep_time = 30;
u32 sleep_time = 150;
u32 sleep_time_flashing = sleep_time;

void flush_keys();

void queue_key(WORD key)
{
	key_buffer.push_back(key);

	if(key_buffer.size() >= 5)
	{
		flush_keys();
		Sleep(text_sleep_time);
	}
}

void queue_char(char ch)
{
	SHORT vk = VkKeyScanExA(ch, GetKeyboardLayout(0));

	// make sure they don't need shift or ctrl 
	assert(!(vk & 0x100));
	assert(!(vk & 0x200));

	queue_key(vk & 0xFF);
}

void queue_text(const char* text)
{
	size_t len = strlen(text);
	for(u32 i = 0; i < len; i++)
	{
		char ch = text[i];

		queue_char(ch);
	}
}

void flush_keys()
{
	u32 count = key_buffer.size();
	INPUT* ips = (INPUT*) calloc(sizeof(INPUT), key_buffer.size() * 2);

	for(u32 i = 0; i < count * 2; i += 2)
	{
		WORD key = key_buffer[i / 2];

		INPUT down;
		// Set up a generic keyboard event.
		down.type = INPUT_KEYBOARD;
		down.ki.wScan = 0; // hardware scan code for key
		down.ki.time = 0;
		down.ki.dwExtraInfo = GetMessageExtraInfo();
		down.ki.wVk = key;
		down.ki.dwFlags = KEYEVENTF_UNICODE;
		ips[i] = down;

		INPUT up;
		// Set up a generic keyboard event.
		up.type = INPUT_KEYBOARD;
		up.ki.wScan = 0; // hardware scan code for key
		up.ki.time = 0;
		up.ki.dwExtraInfo = GetMessageExtraInfo();
		up.ki.wVk = key;
		up.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

		ips[i + 1] = up;
	}

	SendInput(count * 2, ips, sizeof(INPUT));

	free(ips);

	key_buffer.clear();
}

BOOL CALLBACK EnumWindowsProc(HWND wnd, LPARAM param)
{
	TCHAR class_name[200] = { 0 };
	TCHAR module_file_name[MAX_PATH] = { 0 };

	GetClassNameA(wnd, class_name, 200);
	GetWindowModuleFileNameA(wnd, module_file_name, MAX_PATH);

	if(strcmp(class_name, "TankWindowClass") != 0) return true;

	printf("Hi! back from %s %s (%i)\n", class_name, module_file_name, wnd);

	if(IsIconic(wnd)) ShowWindow(wnd, SW_RESTORE);

	SetForegroundWindow(wnd);

	RECT rec;
	POINT top_left = {0};

	GetClientRect(wnd, &rec);
	assert(ClientToScreen(wnd, &top_left));
	rec.left = top_left.x;
	rec.top = top_left.y;
	rec.right += rec.left;
	rec.bottom += rec.top;


	run(rec);

	return false;

	// reset to top right button
	queue_char('p');
	flush_keys();
	Sleep(sleep_time);
	queue_char('p');
	flush_keys();
	Sleep(sleep_time);

	// click add rule
	queue_key(VK_TAB);
	queue_key(VK_SPACE);
	flush_keys();
	Sleep(sleep_time);

	return false;

	// change the event
	queue_key(VK_DOWN);
	queue_key(VK_DOWN);
	queue_key(VK_SPACE);
	flush_keys();
	Sleep(sleep_time);
	
	queue_text("ongoing - each player");
	flush_keys();

	Sleep(sleep_time);
	
	// close event dropdown
	queue_key(VK_ESCAPE);

	// click add condition
	queue_key(VK_DOWN);
	queue_key(VK_LEFT);
	queue_key(VK_SPACE);
	flush_keys();
	Sleep(sleep_time);

	// select the condition value dropdown
	queue_key(VK_TAB);
	queue_key(VK_UP);
	queue_key(VK_UP);
	queue_key(VK_UP);
	queue_key(VK_SPACE);
	flush_keys();
	Sleep(sleep_time);

	// type the name of the condition value and select it
	queue_text("add");
	flush_keys();
	Sleep(sleep_time);
	queue_key(VK_TAB);
	queue_key(VK_SPACE);
	flush_keys();
	Sleep(sleep_time);

	// add it to the condition list
	queue_key(VK_ESCAPE);
	flush_keys();
	Sleep(sleep_time);

	// select the add action button
	queue_key(VK_RIGHT);
	queue_key(VK_SPACE);
	flush_keys();
	Sleep(sleep_time);

	// select the action value dropdown
	queue_key(VK_TAB);
	queue_key(VK_UP);
	queue_key(VK_UP);
	queue_key(VK_UP);
	queue_key(VK_SPACE);
	flush_keys();
	Sleep(sleep_time);

	// type the name of the action and select it
	queue_text("big message");
	flush_keys();
	Sleep(sleep_time);
	queue_key(VK_TAB);
	queue_key(VK_SPACE);
	flush_keys();
	Sleep(sleep_time);

	// add it to the action list
	queue_key(VK_ESCAPE);
	flush_keys();

	Sleep(sleep_time);

	return false;
}

int main()
{
	/*file_data* file = file_load("../example scripts/lucio_chased.wss");

	lexer_init((char*) file->data);

	ast_node node;
	while((node = parse_statement()).type != AST_NODE_TYPE_END)
	{
		dynstr* str = ast_stringify_as_code(node);
		printf("%s", str->raw);
		dynstr_destroy(str);
	}

	file_destroy(file);*/


	EnumWindows(EnumWindowsProc, 0);

	printf("\n");

	system("PAUSE");
	exit(0);
}