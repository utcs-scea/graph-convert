#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <sys/stat.h>
#include <omp.h>

#include "mmap_helper.hpp"

constexpr uint64_t TEST_PAGE_SZ = 1ull << 12;

#if !defined __has_feature
#define __has_feature(ans) 0
#endif

#define ASAN_ENABLED (__has_feature(address_sanitizer) || __SANITIZE_ADDRESS__)

struct OperatorTestCaseHelper {
  SrcDstIncrement left;
  SrcDstIncrement right;
  bool ans;
  constexpr OperatorTestCaseHelper(uint64_t a, uint64_t b, uint64_t c, uint64_t d, bool ans):
    left(a, b), right(c, d), ans(ans) {}
  constexpr ~OperatorTestCaseHelper() {}
};

TEST(SrcDstIncrement, Equals) {
  constexpr OperatorTestCaseHelper testCases[] = {
    {10ull, 10ull, 10ull, 10ull, true},
    {10ull, 10ull, 10ull, 10ull, true},
    {10ull, 10ull, 11ull, 11ull, false},
    {10ull, 10ull, 10ull, 11ull, false},
    {10ull, 10ull, 11ull, 10ull, false},
    {11ull, 11ull, 10ull, 10ull, false},
    {11ull, 10ull, 10ull, 10ull, false},
    {10ull, 11ull, 10ull, 10ull, false},
  };

  for(const auto& test : testCases){
    EXPECT_EQ(test.left == test.right, test.ans) << "Left: " << test.left.srcIncrement << ", "
      << test.left.dstIncrement << "; "
      "Right: " << test.right.srcIncrement << ", " << test.right.dstIncrement;
  }
}

TEST(SrcDstIncrement, Less) {
  constexpr OperatorTestCaseHelper testCases[] = {
    {10ull, 10ull, 10ull, 10ull, false},
    {10ull, 10ull, 10ull, 10ull, false},
    {10ull, 10ull, 11ull, 11ull, true},
    {10ull, 10ull, 10ull, 11ull, false},
    {10ull, 10ull, 11ull, 10ull, false},
    {11ull, 11ull, 10ull, 10ull, false},
    {11ull, 10ull, 10ull, 10ull, false},
    {10ull, 11ull, 10ull, 10ull, false},
  };

  for(const auto& test : testCases){
    EXPECT_EQ(test.left < test.right, test.ans) << "Left: " << test.left.srcIncrement << ", "
      << test.left.dstIncrement << "; "
      "Right: " << test.right.srcIncrement << ", " << test.right.dstIncrement;
  }
}

TEST(SrcDstIncrement, AddInto) {
  SrcDstIncrement a {1,2};
  SrcDstIncrement b {3,4};
  EXPECT_EQ(a += b, SrcDstIncrement(1 + 3, 2 + 4));
  EXPECT_EQ(a, SrcDstIncrement(1 + 3, 2 + 4));
}

TEST(MMapBuffer, Construction) {
  MMapBuffer buffer;
  EXPECT_EQ(buffer.buf, nullptr);
  EXPECT_EQ(buffer.currIndex, 0);
  EXPECT_EQ(buffer.dataLimit, 0);
}

