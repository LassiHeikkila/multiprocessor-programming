#ifndef _PANIC_H_
#define _PANIC_H_

void __attribute__((noreturn)) panic(const char* desc);

#endif  // _PANIC_H_