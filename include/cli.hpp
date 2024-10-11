#ifndef _CLI_HPP_
#define _CLI_HPP_
#include <optional>
#include <cstdint>

// All of these must be 8bytes or less;
enum ConversionType {
  unspecified,
  el2bel,
  bel2gr,
};

inline std::optional<uint64_t> convert8byteStringHash(char* string) {
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

inline consteval uint64_t unsafeHashConvert(const char* string) {
  uint64_t hash = 0;
  uint8_t i = 0;
  for(; string[i] != '\0' && i < 8; i++) {
    hash |= ((uint64_t) string[i]) << (i * 8);
  }
  return hash;
}

inline ConversionType convertFromInput(char* string) {
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

#endif // _CLI_HPP_
