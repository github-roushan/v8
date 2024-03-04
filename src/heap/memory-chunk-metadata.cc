// Copyright 2019 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/heap/memory-chunk-metadata.h"

#include <cstdlib>

#include "src/heap/heap-write-barrier-inl.h"
#include "src/heap/incremental-marking.h"
#include "src/heap/marking-inl.h"
#include "src/objects/heap-object.h"
#include "src/utils/allocation.h"

namespace v8 {
namespace internal {

// static
constexpr MemoryChunkMetadata::MainThreadFlags MemoryChunk::kAllFlagsMask;
// static
constexpr MemoryChunkMetadata::MainThreadFlags
    MemoryChunk::kPointersToHereAreInterestingMask;
// static
constexpr MemoryChunkMetadata::MainThreadFlags
    MemoryChunk::kPointersFromHereAreInterestingMask;
// static
constexpr MemoryChunkMetadata::MainThreadFlags
    MemoryChunk::kEvacuationCandidateMask;
// static
constexpr MemoryChunkMetadata::MainThreadFlags
    MemoryChunk::kIsInYoungGenerationMask;
// static
constexpr MemoryChunkMetadata::MainThreadFlags MemoryChunk::kIsLargePageMask;
// static
constexpr MemoryChunkMetadata::MainThreadFlags
    MemoryChunk::kSkipEvacuationSlotsRecordingMask;

MemoryChunkMetadata::MemoryChunkMetadata(Heap* heap, BaseSpace* space,
                                   size_t chunk_size, Address area_start,
                                   Address area_end, VirtualMemory reservation)
    : size_(chunk_size),
      heap_(heap),
      area_start_(area_start),
      area_end_(area_end),
      allocated_bytes_(area_end - area_start),
      high_water_mark_(area_start - address()),
      owner_(space),
      reservation_(std::move(reservation)) {}

bool MemoryChunkMetadata::InOldSpace() const {
  return owner()->identity() == OLD_SPACE;
}

bool MemoryChunkMetadata::InLargeObjectSpace() const {
  return owner()->identity() == LO_SPACE;
}

#ifdef THREAD_SANITIZER
void MemoryChunkMetadata::SynchronizedHeapLoad() const {
  CHECK(reinterpret_cast<Heap*>(
            base::Acquire_Load(reinterpret_cast<base::AtomicWord*>(&(
                const_cast<MemoryChunkMetadata*>(this)->heap_)))) != nullptr ||
        IsFlagSet(READ_ONLY_HEAP));
}
#endif

class BasicMemoryChunkValidator {
  // Computed offsets should match the compiler generated ones.
  static_assert(MemoryChunkLayout::kSizeOffset ==
                offsetof(MemoryChunkMetadata, size_));
  static_assert(MemoryChunkLayout::kFlagsOffset ==
                offsetof(MemoryChunkMetadata, main_thread_flags_));
  static_assert(MemoryChunkLayout::kHeapOffset ==
                offsetof(MemoryChunkMetadata, heap_));
  static_assert(offsetof(MemoryChunkMetadata, size_) ==
                MemoryChunkLayout::kSizeOffset);
  static_assert(offsetof(MemoryChunkMetadata, heap_) ==
                MemoryChunkLayout::kHeapOffset);
  static_assert(offsetof(MemoryChunkMetadata, area_start_) ==
                MemoryChunkLayout::kAreaStartOffset);
  static_assert(offsetof(MemoryChunkMetadata, area_end_) ==
                MemoryChunkLayout::kAreaEndOffset);
  static_assert(offsetof(MemoryChunkMetadata, allocated_bytes_) ==
                MemoryChunkLayout::kAllocatedBytesOffset);
  static_assert(offsetof(MemoryChunkMetadata, wasted_memory_) ==
                MemoryChunkLayout::kWastedMemoryOffset);
  static_assert(offsetof(MemoryChunkMetadata, high_water_mark_) ==
                MemoryChunkLayout::kHighWaterMarkOffset);
  static_assert(offsetof(MemoryChunkMetadata, owner_) ==
                MemoryChunkLayout::kOwnerOffset);
  static_assert(offsetof(MemoryChunkMetadata, reservation_) ==
                MemoryChunkLayout::kReservationOffset);
};

}  // namespace internal
}  // namespace v8