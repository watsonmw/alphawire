#pragma once

#include "mlib/mlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    MStrView scheme;
    MStrView host;
    int port;
    MStrView path;
} HttpUrl;

typedef struct HttpHeader {
    MStrView key;
    MStrView value;
} HttpHeader;


typedef enum HttpVersion {
    HttpVersion_1_0,
    HttpVersion_1_1,
} HttpVersion;

typedef struct {
    int statusCode;
    HttpHeader* headers;
    MStrView body;

    HttpVersion version;
    MStrView statusText;
    MStrView headersText;

    MMemIO response;
} HttpResponse;

b32 Http_ParseUrl(MAllocator* allocator, MStrView url, HttpUrl* outUrl);
void Http_FreeUrl(MAllocator* allocator, HttpUrl* url);

b32 Http_Get(MAllocator* allocator, MStrView url, HttpResponse* outResponse);
void Http_FreeResponse(MAllocator* allocator, HttpResponse* response);

#ifdef __cplusplus
}
#endif
