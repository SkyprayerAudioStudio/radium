/* Copyright 2000 Kjetil S. Matheussen

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. */


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>


#include "t_gc_proc.h"
#include "nsmtracker.h"
#include "visual_proc.h"
#include "control_proc.h"
#include "OS_memory_proc.h"
#include "OS_Player_proc.h"
#include "threading.h"
#include "memory_proc.h"


extern struct Root *root;

size_t tmemorysize=32000;		/* Hardcoded, must be unhardcoded later. */
int tmemoryisused=1;

void *tmemory;


void init_memory(void){
#ifndef DISABLE_BDWGC
  //	tmemory=GC_malloc_atomic(tmemorysize);
  //	tmemoryisused=0;
#endif
	//return (int) tmemory;
        return;
}


void tfree(void *element){
#ifdef DISABLE_BDWGC

	if(element==NULL){
		RError("Warning, attempting to free NULL\n");
		return;
	}

	OS_tfree(element);

#else

#  if defined(RELEASE)
	GC_free(element);
#  else
        V_free_it(GC_free, element);
#  endif

#endif

}



void ShutDownYepp(void){

	EndProgram();

	exit(1);
}

size_t allocated=0;


#if !defined(RELEASE)
static void dummyfree(void *data){
}

static void gcfinalizer(void *actual_mem_start, void *user_data){
  //printf("gcfinalizer called for %p\n",actual_mem_start);
  V_free_actual_mem_real_start(dummyfree, actual_mem_start);
}

#endif

void *tracker_alloc__(size_t size,void *(*AllocFunction)(size_t size2), const char *filename, int linenumber){
	allocated+=size;

        R_ASSERT(THREADING_is_main_thread());
        R_ASSERT(!PLAYER_current_thread_has_lock());

#ifndef DISABLE_BDWGC
#	ifdef _AMIGA
		return (*GC_amiga_allocwrapper_do)(size,AllocFunction);
#	else
#          if defined(RELEASE)               
                void *ret = AllocFunction(size);
#          else                
		void *ret = V_alloc(AllocFunction,size,filename,linenumber);
                void *actual_mem_start = V_allocated_mem_real_start(ret);
                GC_register_finalizer(actual_mem_start, gcfinalizer, NULL, NULL, NULL);
#          endif
#	endif


        return ret;

#else
	return OS_getmem(size);		// For debugging. (wrong use of GC_malloced memory could be very difficult to trace)
#endif



}

void *tralloc__(size_t size, const char *filename, int linenumber){
  return tracker_alloc__(size,GC_malloc, filename, linenumber);
}

void *tralloc_atomic__(size_t size, const char *filename, int linenumber){
  return tracker_alloc__(size,GC_malloc_atomic, filename, linenumber);
}


/************************************************************
   FUNCTION
     Does never return NULL.
     Clears memory.
************************************************************/
void *talloc__(size_t size, const char *filename, int linenumber){ ///, const char *filename, int linenumber){
	void *ret;

	ret=tracker_alloc__(size,GC_malloc, filename, linenumber);

	if(ret!=NULL) return ret;

        RWarning("Out of memory. Trying to get memory a different way. You should save and quit.");

        ret=calloc(1,size);
	if(ret!=NULL) return ret;


	RWarning("\n\n\n Didn't succeed. OUT OF MEMORY. Have to quit\n\n\n");
	ShutDownYepp();
	return NULL;
}

void *talloc_atomic__(size_t size, const char *filename, int linenumber){
	void *ret;

        //return GC_malloc_atomic(size);
        
	ret=tracker_alloc__(size,GC_malloc_atomic, filename, linenumber);

	if(ret!=NULL) return ret;

#if 0
	if(tmemoryisused==0){
		tmemoryisused=1;
		GC_free(tmemory);
		tmemory=NULL;
	}

#  ifndef DISABLE_BDWGC
	ret=GC_malloc_atomic(size);
#  else
	ret=OS_getmem(size);		// For debugging. (wrong use of GC_malloced memory could be very difficult to trace)
#  endif

#endif
        RWarning("Out of memory. I'll try to continue by allocating a different way, but you should save and quit now.\n");

        ret = calloc(1,size);
        
	if(ret!=NULL) return ret;


        RWarning("Out of memory. Quitting\n");
	ShutDownYepp();

	return NULL;
}


void *talloc_atomic_uncollectable__(size_t size, const char *filename, int linenumber){
	void *ret;

	ret=tracker_alloc__(size,GC_malloc_atomic_uncollectable, filename, linenumber);

	if(ret!=NULL) return ret;

#if 0
	if(tmemoryisused==0){
		tmemoryisused=1;
		GC_free(tmemory);
		tmemory=NULL;
	}

#ifndef DISABLE_BDWGC
 	ret=GC_malloc_atomic_uncollectable(size);
#else
	ret=OS_getmem(size);		// For debugging. (wrong use of GC_malloced memory could be very difficult to trace)
#endif

#endif

        RWarning("Out of memory. I'll try to continue by allocating a different way, but you should save and quit now.\n");
        ret = calloc(1,size);
        
	if(ret!=NULL) return ret;


        RWarning("Didn't succeed. Out of memory. Quitting. (atomic_uncollectable allocator)\n");
	ShutDownYepp();
	return NULL;
}

void *talloc_atomic_clean__(size_t size, const char *filename, int linenumber){
  void *ret = talloc_atomic__(size, filename, linenumber);
  memset(ret, 0, size);
  return ret;
}

void *talloc_realloc__(void *v, size_t new_size, const char *filename, int linenumber){
#ifdef DISABLE_BDWGC
  return realloc(v,new_size);
#else
# if defined(RELEASE)
  return GC_realloc(v,new_size);
# else
  {
    MemoryAllocator allocator = V_get_MemoryAllocator(v);
    int old_size = V_get_size(v);
    
    void *new_mem = tracker_alloc__(new_size, allocator, filename, linenumber);
    memcpy(new_mem, v, R_MIN((int)new_size, old_size));

    return new_mem;
  }
# endif
#endif
}

char *talloc_strdup__(const char *input, const char *filename, int linenumber) {
  if(input==NULL)
    return NULL;
  char *ret = talloc_atomic__(strlen(input) + 1, filename, linenumber);
  sprintf(ret,"%s",input);
  return ret;
}

char *talloc_numberstring__(int number, const char *filename, int linenumber){
  char s[1000];
  sprintf(s,"%d",number);
  return talloc_strdup__(s, filename, linenumber);
}

char *talloc_floatstring__(float number, const char *filename, int linenumber){
  char s[1000];
  snprintf(s,999,"%f",number);
  return talloc_strdup__(s, filename, linenumber);
}


char *talloc_format(const char *fmt,...){
  int size = 16;
  char *ret = talloc_atomic__(size,__FILE__,__LINE__);

  for(;;){
    va_list argp;

    va_start(argp,fmt);
    int len = vsnprintf(ret,size-2,fmt,argp);
    va_end(argp);

    if (len <= 0) {
      size = size * 2;
      ret = talloc_realloc__(ret, size,__FILE__,__LINE__);
    } else if (len >= size-3) {
      size = len + 16;
      ret = talloc_realloc__(ret, size,__FILE__,__LINE__);
    } else
      break;
  }

#if !defined(RELEASE)
  V_validate(ret);
#endif
  
  return ret;
}
