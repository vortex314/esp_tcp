#include "Sys.h"
#include <Logger.h>

void * operator new(size_t size) {
 return malloc(size);
}

void operator delete(void * ptr) {
	free(ptr);
}

void * operator new[](size_t size) {
	return malloc(size);
}

void operator delete[](void * ptr) {
	free(ptr);
}

#include "errno.h"
#include "stdint.h"
#include "stddef.h"
#include "stdlib.h"
#include "fcntl.h"
extern int errno;

#ifdef __cplusplus
extern "C" {
#include "Erc.h"
#endif

IROM void _exit(int code) {
	ASSERT(false);
}

IROM void __aeabi_atexit() {
	return;
}

IROM void __exidx_end() {
	ASSERT(false);
}

IROM void __exidx_start() {
	ASSERT(false);
}
/*
 kill
 Send a signal. Minimal implementation:
 */
IROM int _kill(int pid, int sig) {
	ASSERT(false);
	errno = EINVAL;
	return (-1);
}

//void abort(){};

/*
 link
 Establish a new name for an existing file. Minimal implementation:
 */

/*
 getpid
 Process-ID; this is sometimes used to generate strings unlikely to conflict with other processes. Minimal implementation, for a system without processes:
 */

IROM int _getpid() {
	return 1;
}

IROM int _ctype(char c) {
	return 0;
}
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#ifndef STDOUT_USART
#define STDOUT_USART 2
#endif

#ifndef STDERR_USART
#define STDERR_USART 2
#endif

#ifndef STDIN_USART
#define STDIN_USART 2
#endif
/*
 isatty
 Query whether output stream is a terminal. For consistency with the other minimal implementations,
 */
IROM int _isatty(int file) {
	ERROR("SHouldn't arrive here !");
	switch (file) {
		case STDOUT_FILENO:
		case STDERR_FILENO:
		case STDIN_FILENO:
		return 1;
		default:
		//errno = ENOTTY;
		errno = EBADF;
		return 0;
	}
}
/*
 write
 Write a character to a file. `libc' subroutines will use this system routine for output to all files, including stdout
 Returns -1 on error or number of bytes sent
 */
IROM int _write(int file, char *ptr, int len) {
	ASSERT(false);
	return 0;
}
/*
 read
 Read a character to a file. `libc' subroutines will use this system routine for input from all files, including stdin
 Returns -1 on error or blocks until the number of characters have been read.
 */

IROM int _read(int file, void *ptr, size_t len) {
	ASSERT(false);

	return 0;
}
/*
 int stat(const char *filepath, struct stat *st) {
 st->st_mode = S_IFCHR;
 return 0;
 }
 */
/*
 lseek
 Set position in a file. Minimal implementation:
 */
IROM off_t lseek(int file, off_t ptr, int dir) {
	return 0;
}

IROM int _close(int file) {
	return -1;
}
/*
 fstat
 Status of an open file. For consistency with other minimal implementations in these examples,
 all files are regarded as character special devices.
 The `sys/stat.h' header file required is distributed in the `include' subdirectory for this C library.
 */

IROM int _fstat(int file, struct stat *st) {
	st->st_mode = S_IFCHR;
	return 0;
}

/*
 link
 Establish a new name for an existing file. Minimal implementation:
 */

IROM int _link(char *old, char *nw) {
	errno = EMLINK;
	return -1;
}

/*
 lseek
 Set position in a file. Minimal implementation:
 */
IROM int _lseek(int file, int ptr, int dir) {
	return 0;
}

__extension__ typedef int __guard __attribute__((mode (__DI__)));


IROM int __cxa_guard_acquire(__guard *g) {
	return !*(char *) (g);
}

IROM void __cxa_guard_release(__guard *g) {
	*(char *) g = 1;
}

IROM void __cxa_guard_abort(__guard *) {
}

IROM void __cxa_pure_virtual(void) {
}

IROM int _getpid_r(){
	return 1;
};
IROM void _kill_r(int id){

}

// register char *stack_ptr =0;
#include "osapi.h"
#include "util.h"
caddr_t
IROM _sbrk_r (int incr)
{
	INFO(" _sbrk_r CALLED !!!!! ");
	while(1);
	return 0;
	/*
  extern char end;		// Defined by the linker
  static char *heap_end;
  char *prev_heap_end;

  if (heap_end == 0)
    {
      heap_end = &end;
    }
  prev_heap_end = heap_end;
  if (heap_end + incr > stack_ptr)
    {
      _write (1, "Heap and stack collision\n", 25);
      abort ();
    }
  heap_end += incr;
  return (caddr_t) prev_heap_end;
  */
}
#ifdef MEMCPY
void *memcpy(void *dest, const void *src, size_t n) {
	uint8_t* dp = (uint8_t*) dest;
	const uint8_t* sp = (uint8_t*) src;
	while (n--)
		*dp++ = *sp++;
	return dest;
}
#endif

#ifdef __cplusplus
};
#endif
