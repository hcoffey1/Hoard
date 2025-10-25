// -*- C++ -*-

/*

  The Hoard Multiprocessor Memory Allocator
  www.hoard.org

  Author: Emery Berger, http://www.emeryberger.com
  Copyright (c) 1998-2020 Emery Berger

  See the LICENSE file at the top-level directory of this
  distribution and at http://github.com/emeryberger/Hoard.

*/

#ifndef HOARD_ALIGNEDSUPERBLOCKHEAP_H
#define HOARD_ALIGNEDSUPERBLOCKHEAP_H

#include <pthread.h>
#include <stdio.h>
#include <stdarg.h>
#include "heaplayers.h"

#include "conformantheap.h"
#include "fixedrequestheap.h"

#ifndef HOARD_TRACE_MSG
#define HOARD_TRACE_MSG(...) do { \
    char __buf[256]; \
    int __n = snprintf(__buf, sizeof(__buf), __VA_ARGS__); \
    if (__n > 0) fputs(__buf, stderr); \
  } while (0)
#endif
namespace Hoard {

  template <size_t SuperblockSize,
	    class TheLockType,
	    class MmapSource>
  class SuperblockStore {
  public:

    enum { Alignment = MmapSource::Alignment };

    SuperblockStore() {
#if defined(__SVR4)
      // We only get 64K chunks from mmap on Solaris, so we need to grab
      // more chunks (and align them to 64K!) for smaller superblock sizes.
      // Right now, we do not handle this case and just assert here that
      // we are getting chunks of 64K.
      //
      // However on illumos, it does not seem to be the case
      // but there is no way to distinguish from solaris
      //static_assert(SuperblockSize == 65536,
      //		    "This is needed to prevent mmap fragmentation.");
#endif
    }
    
    void * malloc (size_t) {
      HOARD_TRACE_MSG("HOARD_TRACE: [TID %lu] SuperblockStore::malloc() called\n", (unsigned long)pthread_self());
      //fflush(stderr);
      
      if (_freeSuperblocks.isEmpty()) {
	// Get more memory.
  HOARD_TRACE_MSG("HOARD_TRACE: [TID %lu] SuperblockStore - free list empty, allocating from MmapSource\n", (unsigned long)pthread_self());
  //fflush(stderr);
      size_t request_bytes = ChunksToGrab * SuperblockSize;
      void * ptr = _superblockSource.malloc (request_bytes);
	if (!ptr) {
	  return nullptr;
	}
            HOARD_TRACE_MSG("HOARD_TRACE: [TID %lu] SuperblockStore - chunk ptr=%p size=%zu\n", (unsigned long)pthread_self(), ptr, request_bytes);
            //fflush(stderr);
	char * p = (char *) ptr;
	for (int i = 0; i < ChunksToGrab; i++) {
	  _freeSuperblocks.insert ((DLList::Entry *) p);
	  p += SuperblockSize;
	}
      } else {
  HOARD_TRACE_MSG("HOARD_TRACE: [TID %lu] SuperblockStore - reusing from free list\n", (unsigned long)pthread_self());
  //fflush(stderr);
      }
      return _freeSuperblocks.get();
    }

    void free (void * ptr) {
      _freeSuperblocks.insert ((DLList::Entry *) ptr);
    }

  private:

#if defined(__linux__)
  enum { DesiredChunkSize = 2 * 1024 * 1024 }; // HeMem expects huge-page granularity
#else
  enum { DesiredChunkSize = SuperblockSize };
#endif

#if defined(__linux__)
  static_assert(DesiredChunkSize % SuperblockSize == 0,
          "Superblock size must divide huge page size");
#endif

  enum { ChunksToGrab = (SuperblockSize >= DesiredChunkSize) ? 1 : (DesiredChunkSize / SuperblockSize) };

    MmapSource _superblockSource;
    DLList _freeSuperblocks;

  };

}


namespace Hoard {

  template <class TheLockType,
	    size_t SuperblockSize,
	    class MmapSource>
  class AlignedSuperblockHeapHelper :
    public ConformantHeap<HL::LockedHeap<TheLockType,
					 FixedRequestHeap<SuperblockSize, 
							  SuperblockStore<SuperblockSize, TheLockType, MmapSource> > > > {};


#if 0

  template <class TheLockType,
	    size_t SuperblockSize>
  class AlignedSuperblockHeap : public AlignedMmap<SuperblockSize,TheLockType> {};


#else

  template <class TheLockType,
	    size_t SuperblockSize,
	    class MmapSource>
  class AlignedSuperblockHeap :
    public AlignedSuperblockHeapHelper<TheLockType, SuperblockSize, MmapSource> {
  public:
    AlignedSuperblockHeap() {
      static_assert(AlignedSuperblockHeapHelper<TheLockType, SuperblockSize, MmapSource>::Alignment % SuperblockSize == 0,
		    "Ensure proper alignment.");
    }

  };
#endif

}

#endif
