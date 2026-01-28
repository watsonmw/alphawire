#include "mlib/utf8.h"

size_t UTF8_GetConvertUTF16Len(const u16* utf16, size_t utf16Len) {
    size_t utf8Len = 0;
    u32 codePoint;

    for (size_t i = 0; i < utf16Len; i++) {
        // Handle surrogate pairs
        if (utf16[i] >= 0xD800 && utf16[i] <= 0xDBFF) {
            if (i + 1 >= utf16Len) {
                return 0; // Incomplete surrogate pair
            }
            if (utf16[i + 1] < 0xDC00 || utf16[i + 1] > 0xDFFF) {
                return 0; // Invalid low surrogate
            }

            codePoint = 0x10000;
            codePoint += (utf16[i] - 0xD800) << 10;
            codePoint += (utf16[i + 1] - 0xDC00);
            i++; // Skip low surrogate
        } else {
            if (utf16[i] >= 0xDC00 && utf16[i] <= 0xDFFF) {
                return 0; // Unexpected low surrogate
            }
            codePoint = utf16[i];
        }

        // Calculate UTF-8 bytes needed
        if (codePoint < 0x80) {
            utf8Len += 1;
        } else if (codePoint < 0x800) {
            utf8Len += 2;
        } else if (codePoint < 0x10000) {
            utf8Len += 3;
        } else {
            utf8Len += 4;
        }
    }

    return utf8Len;
}

size_t UTF8_ConvertFromUTF16(const u16* utf16In, size_t inLen, char* utf8Out, size_t outLen) {
    size_t outPos = 0;
    u32 codePoint;

    for (size_t i = 0; i < inLen && outPos < outLen; i++) {
        // Handle surrogate pairs
        if (utf16In[i] >= 0xD800 && utf16In[i] <= 0xDBFF) {
            if (i + 1 >= inLen) {
                return 0; // Incomplete surrogate pair
            }

            if (utf16In[i + 1] < 0xDC00 || utf16In[i + 1] > 0xDFFF) {
                return 0; // Invalid low surrogate
            }

            codePoint = 0x10000;
            codePoint += (utf16In[i] - 0xD800) << 10;
            codePoint += (utf16In[i + 1] - 0xDC00);
            i++; // Skip low surrogate
        } else {
            if (utf16In[i] >= 0xDC00 && utf16In[i] <= 0xDFFF) {
                return 0; // Unexpected low surrogate
            }

            codePoint = utf16In[i];
        }

        // Encode to UTF-8
        if (codePoint < 0x80) {
            if (outPos + 1 > outLen) {
                return 0;
            }

            utf8Out[outPos++] = (char)codePoint;
        } else if (codePoint < 0x800) {
            if (outPos + 2 > outLen) {
                return 0;
            }

            utf8Out[outPos++] = (char)((codePoint >> 6) | 0xC0);
            utf8Out[outPos++] = (char)((codePoint & 0x3F) | 0x80);
        } else if (codePoint < 0x10000) {
            if (outPos + 3 > outLen) {
                return 0;
            }

            utf8Out[outPos++] = (char)((codePoint >> 12) | 0xE0);
            utf8Out[outPos++] = (char)(((codePoint >> 6) & 0x3F) | 0x80);
            utf8Out[outPos++] = (char)((codePoint & 0x3F) | 0x80);
        } else {
            if (outPos + 4 > outLen) {
                return 0;
            }

            utf8Out[outPos++] = (char)((codePoint >> 18) | 0xF0);
            utf8Out[outPos++] = (char)(((codePoint >> 12) & 0x3F) | 0x80);
            utf8Out[outPos++] = (char)(((codePoint >> 6) & 0x3F) | 0x80);
            utf8Out[outPos++] = (char)((codePoint & 0x3F) | 0x80);
        }
    }

    return outPos;
}

u32 UTF8_Decode(const u8* str, size_t* len) {
    u32 codepoint = 0;
    *len = UTF8_SequenceLength(str[0]);

    switch (*len) {
        case 1:
            return str[0];
        case 2:
            codepoint = ((str[0] & 0x1F) << 6) | (str[1] & 0x3F);
            break;
        case 3:
            codepoint = ((str[0] & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
            break;
        case 4:
            codepoint = ((str[0] & 0x07) << 18) | ((str[1] & 0x3F) << 12) |
                        ((str[2] & 0x3F) << 6) | (str[3] & 0x3F);
            break;
    }
    return codepoint;
}

int UTF8_StrCmp(const char* str1, const char* str2) {
    const u8* s1 = (const u8*)str1;
    const u8* s2 = (const u8*)str2;

    while (*s1 && *s2) {
        size_t len1, len2;
        u32 c1 = UTF8_Decode(s1, &len1);
        u32 c2 = UTF8_Decode(s2, &len2);

        if (c1 != c2) {
            return (c1 < c2) ? -1 : 1;
        }

        s1 += len1;
        s2 += len2;
    }

    if (*s1) {
        return 1;
    } else if (*s2) {
        return -1;
    } else {
        return 0;
    }
}

int UFT8_StrCaseCmp(const char* str1, const char* str2) {
    const u8* s1 = (const u8*)str1;
    const u8* s2 = (const u8*)str2;

    while (*s1 && *s2) {
        size_t len1, len2;
        u32 c1 = UTF8_Decode(s1, &len1);
        u32 c2 = UTF8_Decode(s2, &len2);

        // Simple case folding for ASCII and Latin-1
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;

        if (c1 != c2) {
            return (c1 < c2) ? -1 : 1;
        }

        s1 += len1;
        s2 += len2;
    }

    if (*s1) {
        return 1;
    } else if (*s2) {
        return -1;
    } else {
        return 0;
    }
}
