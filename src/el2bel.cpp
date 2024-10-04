
#include "mmap_helper.hpp"

#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <omp.h>
#include <fcntl.h>
#include <linux/limits.h>

constexpr uint64_t NORMAL_PAGE_SZ = 1 << 12;
// constexpr uint64_t HUGE_PAGE_SZ = 2 << 20;
// constexpr uint64_t GIGA_PAGE_SZ = 1 << 30;
#define MAP_HUGE_2MB    (21 << MAP_HUGE_SHIFT)
#define MAP_HUGE_1GB    (30 << MAP_HUGE_SHIFT)

constexpr bool isdigitHand(char c) {
  return '0' <= c && c <= '9';
}

inline uint64_t string2u64(char* string, uint64_t stringSize, uint64_t& u64) {
  uint64_t i = 0;
  uint64_t val = 0;
  char c;
  for(; i < stringSize && isdigitHand(c = string[i]); i++) {
    val *= 10;
    val += c - '0';
  }

  u64 = val;
  for(; i < stringSize && !isdigitHand(string[i]); i++);
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
  for(; i < srcSize && !isdigitHand(src[i]); i++);
  for(; i < srcSize && placed < dstSize; placed++) {
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
      currIndexInFiles + totalAmountForward < limits;
      totalAmountForward += amountForward)
  {
    src.allocateStateMachineToFirstValue<NORMAL_PAGE_SZ>(srcFd,
        PROT_READ, false, limits.srcIncrement,
        currIndexInFiles.srcIncrement + totalAmountForward.srcIncrement,
        [](char c){ return isdigit(c);} );
    dst.allocateStateMachine<NORMAL_PAGE_SZ>(dstFd,
        PROT_WRITE, true, limits.dstIncrement,
        currIndexInFiles.dstIncrement + totalAmountForward.dstIncrement);
    amountForward = el2belInternal((char*)(((char*)src.buf) + src.currIndex),
        (uint64_t*) (((char*)dst.buf) + dst.currIndex),
        SrcDstIncrement{src.dataLimit - src.currIndex, dst.dataLimit - dst.currIndex});
    src.currIndex += amountForward.srcIncrement;
    dst.currIndex += amountForward.dstIncrement;
    src.freeStateMachine();
    dst.freeStateMachine();
  }

  dst.forceFree();
  int res = ftruncate(dstFd, totalAmountForward.dstIncrement);
  if(res) throw "Truncate failed";
}

char pathName[PATH_MAX];

inline uint64_t getStartBound(std::uint64_t tid, std::uint64_t numThreads, int fd, std::uint64_t fileSize) {
  char c;
  const uint64_t chunkSize = fileSize/numThreads;
  uint64_t bound = 0;
  if(tid == 0) {
    return 0;
  } else if (tid == numThreads){
    return fileSize;
  } else {
    bound = tid * (chunkSize);
    for(; bound != 0 && pread(fd, &c, 1, bound) && c != '\n'; bound--);
    return bound;
  }
}

int el2belConvert(int inputFileFd, const char* outputFileStub) {
  struct stat fileinfo;
  int rc = fstat(inputFileFd, &fileinfo);
  if(rc) throw "Failed to get fileinfo";

  #pragma omp parallel private(pathName)
  {
    uint64_t tid;
    tid = omp_get_thread_num();
    // 20 for uint64_t max  and then 2 for '\0' and '-'
    char* outputFileName = pathName;
    snprintf(outputFileName, PATH_MAX, "%s-%lu", outputFileStub, tid);
    outputFileName[PATH_MAX - 1] = '\0';
    int outputFileFd = open(outputFileName, O_RDWR | O_LARGEFILE | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if(outputFileFd < 0) {
      throw "Failed to open Output file.";
    }

    uint64_t numThreads = omp_get_num_threads();
    uint64_t startBound = getStartBound(tid, numThreads, inputFileFd, fileinfo.st_size);
    uint64_t endBound = getStartBound(tid + 1, numThreads, inputFileFd, fileinfo.st_size);
    el2belMapFromFile(inputFileFd, outputFileFd, {startBound, 0}, {endBound, UINT64_MAX});
    close(outputFileFd);
  }

  return 0;
}
