#include <fcntl.h>
#include <gtest/gtest.h>
#include <unistd.h>
#include <sys/stat.h>
#include "mmap_helper.hpp"

constexpr uint64_t TEST_PAGE_SZ = 1ull << 12;

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
    {10ull, 10ull, 10ull, 11ull, true},
    {10ull, 10ull, 11ull, 10ull, true},
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

TEST(MMapBuffer, StateMachine) {

  //First Test Writing
  int fd = open("./", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
  ASSERT_GT(fd, 0);

  MMapBuffer buffer;
  ASSERT_NO_THROW(buffer.allocateStateMachine<TEST_PAGE_SZ>(fd, PROT_WRITE, true, 2ull << 30, 0));
  ASSERT_NE(buffer.buf, nullptr);
  ASSERT_NE(buffer.buf, MAP_FAILED);
  EXPECT_EQ(buffer.currIndex, 0);
  EXPECT_EQ(buffer.dataLimit, TEST_PAGE_SZ);
  struct stat fileinfo;
  ASSERT_EQ(fstat(fd, &fileinfo), 0);
  ASSERT_EQ(fileinfo.st_size, TEST_PAGE_SZ);

  // Write to the file
  char* buf = (char*)buffer.buf;
  for(; buffer.currIndex < buffer.dataLimit; buffer.currIndex++)
    ASSERT_EXIT((buf[buffer.currIndex] = 'a', exit(0)), ::testing::ExitedWithCode(0),".*") <<
      "Failure when accessing:" << (void*) buf << ". Prot Allows Writing.";
  EXPECT_EQ(buffer.currIndex, TEST_PAGE_SZ);

  // Free the writer
  ASSERT_NO_THROW(buffer.freeStateMachine());
  EXPECT_EQ(buffer.buf, nullptr);

  // Allocate the Reader
  ASSERT_NO_THROW(buffer.allocateStateMachine<TEST_PAGE_SZ>(fd, PROT_READ, false, 2ull << 30, 0));
  EXPECT_NE(buffer.buf, nullptr);
  EXPECT_NE(buffer.buf, MAP_FAILED);
  EXPECT_EQ(buffer.currIndex, 0);
  EXPECT_EQ(buffer.dataLimit, TEST_PAGE_SZ);

  // Read from the reader
  buf = (char*) buffer.buf;
  ASSERT_EXIT((buf[0] = 1,exit(0)),
      ::testing::KilledBySignal(SIGSEGV),".*") << "prot values are not passed in correctly.";
  for(; buffer.currIndex < buffer.dataLimit; buffer.currIndex++) {
    ASSERT_EXIT((exit(buf[buffer.currIndex])),
        ::testing::ExitedWithCode((int)'a'),".*") << "The buffer is not readable.";
  }

  // Free the Reader
  ASSERT_NO_THROW(buffer.freeStateMachine());
  EXPECT_EQ(buffer.buf, nullptr);

  close(fd);
}

TEST(el2belMapFromFile, CommaTest) {

  // Create A file with a few numbers in it.
  int elFd = open("./", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
  ASSERT_GT(elFd, 0);

  auto size = pwrite(elFd, "1,1", 3, 0);
  ASSERT_EQ(size, 3);

  // Create a file to write the bel
  int belFd = open("./", O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
  ASSERT_GT(belFd, 0);

  // Attempt to write
  el2belMapFromFile(elFd, belFd, {0,0}, {3, UINT64_MAX});

  //Check File Size
  struct stat fileinfo;
  ASSERT_EQ(fstat(belFd, &fileinfo), 0);
  ASSERT_EQ(fileinfo.st_size, 16);

  // Check content
  MMapBuffer buffer {};
  ASSERT_NO_THROW(buffer.allocateStateMachine<TEST_PAGE_SZ>(belFd, PROT_READ, false, 16, 0));
  EXPECT_NE(buffer.buf, nullptr);
  EXPECT_NE(buffer.buf, MAP_FAILED);
  EXPECT_EQ(buffer.currIndex, 0);
  EXPECT_EQ(buffer.dataLimit, 16);

  close(elFd);
  close(belFd);
}
