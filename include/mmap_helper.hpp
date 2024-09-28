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

  template<uint64_t PAGE_SIZE>
  void allocateStateMachine(int fd, int prot, bool isPublic, uint64_t limits, uint64_t currIndex) {
    assert(limits >= currIndex);
    if(limits == currIndex) return;
    assert(!buf);
    this->currIndex = 0;
    this->dataLimit = std::min(limits - currIndex, PAGE_SIZE);
    int privateFlag = (isPublic) ? MAP_SHARED : MAP_PRIVATE;
    if(prot == PROT_WRITE) {
      int ret = ftruncate(fd, currIndex + dataLimit);
      if(ret) {
        throw "ftruncate failed";
      }
    }
    buf = mmap(nullptr, this->dataLimit, prot, privateFlag |MAP_POPULATE, fd, currIndex);
    if(buf == MAP_FAILED) throw "Mmap Failed";
  }
};

struct SrcDstIncrement {
  uint64_t srcIncrement;
  uint64_t dstIncrement;

  constexpr SrcDstIncrement() : srcIncrement(0), dstIncrement(0) {}

  constexpr SrcDstIncrement(uint64_t src, uint64_t dst) : srcIncrement(src), dstIncrement(dst) {}

  constexpr ~SrcDstIncrement() {}

  bool operator==(const SrcDstIncrement& other) const {
    return (srcIncrement == other.srcIncrement) && (dstIncrement == other.dstIncrement);
  }
  bool operator<(const SrcDstIncrement& other) const {
    return (srcIncrement < other.srcIncrement) || (dstIncrement < other.dstIncrement);
  }
  SrcDstIncrement operator+=(const SrcDstIncrement& other) {
    srcIncrement += other.srcIncrement;
    dstIncrement += other.dstIncrement;
    return *this;
  }
};

void el2belMapFromFile(int srcFd, int dstFd, SrcDstIncrement currIndexInFiles, SrcDstIncrement limits);

#undef MAP_HUGE_2MB
#undef MAP_HUGE_1GB

#endif // _MMAP_HELPER_HPP_
