

#ifndef RADIUM_COMMON_VALIDATE_MEM_PROC_H
#define RADIUM_COMMON_VALIDATE_MEM_PROC_H

#if 0

#if !defined(RELEASE)
  // Probably want to comment out the line below if compiling with -fsanitize=thread
  #if !defined DISABLE_BDWGC
    #if FOR_LINUX
      #define VALIDATE_MEM 1 // VALIDATE_MEM validates mem allocated by bdwgc that fsanitize=address doesn't. However, for best memory validation, DISABLE_BDWGC should be defined instead.
    #endif
  #endif
#endif

#endif

#if defined(RELEASE)
  #if defined(VALIDATE_MEM)
    #error "oops"
  #endif
#endif


typedef void *(*MemoryAllocator)(size_t size);
typedef void (*MemoryFreeer)(void* mem);

#ifdef __cplusplus
extern "C" {
#endif
  void V_validate(void *mem);
  void V_validate_all(void);
  void V_run_validation_thread(void); // Is run automatically unless DONT_RUN_VALIDATION_THREAD is defined
  
  void *V_alloc(MemoryAllocator allocator, int size, const char *filename, int linenumber);

  void V_free_actual_mem_real_start(MemoryFreeer freeer, void *actual_mem_real_start);
  void V_free_it(MemoryFreeer freeer, void *allocated_mem);

  // too complicated:
  //void *V_realloc_it(MemoryFreeer freeer, void *ptr, size_t size, const char *filename, int linenumber);

  // Use these ones instead of V_realloc_it, and realloc manually.
  MemoryAllocator V_get_MemoryAllocator(void *mem);
  int V_get_size(void *mem);
  
  void *V_allocated_mem_real_start(void *allocated_mem);
  
  void *V_malloc__(size_t size, const char *filename, int linenumber);
  char *V_strdup__(const char *s, const char *filename, int linenumber);
  void *V_calloc__(size_t n, size_t size, const char *filename, int linenumber);
  void V_free__(void *ptr);
  void *V_realloc__(void *ptr, size_t size, const char *filename, int linenumber);
#ifdef __cplusplus
}
#endif




#if !defined(VALIDATE_MEM)

#  if defined(RELEASE)
#    define V_malloc(size) malloc(size)
#    define V_strdup(s) strdup(s)
#    define V_calloc(n, size) calloc(n, size)
#    define V_free(ptr) free((void*)ptr)
#    define V_realloc(ptr, size) realloc(ptr, size);
#  else

static void* my_calloc(size_t size1,size_t size2){
  size_t size = size1*size2;

  char*  ret  = (char*)malloc(size);
  
  // Ensure the memory is physically available.
  for(unsigned int i=0;i<size;i++)
    ret[i] = 0;
  
  return ret;
}

static inline void *V_malloc(size_t size){
  R_ASSERT(!PLAYER_current_thread_has_lock());
  return malloc(size);
}
static inline char *V_strdup(const char *s){
  R_ASSERT(MIXER_is_saving() || !PLAYER_current_thread_has_lock());
  return strdup(s);
}
static inline void *V_calloc(size_t n, size_t size){
  R_ASSERT(!PLAYER_current_thread_has_lock());
  return my_calloc(n,size);
}
static inline void V_free(void *ptr){
  R_ASSERT(!PLAYER_current_thread_has_lock());
  free(ptr);
}
static inline void *V_realloc(void *ptr, size_t size){
  R_ASSERT(!PLAYER_current_thread_has_lock());
  return realloc(ptr, size);
}

#  endif

static inline void V_shutdown(void){
}
  
#else

#  define V_malloc(size) V_malloc__(size, __FILE__, __LINE__)
#  define V_strdup(s) V_strdup__(s, __FILE__, __LINE__)
#  define V_calloc(n, size) V_calloc__(n, size, __FILE__, __LINE__)
#  define V_free(ptr) V_free__((void*)(ptr))
#  define V_realloc(ptr, size) V_realloc__((void*)ptr, size, __FILE__, __LINE__)

#  ifdef __cplusplus
extern "C" {
#  endif
  void V_shutdown(void);
#  ifdef __cplusplus
}
#  endif

#endif // !RELEASE



#if 0

  // This can be added to various files

  void *V_malloc__(size_t size, const char *filename, int linenumber);
  char *V_strdup__(const char *s, const char *filename, int linenumber);
  void *V_calloc__(size_t n, size_t size, const char *filename, int linenumber);
  void V_free__(void *ptr);
  void *V_realloc__(void *ptr, size_t size, const char *filename, int linenumber);

  #define V_malloc(size) V_malloc__(size, __FILE__, __LINE__)
  #define V_strdup(s) V_strdup__(s, __FILE__, __LINE__)
  #define V_calloc(n, size) V_calloc__(n, size, __FILE__, __LINE__)
  #define V_free(ptr) V_free__((void*)(ptr))
  #define V_realloc(ptr, size) V_realloc__((void*)ptr, size, __FILE__, __LINE__)

  #define malloc(a) V_malloc(a)
  #define strdup(s) V_strdup(s)
  #define calloc(a,b) V_calloc(a,b)
  #define realloc(a,b) V_realloc(a,b)
  #define free(a) V_free(a)

#endif

  
#endif
