#ifndef _SRCDST_HPP_
#define _SRCDST_HPP_
#include <cstdint>

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

#endif // _SRCDST_HPP_
