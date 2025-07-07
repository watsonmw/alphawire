#pragma once

#include <../mlib/mlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// Returns required size for UTF-8 output buffer, or 0 on error
size_t UTF8_GetConvertUTF16Len(const u16* utf16, size_t utf16Len);

// Returns number of bytes written to utf8_out, or 0 on error
size_t UTF8_ConvertFromUTF16(const u16* utf16In, size_t inLen, char* utf8Out, size_t outLen);

// Compare two UTF-8 strings
// Returns: -1 if str1 < str2, 0 if str1 == str2, 1 if str1 > str2
int UTF8_StrCmp(const char* str1, const char* str2);

// Case-insensitive UTF-8 string comparison
// Simple case folding for Basic Latin and Latin-1 Supplement
int UFT8_StrCaseCmp(const char* str1, const char* str2);

#ifdef __cplusplus
} // extern "C"
#endif
