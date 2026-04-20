#include "utils.h"

#include "types.h"

#ifdef IS_UNIX
#include <unistd.h>

#include <sys/ioctl.h>

#include <signal.h>
#elif IS_WIN
#include <windows.h>
#endif

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <time.h>

void set_cursor_pos(_ssize_t col, _ssize_t row) {
	printf("\x1B[%ld;%ldH", col, row);
}

void get_terminal_size(_ssize_t* columns, _ssize_t* rows) {
	#ifdef IS_WIN
		CONSOLE_SCREEN_BUFFER_INFO csbi;
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
		if (columns) *columns = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		if (rows) *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	#elifdef IS_UNIX
		struct winsize size;

		ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);

		if (columns) *columns = size.ws_col;

		if (rows) *rows = size.ws_row;
	#endif
}

void change_title(const char* title) {
	printf("\x1B]0;%s\007", title);
}

uint64 emulator_read_tsc() {
	uint32 lo = 0, hi = 0;
	
	asm volatile("rdtsc" : "=a"(lo), "=d"(hi));

	uint64 result = ((uint64)hi << 32) | ((uint64)lo);

	return result;
}

#ifdef IS_UNIX
#include <fcntl.h>

#include <termios.h>

void switch_to_default(struct termios* orig_termios) {
	if (!orig_termios) return;
	
	tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_termios);
}

struct termios* switch_to_raw() {
	struct termios* orig_termios = malloc(sizeof(struct termios));

	memset(orig_termios, 0, sizeof(struct termios));
	
	tcgetattr(STDIN_FILENO, orig_termios);

	struct termios raw = *orig_termios;
	
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	
	raw.c_iflag &= ~(ICRNL | IXON);
	
	raw.c_oflag &= ~(OPOST);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

	return orig_termios;
}

void init_timer(timer_t* timer, time_t ns, void (*handler)(int)) {
	struct sigevent sev;

	struct sigaction sa;

	sa.sa_handler = handler;

	sa.sa_flags = SA_RESTART;

	sigemptyset(&sa.sa_mask);

	sigaction(SIGRTMIN, &sa, NULL);

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGRTMIN;
	sev.sigev_value.sival_ptr = timer;

	timer_create(CLOCK_REALTIME, &sev, timer);

	setup_timer(*timer, ns);
}

void setup_timer(timer_t timerid, time_t ns) {
	struct itimerspec timer_cfg;
	
	timer_cfg.it_value.tv_sec = 0;
	timer_cfg.it_value.tv_nsec = ns;

	timer_cfg.it_interval.tv_sec = 0;
	timer_cfg.it_interval.tv_nsec = ns;

	timer_settime(timerid, 0, &timer_cfg, NULL);
}
#endif