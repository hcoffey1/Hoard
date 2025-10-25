# Hoard Integration Changes

This fork carries several allocator adjustments so Hoard can coexist with HeMem's 2 MB huge-page assumptions and the `pebs_allocator` hooks.

- **2 MB Superblocks**
  - `SUPERBLOCK_SIZE` is now `2 * 1024 * 1024` in `include/hoard/hoardheap.h` so every superblock spans an entire huge page.
  - `SuperblockStore` (`include/superblocks/alignedsuperblockheap.h`) requests memory from `MmapSource` in huge-page multiples and slices that mapping into individual superblocks.

- **Safe Superblock Recycling**
  - `HoardManager::free` (`include/hoard/hoardmanager.h`) now validates the derived superblock header before touching heap metadata, bailing out early if corruption or a foreign pointer is detected.
  - `HoardSuperblockHeader::free` (`include/hoard/hoardsuperblockheader.h`) includes a guard that flags (via the `HOARD_TRACE_MSG` hook) when the free-list accounting would overflow, helping diagnose double-frees without crashing Hoard.

- **ManageOneSuperblock Telemetry**
  - `ManageOneSuperblock` (`include/superblocks/manageonesuperblock.h`) emits structured trace points for superblock acquisition, exhaustion, and remote frees. The macro backing these traces is currently a no-op, so production builds stay quiet while a debug build can re-enable the messages by redefining `HOARD_TRACE_MSG`.

- **Trace Suppression and Warning Cleanup**
  - `HOARD_TRACE_MSG` in every Hoard header resolves to an empty macro so allocator internals no longer spam stderr during HeMem runs.
  - `alignedmmap.h` now gates the "already aligned" fast path on both divisibility and relative alignment to avoid enum-conversion warnings with 2 MB alignments, and `source/unixtls.cpp` no longer declares an unused static pointer when forcing TLS initialization.

These changes, combined with the earlier `SuperblockStore` adjustments, let Hoard deliver superblocks that line up with HeMem's 2 MB migration granularity while keeping the allocator's safety checks intact.
