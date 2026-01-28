#pragma once

#include "mlib/mlib.h"

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

// Given current byte, return the length of this UTF-8 codepoint
MINLINE i32 UTF8_SequenceLength(u8 firstByte) {
    if ((firstByte & 0x80) == 0) {
        return 1; // 0xxxxxxx
    } else if ((firstByte & 0xE0) == 0xC0) {
        return 2; // 110xxxxx
    } else if ((firstByte & 0xF0) == 0xE0) {
        return 3; // 1110xxxx
    } else if ((firstByte & 0xF8) == 0xF0) {
        return 4; // 11110xxx
    } else {
        return 1; // Invalid UTF-8, treat as single byte
    }
}

// Decode a UTF-8 sequence into a Unicode codepoint
u32 UTF8_Decode(const u8* str, size_t* len);

// Decodes next UTF-8 codepoint and advances pointer.
// Returns 0 on end of string or error.
MINLINE u32 UTF8_Next(const char** str) {
    if (!str || !*str || !**str) return 0;
    size_t len;
    u32 cp = UTF8_Decode((const u8*)*str, &len);
    *str += len;
    return cp;
}

#ifdef __cplusplus
} // extern "C"
#endif
