//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

#ifndef BASE_MURMURHASH3_H
#define BASE_MURMURHASH3_H

//-----------------------------------------------------------------------------
// Platform-specific functions and macros
#include <stdint.h>
#include <stddef.h>

//-----------------------------------------------------------------------------
namespace base
{
	
uint32_t murmurHash3(const void* key, size_t length);

} // namespace base

//-----------------------------------------------------------------------------

#endif // BASE_MURMURHASH3_H
