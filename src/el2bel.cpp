
#include "mmap_helper.hpp"

#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <omp.h>
#include <fcntl.h>

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
  for(char c = string[i]; i < stringSize && isdigit(c); i++, c = string[i]) {
    val *= 10;
    val += c - '0';
  }

  u64 = val;
  for(char c = string[i]; i < stringSize && !isdigit(c); i++, c = string[i]);
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
    totalAmountForward.srcIncrement +=
      src.allocateStateMachineToFirstValue<NORMAL_PAGE_SZ>(srcFd,
        PROT_READ, false, limits.srcIncrement,
        currIndexInFiles.srcIncrement + totalAmountForward.srcIncrement,
        [](char c){ return  c == '\n' ||  c == '\0' || isspace(c);} );
    dst.allocateStateMachine<NORMAL_PAGE_SZ>(dstFd, PROT_WRITE, true, limits.dstIncrement, currIndexInFiles.dstIncrement + totalAmountForward.dstIncrement);
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

int el2belConvert(int inputFileFd, const char* outputFileStub) {
  struct stat fileinfo;
  int rc = fstat(inputFileFd, &fileinfo);
  if(rc) throw "Failed to get fileinfo";

  #pragma omp parallel
  {
    uint64_t tid;
    uint64_t threads;
    tid = omp_get_thread_num();
    threads = omp_get_num_threads();
    // 20 for uint64_t max  and then 2 for '\0' and '-'
    uint64_t fileNameSize = sizeof(char) * (strlen(outputFileStub) + 22);
    char* outputFileName = (char *) malloc(fileNameSize);
    snprintf(outputFileName, fileNameSize - 1, "%s-%lx", outputFileStub, tid);
    outputFileName[fileNameSize - 1] = '\0';
    int outputFileFd = open(outputFileName, O_RDWR | O_LARGEFILE | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if(outputFileFd < 0) {
      throw "Failed to open Output file.";
    }

    const uint64_t fileSize  = fileinfo.st_size;
    const uint64_t chunkSize = fileSize/threads;
    uint64_t startBound;

    char c;
    if(tid == 0) {
      startBound = 0;
    } else {
      startBound = tid * (chunkSize);
      for(; pread(inputFileFd, &c, 1, startBound) && c != '\n'; startBound--);
    }
    uint64_t endBound;
    if(tid == threads - 1) {
      endBound = fileSize;
    } else {
      endBound = (tid + 1) * (chunkSize);
      for(; pread(inputFileFd, &c, 1, endBound) && c != '\n'; endBound--);
    }
    el2belMapFromFile(inputFileFd, outputFileFd, {startBound, 0}, {endBound, UINT64_MAX});
    close(outputFileFd);
  }

  return 0;
}
