#pragma once

#include "mlib/mlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// parsed HTTP URL.
typedef struct {
    MStrView scheme;  // "http" or "https"
    MStrView host;    // "example.com"
    int port;         // 80, 8080, or 443
    MStrView path;    // "/index.html"
} HttpUrl;

typedef struct HttpHeader {
    MStrView key;     // "Content-Type"
    MStrView value;   // "text/plain"
} HttpHeader;

typedef enum HttpVersion {
    HttpVersion_1_0,
    HttpVersion_1_1,
} HttpVersion;

typedef MArray(HttpHeader) HttpHeaders;

// The `headers`, `body`, and other `MStrView` fields point into the `response` memory.
// Call `Http_FreeResponse` to release all associated memory.
typedef struct {
    int statusCode;          // HTTP status code (e.g., 200)
    HttpHeaders headers;
    MStrView body;           // Response body

    HttpVersion version;     // HTTP version
    MStrView statusText;     // Status text (e.g., "OK")
    MStrView headersText;    // Raw headers text

    MMemIO response;         // Raw response buffer and allocator
} HttpResponse;

/**
 * Parses an HTTP URL string into an HttpUrl structure.
 *
 * @param allocator Allocator for any needed allocations (currently unused as views point into input).
 * @param url The URL string to parse.
 * @param outUrl Pointer to the structure to populate.
 * @return TRUE if parsing succeeded, FALSE otherwise.
 */
b32 Http_ParseUrl(MAllocator* allocator, MStrView url, HttpUrl* outUrl);

/**
 * Frees resources associated with an HttpUrl.
 *
 * @param allocator Allocator used during parsing.
 * @param url Pointer to the structure to free.
 */
void Http_FreeUrl(MAllocator* allocator, HttpUrl* url);

/**
 * Performs an HTTP GET request.
 *
 * This implementation currently does not support HTTPS (TLS).
 *
 * @param allocator Allocator for the response buffer and headers.
 * @param url The URL to request.
 * @param outResponse Pointer to the response structure to populate.
 * @return TRUE if the request succeeded and response was parsed, FALSE otherwise.
 */
b32 Http_Get(MAllocator* allocator, MStrView url, HttpResponse* outResponse);

/**
 * Frees all resources associated with an HttpResponse.
 *
 * @param allocator Allocator used during the request.
 * @param response Pointer to the response structure to free.
 */
void Http_FreeResponse(MAllocator* allocator, HttpResponse* response);

#ifdef __cplusplus
}
#endif
