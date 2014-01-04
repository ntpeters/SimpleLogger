/*
	Header file for modified 'backtrace-symbols.c' by Jeff Muizelaar

	Nathan Peterson 2014
*/

char **backtrace_symbols(void *const *buffer, int size);
void backtrace_symbols_fd(void *const *buffer, int size, int fd);