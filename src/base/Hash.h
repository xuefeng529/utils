#ifndef BASE_HASH_H
#define BASE_HASH_H

#include <stdint.h>
#include <stddef.h>

namespace base
{

uint32_t hash(const void *key, size_t length);

} // namespace base

#endif // BASE_HASH_H
