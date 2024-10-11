#ifndef _EL2BEL_HPP_
#define _EL2BEL_HPP_
#include "srcdst.hpp"

int el2belConvert(int inputFileFd, const char* outputFileStub);

void el2belMapFromFile(int srcFd, int dstFd, SrcDstIncrement currIndexInFiles, SrcDstIncrement limits);

#endif // _EL2BEL_HPP_
