#include <getopt.h>
#include <iostream>
#include <optional>
#include <fcntl.h>

void help() {
  std::cerr
    << "graph-convert [-i <input-file>] -o <output-file-stub> -t el2bel\n"
    << "\t-i <input-file>: This is the input file of the type specified\n"
    << "\t-o <output-file-stub>: This is the output file stub of the type specified. It should not include the file extension\n"
    << "\t-t <conversion-type>: This specifies what conversion should take place. Multiple specfied construct a pipeline.\n"
    << std::flush;
}

// All of these must be 8bytes or less;
enum ConversionType {
  unspecified,
  el2bel,
  bel2gr,
};

std::optional<uint64_t> convert8byteStringHash(char* string) {
  uint64_t hash = 0;
  uint8_t i = 0;
  for(; string[i] != '\0' && i < 8; i++) {
    hash |= ((uint64_t) string[i]) << (i * 8);
  }
  if(string[i] != '\0') {
    return std::nullopt;
  }
  return hash;
}

consteval uint64_t unsafeHashConvert(const char* string) {
  uint64_t hash = 0;
  uint8_t i = 0;
  for(; string[i] != '\0' && i < 8; i++) {
    hash |= (uint64_t) string[i] << i;
  }
  return hash;
}

ConversionType convertFromInput(char* string) {
  std::optional<uint64_t> maybeHash = convert8byteStringHash(string);
  if(maybeHash == std::nullopt)
    return unspecified;

  auto hashed = *maybeHash;
  switch(hashed) {
    case unsafeHashConvert("el2bel"):
      return el2bel;
    case unsafeHashConvert("bel2gr"):
      return bel2gr;
    default:
      return unspecified;
  }
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
  int inputFileFd;
  if(inputFileName == nullptr && (inputFileFd = open(inputFileName, O_RDONLY |O_LARGEFILE, 0)) < 0) {
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
      //return el2bel(inputFileFd, outputFileStub);
      break;
    default:
      std::cerr << "Should not get here" << std::endl;
      help();
      std::exit(-4);
  }
}
