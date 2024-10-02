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
    this->dataLimit = std::min(limits - fileIndex + this->currIndex, PAGE_SIZE);
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
  uint64_t allocateStateMachineToFirstValue(int fd, int prot, bool isPublic, uint64_t limits, uint64_t fileIndex, F matcher) {
  if(stillActive(limits, fileIndex)) return 0;
  allocateStateMachine<PAGE_SIZE>(fd, prot, isPublic, limits, fileIndex);
  // Check for end of current file slice
  if(limits == fileIndex + this->dataLimit) return 0;
  // Backtrack for safety reasons to ensure boundary values are fine
  uint64_t ret = 0;
  for(; matcher(((char*)this->buf)[this->dataLimit - 1]); this->dataLimit--, ret++);
  return ret;
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
    return (srcIncrement < other.srcIncrement) && (dstIncrement < other.dstIncrement);
  }
  SrcDstIncrement operator+=(const SrcDstIncrement& other) {
    srcIncrement += other.srcIncrement;
    dstIncrement += other.dstIncrement;
    return *this;
  }

  SrcDstIncrement operator+(const SrcDstIncrement& other) {
    return SrcDstIncrement{srcIncrement + other.srcIncrement, dstIncrement + other.dstIncrement};
  }
};

void el2belMapFromFile(int srcFd, int dstFd, SrcDstIncrement currIndexInFiles, SrcDstIncrement limits);

int el2belConvert(int inputFileFd, const char* outputFileStub);

#undef MAP_HUGE_2MB
#undef MAP_HUGE_1GB

#endif // _MMAP_HELPER_HPP_
