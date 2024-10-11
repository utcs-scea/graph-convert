#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <chrono>
#include <fcntl.h>
#include <iostream>

#include "cli.hpp"
#include "VLC/runtime.h"
#include "VLC/loader.h"

void help() {
  std::cerr
    << "vlc-trivial [-i <input-file>] -o <output-file-stub> -t el2bel\n"
    << "\t-i <input-file>: This is the input file of the type specified\n"
    << "\t-o <output-file-stub>: This is the output file stub of the type specified. It should not include the file extension\n"
    << "\t-t <conversion-type>: This specifies what conversion should take place. Multiple specfied construct a pipeline.\n"
    << std::flush;
}

typedef int (*el2belConvert_t)(int inputFileFd, char* outputFileStub);

void register_functions() {
  std::unordered_map<std::string, std::string> names{
    {"el2belConvert", "_Z13el2belConvertiPKc"}};
  VLC::Loader::register_func_names(names);
}

using Clock = std::chrono::steady_clock;
using TimePoint = std::chrono::time_point<Clock>;
using NS = std::chrono::nanoseconds;

int launch_el2bel(int inputFileFd, char* outputFileStub, int vlc_id, TimePoint& beg, TimePoint& end) {
  VLC::Context vlc(vlc_id, gettid());

  // please change the number based on your system
  vlc.avaliable_cpu("0-31");
  VLC::register_vlc(&vlc);

  VLC::Loader loader("libgconvert_shared.so", vlc_id, false);

  auto el2belConvert = loader.load_func<el2belConvert_t>("el2belConvert");
  beg = Clock::now();
  auto ret = el2belConvert(inputFileFd, outputFileStub);
  end = Clock::now();
  return ret;
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

  auto vlc_beg = Clock::now();
  auto prog_beg = vlc_beg;
  auto prog_end = vlc_beg;

  // initialize VLC environment
  VLC::Runtime vlc;
  vlc.initialize();

  register_functions();
  int ret = -1;

  switch(convert) {
    case el2bel:
      ret = launch_el2bel(inputFileFd, outputFileStub, 0, prog_beg, prog_end);
      break;
    case bel2gr:
    default:
      std::cerr << "Should not get here" << std::endl;
      help();
      std::exit(-4);
  }

  auto vlc_end = Clock::now();
  auto vlc_time = std::chrono::duration_cast<NS>(vlc_end - vlc_beg);
  auto prog_time = std::chrono::duration_cast<NS>(prog_end - prog_beg);
  std::cerr << "VLC  TIMES:\t" << vlc_time.count()  << "ns" << std::endl;
  std::cerr << "PROG TIMES:\t" << prog_time.count() << "ns" << std::endl;

  return ret;
}
