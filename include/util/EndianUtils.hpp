#ifndef ENDIAN_UTILS_HPP
#define ENDIAN_UTILS_HPP

#include <cstdint>
#include <arpa/inet.h>

uint64_t htonll(uint64_t value);
uint64_t ntohll(uint64_t value);

#endif
