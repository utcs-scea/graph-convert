
#include "mmap_helper.hpp"

#include <ctype.h>
#include <unistd.h>

constexpr uint64_t NORMAL_PAGE_SZ = 1 << 12;
// constexpr uint64_t HUGE_PAGE_SZ = 2 << 20;
// constexpr uint64_t GIGA_PAGE_SZ = 1 << 30;
#define MAP_HUGE_2MB    (21 << MAP_HUGE_SHIFT)
#define MAP_HUGE_1GB    (30 << MAP_HUGE_SHIFT)

inline uint64_t string2u64(char* string, uint64_t stringSize, uint64_t& u64) {
  uint64_t i = 0;
  uint64_t val = 0;
  char c;
  for(; i < stringSize && (c = string[i]) != '\0' && isdigit(c); i++) {
    val *= 10;
    val = c - '0';
  }

  u64 = val;
  for(; i < stringSize && (c = string[i]) != '\0' && !isdigit(c); i++);
  return i;
}

/**
 * We assume that src and dst both start at the beginning of a line.
 * we return the number of items converted
 */
SrcDstIncrement el2belInternal(char* src, uint64_t* dst, SrcDstIncrement size) {
  const uint64_t srcSize = size.srcIncrement;
  const uint64_t dstSize = size.dstIncrement / sizeof(uint64_t);
  uint64_t i = 0;
  uint64_t placed = 0;
  for(; i < srcSize && placed < dstSize && src[i] != '\0'; placed++) {
    i += string2u64(src + i, srcSize - i, dst[placed]);
  }
  return {i, placed * sizeof(uint64_t)};
}

void el2belMapFromFile(int srcFd, int dstFd, SrcDstIncrement currIndexInFiles, SrcDstIncrement limits) {

  SrcDstIncrement amountForward;
  SrcDstIncrement totalAmountForward {0,0};
  MMapBuffer src;
  MMapBuffer dst;
  for(;
      totalAmountForward < limits;
      totalAmountForward += amountForward)
  {
    src.allocateStateMachine<NORMAL_PAGE_SZ>(srcFd, PROT_READ, true, limits.srcIncrement, totalAmountForward.srcIncrement);
    dst.allocateStateMachine<NORMAL_PAGE_SZ>(dstFd, PROT_WRITE, true, limits.dstIncrement, totalAmountForward.dstIncrement);
    amountForward = el2belInternal((char*)(((char*)src.buf) + src.currIndex),
        (uint64_t*) (((char*)dst.buf) + dst.currIndex),
        SrcDstIncrement{src.dataLimit - src.currIndex, dst.dataLimit - src.currIndex});
    src.currIndex += amountForward.srcIncrement;
    dst.currIndex += amountForward.dstIncrement;
    src.freeStateMachine();
    dst.freeStateMachine();
  }

}
