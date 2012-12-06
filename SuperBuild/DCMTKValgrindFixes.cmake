set(oflist ${DCMTK_SOURCE}/ofstd/libsrc/oflist.cc)
set(ofthread ${DCMTK_SOURCE}/ofstd/libsrc/ofthread.cc)

file(READ ${oflist} code)
string(REPLACE
"OFListBase::~OFListBase()
{
    base_clear();
    if (afterLast)
        delete afterLast;
}"
"OFListBase::~OFListBase()
{
    base_clear();
    if (afterLast)
        {
        delete afterLast;
        afterLast = NULL;
        }
}"
code "${code}"
)
file(WRITE ${oflist} "${code}")



file(READ ${ofthread} code)
string(REPLACE
"OFThreadSpecificData::~OFThreadSpecificData()
{
#ifdef WINDOWS_INTERFACE
  if (theKey) TlsFree(* OFthread_cast(DWORD *, theKey));
#elif defined(POSIX_INTERFACE)
  delete OFthread_cast(pthread_key_t *, theKey);
#elif defined(SOLARIS_INTERFACE)
  delete OFthread_cast(thread_key_t *, theKey);
#else
#endif
}"
"OFThreadSpecificData::~OFThreadSpecificData()
{
#ifdef WINDOWS_INTERFACE
  if (theKey) TlsFree(* OFthread_cast(DWORD *, theKey));
  theKey = NULL;
#elif defined(POSIX_INTERFACE)
  delete OFthread_cast(pthread_key_t *, theKey);
#elif defined(SOLARIS_INTERFACE)
  delete OFthread_cast(thread_key_t *, theKey);
#else
#endif
}
"
code "${code}")

string(REPLACE
"OFSemaphore::~OFSemaphore()
{
#ifdef WINDOWS_INTERFACE
  CloseHandle((HANDLE)theSemaphore);
#elif defined(POSIX_INTERFACE)
  if (theSemaphore) sem_destroy(OFthread_cast(sem_t *, theSemaphore));
  delete OFthread_cast(sem_t *, theSemaphore);
#elif defined(SOLARIS_INTERFACE)
  if (theSemaphore) sema_destroy(OFthread_cast(sema_t *, theSemaphore));
  delete OFthread_cast(sema_t *, theSemaphore);
#else
#endif
}"
"OFSemaphore::~OFSemaphore()
{
#ifdef WINDOWS_INTERFACE
  CloseHandle((HANDLE)theSemaphore);
#elif defined(POSIX_INTERFACE)
  if (theSemaphore)
      {
      sem_destroy(OFthread_cast(sem_t *, theSemaphore));
      delete OFthread_cast(sem_t *, theSemaphore);
      theSemaphore = NULL;
      }
#elif defined(SOLARIS_INTERFACE)
  if (theSemaphore)
      {
      sema_destroy(OFthread_cast(sema_t *, theSemaphore));
      delete OFthread_cast(sema_t *, theSemaphore);
      theSemaphore = NULL;
      }
#else
#endif
}"
code "${code}")
string(REPLACE
"OFMutex::~OFMutex()
{
#ifdef WINDOWS_INTERFACE
  CloseHandle((HANDLE)theMutex);
#elif defined(POSIX_INTERFACE)
  if (theMutex) pthread_mutex_destroy(OFthread_cast(pthread_mutex_t *, theMutex));
  delete OFthread_cast(pthread_mutex_t *, theMutex);
#elif defined(SOLARIS_INTERFACE)
  if (theMutex) mutex_destroy(OFthread_cast(mutex_t *, theMutex));
  delete OFthread_cast(mutex_t *, theMutex);
#else
#endif
}
"
"OFMutex::~OFMutex()
{
#ifdef WINDOWS_INTERFACE
  CloseHandle((HANDLE)theMutex);
#elif defined(POSIX_INTERFACE)
  if (theMutex)
      {
      pthread_mutex_destroy(OFthread_cast(pthread_mutex_t *, theMutex));
      delete OFthread_cast(pthread_mutex_t *, theMutex);
      theMutex = NULL;
      }
#elif defined(SOLARIS_INTERFACE)
  if (theMutex)
      {
      mutex_destroy(OFthread_cast(mutex_t *, theMutex));
      delete OFthread_cast(mutex_t *, theMutex);
      theMutex = NULL;
      }
#else
#endif
}
"
code "${code}")
string(REPLACE
"OFReadWriteLock::~OFReadWriteLock()
{
#if defined(WINDOWS_INTERFACE) || defined(POSIX_INTERFACE_WITHOUT_RWLOCK)
  delete OFthread_cast(OFReadWriteLockHelper *, theLock);
#elif defined(POSIX_INTERFACE)
  if (theLock) pthread_rwlock_destroy(OFthread_cast(pthread_rwlock_t *, theLock));
  delete OFthread_cast(pthread_rwlock_t *, theLock);
#elif defined(SOLARIS_INTERFACE)
  if (theLock) rwlock_destroy(OFthread_cast(rwlock_t *, theLock));
  delete OFthread_cast(rwlock_t *, theLock);
#else
#endif
}
"

"OFReadWriteLock::~OFReadWriteLock()
{
#if defined(WINDOWS_INTERFACE) || defined(POSIX_INTERFACE_WITHOUT_RWLOCK)
  delete OFthread_cast(OFReadWriteLockHelper *, theLock);
#elif defined(POSIX_INTERFACE)
  if (theLock)
      {
      pthread_rwlock_destroy(OFthread_cast(pthread_rwlock_t *, theLock));
      delete OFthread_cast(pthread_rwlock_t *, theLock);
      theLock = 0;
      }
#elif defined(SOLARIS_INTERFACE)
  if (theLock)
      {
      rwlock_destroy(OFthread_cast(rwlock_t *, theLock));
      delete OFthread_cast(rwlock_t *, theLock);
      theLock = 0;
      }
#else
#endif
}
"

code "${code}")

file(WRITE ${ofthread} "${code}")