#if ! ASAN_ENABLED
TEST(MMapBuffer, StateMachine) {

  //First Test Writing
  int fd = open("./", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
  ASSERT_GT(fd, 0);

  MMapBuffer buffer;
  ASSERT_NO_THROW(buffer.allocateStateMachine<TEST_PAGE_SZ>(fd, PROT_WRITE, true, 2ull << 30, 0));
  ASSERT_NE(buffer.buf, nullptr);
  ASSERT_NE(buffer.buf, MAP_FAILED);
  EXPECT_EQ(buffer.currIndex, 0);
  EXPECT_EQ(buffer.dataLimit, TEST_PAGE_SZ *2);
  struct stat fileinfo;
  ASSERT_EQ(fstat(fd, &fileinfo), 0);
  ASSERT_EQ(fileinfo.st_size, TEST_PAGE_SZ *2);

  // Write to the file
  char* buf = (char*)buffer.buf;
  ASSERT_EXIT((buf[0] = 'a', exit(0)), ::testing::ExitedWithCode(0),".*") <<
    "Failure when accessing:" << (void*) buf << ". Prot Allows Writing.";
  ASSERT_EXIT((buf[buffer.dataLimit -1] = 'a', exit(0)), ::testing::ExitedWithCode(0),".*") <<
    "Failure when accessing:" << (void*) buf << ". Prot Allows Writing.";

  memset(buf, 'a', buffer.dataLimit - buffer.currIndex);
  buffer.currIndex += (buffer.dataLimit - buffer.currIndex);
  EXPECT_EQ(buffer.currIndex, TEST_PAGE_SZ *2);

  // Free the writer
  ASSERT_NO_THROW(buffer.freeStateMachine());
  EXPECT_EQ(buffer.buf, nullptr);

  // Allocate the Reader
  ASSERT_NO_THROW(buffer.allocateStateMachine<TEST_PAGE_SZ>(fd, PROT_READ, false, 2ull << 30, 0));
  EXPECT_NE(buffer.buf, nullptr);
  EXPECT_NE(buffer.buf, MAP_FAILED);
  EXPECT_EQ(buffer.currIndex, 0);
  EXPECT_EQ(buffer.dataLimit, TEST_PAGE_SZ * 2);

  // Read from the reader
  buf = (char*) buffer.buf;
  ASSERT_EXIT((buf[0] = 1,exit(0)),
      ::testing::KilledBySignal(SIGSEGV),".*") << "prot values are not passed in correctly.";
  ASSERT_EXIT((exit(buf[0])),
      ::testing::ExitedWithCode((int)'a'),".*") << "The buffer is not readable.";
  ASSERT_EXIT((exit(buf[buffer.dataLimit -1])),
      ::testing::ExitedWithCode((int)'a'),".*") << "The buffer is not readable.";
  for(; buffer.currIndex < buffer.dataLimit; buffer.currIndex++) {
    ASSERT_EQ(buf[buffer.currIndex], 'a');
  }

  // Free the Reader
  ASSERT_NO_THROW(buffer.freeStateMachine());
  EXPECT_EQ(buffer.buf, nullptr);

  close(fd);
}
#else
#endif

class el2belSingleThreadTest : public ::testing::TestWithParam<std::tuple<const char*, uint64_t, std::vector<uint64_t>>> {};

TEST_P(el2belSingleThreadTest, SimpleTest) {

  // Create A file with a few numbers in it.
  int elFd = open("./", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
  ASSERT_GT(elFd, 0);
  const char* input = std::get<0>(GetParam());
  uint64_t length = std::get<1>(GetParam());

  auto size = pwrite(elFd, input, length, 0);
  ASSERT_EQ(size, length);

  // Create a file to write the bel
  int belFd = open("./", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
  ASSERT_GT(belFd, 0);

  // Attempt to write
  el2belMapFromFile(elFd, belFd, {0,0}, {length, UINT64_MAX});

  std::vector<uint64_t> ans = std::get<2>(GetParam());

  //Check File Size
  struct stat fileinfo;
  ASSERT_EQ(fstat(belFd, &fileinfo), 0);
  ASSERT_EQ(fileinfo.st_size, ans.size() * 8);

  // Check content
  uint64_t actual;
  for(uint64_t i = 0; i < ans.size(); i++) {
    size = pread(belFd, &actual, 8, i * 8);
    ASSERT_EQ(size, 8);
    uint64_t expected = ans[i];
    ASSERT_EQ(actual, expected);
  }

  close(elFd);
  close(belFd);
}

INSTANTIATE_TEST_SUITE_P(SimpleValues, el2belSingleThreadTest,
                         ::testing::Values(std::make_tuple("1,1", 3, std::vector<uint64_t>({1, 1})),
                                          std::make_tuple("100\00056", 6, std::vector<uint64_t>({100, 56})),
                                          std::make_tuple("100 56", 6, std::vector<uint64_t>({100, 56}))
                           ));

struct el2belOMPThreadTestArgs {
  uint64_t numThreads;
  const char* contents;
  uint64_t contentLength;
  std::vector<uint64_t> ans;
};

class el2belOMPThreadTest: public ::testing::TestWithParam<el2belOMPThreadTestArgs> {};

TEST_P(el2belOMPThreadTest, SimpleTest) {
  const el2belOMPThreadTestArgs args = GetParam();
  omp_set_dynamic(0);
  omp_set_num_threads(args.numThreads);

  // Create A file with a few numbers in it.
  int elFd = open("./", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
  ASSERT_GT(elFd, 0);
  const char* input = args.contents;
  const uint64_t length = args.contentLength;

  auto size = pwrite(elFd, input, length, 0);
  ASSERT_EQ(size, length);

  // Create a file to write the bel

  // Attempt to write
  const char* outputStub = "./el2belOMPThreadTest";
  ASSERT_EQ(el2belConvert(elFd, outputStub), 0);

  const std::vector<uint64_t> ans = args.ans;

  // Check result
  uint64_t totalSize = 0;
  uint64_t newStringLength = strlen("./el2belOMPThreadTest") + 22;
  char* belFileName = (char*) malloc(sizeof(char) * newStringLength);
  auto vecIter = ans.begin();
  uint64_t actual;
  for(uint64_t i = 0; i < args.numThreads; i++) {

    // Get file
    snprintf(belFileName, newStringLength - 1, "%s-%lu", outputStub, i);
    int belFd = open(belFileName, O_RDONLY, S_IRUSR | S_IWUSR);
    ASSERT_GT(belFd, 0);

    // Stat file
    struct stat fileinfo;
    ASSERT_EQ(fstat(belFd, &fileinfo), 0);
    // Check Sizes
    ASSERT_LT(totalSize, ans.size() * 8);
    totalSize += fileinfo.st_size;

    // Check Contents
    for(uint64_t j = 0; j < fileinfo.st_size; j+=8, vecIter++) {
      size = pread(belFd, &actual, 8, j);
      ASSERT_EQ(actual, *vecIter);
    }
    close(belFd);
    unlink(belFileName);
  }

  // Check Size
  ASSERT_EQ(totalSize, ans.size() * 8);

  free(belFileName);

  close(elFd);
}

INSTANTIATE_TEST_SUITE_P(SimpleValues, el2belOMPThreadTest,
                         ::testing::Values(el2belOMPThreadTestArgs{1, "1,1", 3, {1, 1}},
                                          el2belOMPThreadTestArgs{2, "1,1\n1,1", 7, {1,1,1,1}},
                                          el2belOMPThreadTestArgs{2, "1,1\n1,1\n1,1", 11, {1,1,1,1,1,1}},
                                          el2belOMPThreadTestArgs{2, "1,1\n1,1\n1,1\n1,1", 16, {1,1,1,1,1,1,1,1}},
                                          el2belOMPThreadTestArgs{3, "1,1\n1,1\n1,1", 11, {1,1,1,1,1,1}},
                                          el2belOMPThreadTestArgs{3, "1,2\n3,4\n5,6", 11, {1,2,3,4,5,6}},
                                          el2belOMPThreadTestArgs{3, "1,2", 3, {1,2}},
                                          el2belOMPThreadTestArgs{3, "1,2\n3,4", 7, {1,2,3,4}}
                           ));

