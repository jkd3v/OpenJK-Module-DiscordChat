#ifdef WIN32

#include <Windows.h>
#define portable_thread HANDLE
#define portable_thread_id DWORD
#define portable_thread_create(thread, threadId, fct, args) thread = CreateThread(NULL, 0, fct, args, 0, &threadId);
#define portable_thread_wait(thread) WaitForSingleObject(thread, INFINITE);
#define portable_thread_clear(thread) CloseHandle(thread);
#define portable_thread_f_return DWORD WINAPI
#define portable_thread_f_parameter LPVOID

#else

#include <pthread.h>
#define portable_thread pthread_t
#define portable_thread_id int
#define portable_thread_create(thread, threadId, fct, args) threadId = pthread_create(&thread, NULL, fct, args);
#define portable_thread_wait(thread) pthread_join(thread, NULL);
#define portable_thread_clear(thread)
#define portable_thread_f_return void *
#define portable_thread_f_parameter void *

#endif