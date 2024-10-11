#include <getopt.h>
#include <iostream>
#include <fcntl.h>

#include "cli.hpp"
#include "el2bel.hpp"
#include "mmap_helper.hpp"

void help() {
  std::cerr
    << "graph-convert [-i <input-file>] -o <output-file-stub> -t el2bel\n"
    << "\t-i <input-file>: This is the input file of the type specified\n"
    << "\t-o <output-file-stub>: This is the output file stub of the type specified. It should not include the file extension\n"
    << "\t-t <conversion-type>: This specifies what conversion should take place. Multiple specfied construct a pipeline.\n"
    << std::flush;
}

int main(int argc, char** argv) {
  char* inputFileName = nullptr;
  char* outputFileStub = nullptr;
  ConversionType convert = unspecified;

  // Process Input
  int c;
  opterr = 0;
  while ((c = getopt(argc, argv, "i:o:t:")) != -1) {
    switch(c) {
      case 'i':
        inputFileName = optarg;
        break;
      case 'o':
        outputFileStub = optarg;
        break;
      case 't':
        convert = convertFromInput(optarg);
        break;
      default:
        help();
        abort();
    }
  }

  // Input Validation
  int inputFileFd = -1;
  if(inputFileName == nullptr || (inputFileFd = open(inputFileName, O_RDONLY |O_LARGEFILE, 0)) < 0) {
    std::cerr << "input-file was not specified correctly" << std::endl;
    help();
    std::exit(-1);
  }
  if(outputFileStub == nullptr) {
    std::cerr << "output-file-stub was not specified correctly" << std::endl;
    help();
    std::exit(-2);
  }
  if(convert == unspecified) {
    std::cerr << "conversion-type was not specified" << std::endl;
    help();
    std::exit(-3);
  }


  switch(convert) {
    case el2bel:
      return el2belConvert(inputFileFd, outputFileStub);
      break;
    case bel2gr:
    default:
      std::cerr << "Should not get here" << std::endl;
      help();
      std::exit(-4);
  }
}
