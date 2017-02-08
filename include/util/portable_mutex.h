#ifdef WIN32

#include <Windows.h>
#define portable_mutex HANDLE
#define portable_mutex_create(mutex) mutex = CreateMutex(NULL, FALSE, NULL)
#define portable_mutex_lock(mutex) if(mutex) WaitForSingleObject(mutex, INFINITE)
#define portable_mutex_unlock(mutex) if(mutex) ReleaseMutex(mutex)
#define portable_mutex_destroy(mutex) CloseHandle(mutex); mutex = NULL

#else

#include <pthread.h>
#define portable_mutex pthread_mutex_t 
#define portable_mutex_create(mutex) pthread_mutex_init(&mutex, NULL)
#define portable_mutex_lock(mutex) pthread_mutex_lock(&mutex)
#define portable_mutex_unlock(mutex) pthread_mutex_unlock(&mutex)
#define portable_mutex_destroy(mutex)

#endif