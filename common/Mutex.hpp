

#ifndef _RADIUM_COMMON_MUTEX_HPP
#define _RADIUM_COMMON_MUTEX_HPP

#include <pthread.h>
#include <sys/time.h>

#ifdef FOR_MACOSX
# include <errno.h>
# include <sys/types.h>
# include <sys/socket.h>
#endif


// QMutex/QMutexLocker/QWaitCondition, which were used everywhere, and worked great othervice, didn't work with tsan, so that's the reason for this file.
// Tried to use boost instead first, but it generated too much drama. (took too much time trying to understand the APIs, plus that I had to add -Wno-unused-variable as a compiler flag and link with two boost libraries)
//
// However, seems like pthread works just fine with mingw, so this is just fine.



namespace radium {

//clock_gettime is not implemented on OSX, but for the type of usage in this file, it's absolutely no problem at all using gettimeofday instead, so we just as well do that on all platforms. Code copied from http://stackoverflow.com/questions/5167269/clock-gettime-alternative-in-mac-os-x
static inline int my_clock_gettime(struct timespec* t) {
    struct timeval now;
    int rv = gettimeofday(&now, NULL);
    if (rv) return rv;
    t->tv_sec  = now.tv_sec;
    t->tv_nsec = now.tv_usec * 1000;
    return 0;
}
  

struct Mutex {

  pthread_mutex_t mutex;

  Mutex(bool is_recursive = false){
    if (is_recursive){
      pthread_mutexattr_t attr;
      pthread_mutexattr_init(&attr);
      pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
      pthread_mutex_init(&mutex, &attr);
    } else {
      pthread_mutex_init(&mutex, NULL);
    }
  }

  ~Mutex(){
    pthread_mutex_destroy(&mutex);
  }

  void lock(void){
    pthread_mutex_lock(&mutex);
  }

  void unlock(void){
    pthread_mutex_unlock(&mutex);
  }
};

struct ScopedMutex{
  Mutex *mutex;
  
  ScopedMutex(Mutex *mutex)
    : mutex(mutex)
  {
    mutex->lock();
  }

  ~ScopedMutex(){
    mutex->unlock();
  }
};



struct CondWait {

  pthread_cond_t cond;

  CondWait(){
    pthread_cond_init(&cond,NULL);
  }

  ~CondWait(){
    pthread_cond_destroy(&cond);
  }

  bool wait(Mutex *mutex, int timeout_in_milliseconds) {
    struct timespec ts;

    R_ASSERT( (timeout_in_milliseconds % 1000) == 0); // Only whole seconds are supported for now (don't bother to support milliseconds until it's actually needed)

    my_clock_gettime(&ts);
    ts.tv_sec += timeout_in_milliseconds/1000;

    int ret = pthread_cond_timedwait(&cond,&mutex->mutex,&ts);
    if (ret==0)
      return true;
    if (ret==ETIMEDOUT)
      return false;

    if (ret==EINVAL)
      RError("pthread_cond_wait returned EINVAL");
    else if (ret==EPERM)
      RError("pthread_cond_wait returned EPERM");
#ifndef RELEASE
    else
      RError("Unknown return message from pthread_cond_wait: %d",ret);
#endif
    return true;
  }

  void wait(Mutex *mutex){
    int ret =  pthread_cond_wait(&cond,&mutex->mutex);
    if (ret==EINVAL)
      RError("pthread_cond_wait returned EINVAL");
    else if (ret==EPERM)
      RError("pthread_cond_wait returned EINVAL");
#ifndef RELEASE
    else if (ret!=0)
      RError("Unknown return message from pthread_cond_wait: %d",ret);
#endif
  }

  void notify_one(void){
    pthread_cond_signal(&cond);
  }

  void notify_all(void){
    pthread_cond_broadcast(&cond);
  }
};


}



#endif
