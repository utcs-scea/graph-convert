#ifndef _MMAP_HELPER_HPP_
#define _MMAP_HELPER_HPP_
#include <cstdint>
#include <cassert>
#include <sys/mman.h>
#include <algorithm>
#include <unistd.h>

#define MAP_HUGE_2MB    (21 << MAP_HUGE_SHIFT)
#define MAP_HUGE_1GB    (30 << MAP_HUGE_SHIFT)

struct MMapBuffer {
  void* buf;
  uint64_t currIndex;
  uint64_t dataLimit;

  MMapBuffer(): buf(nullptr), currIndex(0), dataLimit(0) {}

  void forceFree() {
    if(buf) {
      int ret = munmap(buf, dataLimit);
      if(ret) {
        throw "Munmap failed.";
      }
      buf = nullptr;
    }
  }

  void freeStateMachine() {
    assert(buf);
    if(currIndex == dataLimit) {
      int ret = munmap(buf, dataLimit);
      if(ret) {
        throw "Munmap failed.";
      }
      buf = nullptr;
    }
  }

  bool stillActive(uint64_t limits, uint64_t fileIndex) const {
    assert(limits >= currIndex);
    return (limits > fileIndex) && (this->currIndex < this->dataLimit);
  }

  template<uint64_t PAGE_SIZE>
  void allocateStateMachine(int fd, int prot, bool isPublic, uint64_t limits, uint64_t fileIndex) {
    if(stillActive(limits, fileIndex)) return;
    assert(!buf);
    uint64_t lower4kBoundary = fileIndex & ~(0xFFF);
    this->currIndex = fileIndex - lower4kBoundary;
    //Must map two pages in order to ensure that the next part of the page can be mapped
    this->dataLimit = std::min(limits - fileIndex + this->currIndex, 2 * PAGE_SIZE);
    int privateFlag = (isPublic) ? MAP_SHARED : MAP_PRIVATE;
    if(prot == PROT_WRITE) {
      int ret = ftruncate(fd, lower4kBoundary + dataLimit);
      if(ret) {
        throw "ftruncate failed";
      }
    }
    buf = mmap(nullptr, this->dataLimit, prot, privateFlag |MAP_POPULATE, fd, lower4kBoundary);
    if(buf == MAP_FAILED) throw "Mmap Failed";
  }

  template<uint64_t PAGE_SIZE, typename F>
  void allocateStateMachineToFirstValue(int fd, int prot, bool isPublic, uint64_t limits, uint64_t fileIndex, F matcher) {
    if(stillActive(limits, fileIndex)) return;
    allocateStateMachine<PAGE_SIZE>(fd, prot, isPublic, limits, fileIndex);
    // Check for end of current file slice
    if(limits == fileIndex + this->dataLimit - currIndex) return;
    // Backtrack for safety reasons to ensure boundary values are fine
    for(; matcher(((char*)this->buf)[this->dataLimit - 1]); this->dataLimit--);
  }

};

#undef MAP_HUGE_2MB
#undef MAP_HUGE_1GB

#endif // _MMAP_HELPER_HPP_
