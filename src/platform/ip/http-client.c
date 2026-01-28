#include "http-client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../mlib/msock.h"

b32 Http_ParseUrl(MAllocator* allocator, MStrView url, HttpUrl* outUrl) {
    memset(outUrl, 0, sizeof(HttpUrl));

    MStrView parse = url;

    // Scheme
    i32 schemeEnd = MStrViewFindC(parse, "://");
    if (schemeEnd) {
        outUrl->scheme = MStrViewLeft(parse, schemeEnd);
        MStrViewAdvance(&parse, schemeEnd + 3);
    } else {
        outUrl->scheme = MStrViewMakeP("http", 4);
    }

    // Host and Port
    i32 pathStart = MStrViewFindChar(parse, '/');
    i32 hostEnd = pathStart >= 0 ? pathStart : (i32)parse.size;
    i32 portStart = MStrViewFindChar(parse, ':');

    if (portStart >= 0 && portStart < hostEnd) {
        outUrl->host = MStrViewLeft(parse, portStart);
        MParseResult r = MParseI32(parse.str + portStart + 1, parse.str + hostEnd, &outUrl->port);
        if (r.end) {
            MStrViewAdvance(&parse, (i32)(r.end - parse.str));
        }
    } else {
        outUrl->host = MStrViewLeft(parse, hostEnd);
        if (MStrViewCmpC(outUrl->scheme, "https") == 0) {
            outUrl->port = 443;
        } else {
            outUrl->port = 80;
        }
    }

    // Path
    if (pathStart) {
        i32 newline = -1;
        for (i32 i = 0; i < parse.size; i++) {
            if (MCharIsNewLine(parse.str[i])) {
                newline = i;
            }
        }

        if (newline >= 0) {
            outUrl->host = MStrViewLeft(parse, newline);
        } else {
            outUrl->path = parse;
        }
    } else {
        outUrl->path = MStrViewMakeP("/", 1);
    }

    return TRUE;
}

void Http_FreeUrl(MAllocator* allocator, HttpUrl* url) {
}

void Http_FreeResponse(MAllocator* allocator, HttpResponse* response) {
    MMemFree(&response->response);
}

static b32 Http_ParseStatusLine(HttpResponse* response) {
    MMemIO r = response->response;
    MStrView v = {(char*)r.mem, r.size};
    const char* versionStrStart = "HTTP/";
    
    enum {
        State_VersionStart,
        State_Version1,
        State_Version2,
        State_Version3,
        State_VersionEnd,
        State_StatusCode,
        State_StatusCodeSpace,
        State_StatusText,
        State_End
    } state = State_VersionStart;
    
    char* secondSpace = NULL;
    char* lineEnd = NULL;
    i32 statusCode = 0;
    HttpVersion version = HttpVersion_1_0;

    for (i32 i = 0; i < v.size; i++) {
        const char c = v.str[i];
        
        if (c == '\r') {
            if (i + 1 < v.size && v.str[i+1] == '\n') {
                lineEnd = v.str + i;
                state = State_End;
                if (secondSpace >= 0) {
                    response->statusText = MStrViewMakeP(secondSpace + 1, (i32)(lineEnd - (secondSpace + 1)));
                } else {
                    return FALSE;
                }
                break;
            }
        }

        if (state == State_VersionStart) {
            if (c != versionStrStart[i]) {
                return FALSE;
            } else if (i == 4) {
                state = State_Version1;
            }
        } else if (state == State_Version1) {
            if (c != '1') {
                return FALSE;
            }
            state = State_Version2;
        } else if (state == State_Version2) {
            if (c != '.') {
                return FALSE;
            }
            state = State_Version3;
        } else if (state == State_Version3) {
            if (c == '0') {
                version = HttpVersion_1_0;
            } else if (c == '1') {
                version = HttpVersion_1_1;
            }
            state = State_VersionEnd;
        } else if (state == State_VersionEnd) {
            if (c == ' ') {
                state = State_StatusCode;
            } else {
                return FALSE;
            }
        } else if (state == State_StatusCode) {
            const char* numStart = v.str + i;
            const char* strEnd = v.str + v.size;
            MParseResult pr = MParseI32(numStart, strEnd, &statusCode);
            if (pr.result != MParse_SUCCESS) {
                return FALSE;
            }
            // skip over status code
            i32 len = (i32)(pr.end - numStart);
            i += (len - 1);
            state = State_StatusCodeSpace;
        } else if (state == State_StatusCodeSpace) {
            if (c == ' ') {
                secondSpace = v.str + i;
                state = State_StatusText;
            } else {
                return FALSE;
            }
        }
    }

    response->version = version;
    response->statusCode = statusCode;

    return TRUE;
}

static b32 Http_ParseResponseHeaders(HttpResponse* response) {
    if (!Http_ParseStatusLine(response)) {
        return FALSE;
    }

    MMemIO r = response->response;
    MStrView responseView = {(char*)r.mem, r.size};
    MStrViewAdvance(&responseView, response->statusText.size + 2);
    
    MStrView headerEndStr = {"\r\n\r\n", 4};
    i32 headersEnd = MStrViewFind(responseView, headerEndStr);
    if (headersEnd < 0) {
        return FALSE;
    }

    response->headersText.str = (char*)response->response.mem;
    response->headersText.size = headersEnd;

    i32 bodyStart = headersEnd + 4;
    if (response->response.size < bodyStart) {
        return FALSE;
    }

    response->body.str = (char*)response->response.mem + bodyStart;
    response->body.size = response->response.size - bodyStart;

    return TRUE;
}

b32 Http_Get(MAllocator* allocator, MStrView url, HttpResponse* outResponse) {
    memset(outResponse, 0, sizeof(HttpResponse));
    MMemInitEmpty(&outResponse->response, allocator);

    HttpUrl parsedUrl;
    if (!Http_ParseUrl(allocator, url, &parsedUrl)) {
        return FALSE;
    }

    MSock sock = MSockMakeTcpSocket();
    if (sock == MSOCK_INVALID) {
        Http_FreeUrl(allocator, &parsedUrl);
        return FALSE;
    }

    MSockSetSocketTimeout(sock, 5000);
    if (MSockConnectHost(sock, parsedUrl.host, (u16)parsedUrl.port, NULL) == MSOCK_ERROR) {
        MSockClose(sock);
        Http_FreeUrl(allocator, &parsedUrl);
        return FALSE;
    }

    MMemIO memIo;
    MMemInitEmpty(&memIo, allocator);
    MStrAppendf(&memIo,
        "GET %.*s HTTP/1.1\r\n"
        "Host: %.*s\r\n"
        "Connection: close\r\n"
        "\r\n",
        parsedUrl.path.size, parsedUrl.path.str,
        parsedUrl.host.size, parsedUrl.host.str);

    MSockSendAll(sock, &memIo);
    MMemFree(&memIo);

    MSockRecvAll(sock, &outResponse->response);

    MSockClose(sock);
    Http_FreeUrl(allocator, &parsedUrl);

    if (outResponse->response.size == 0) {
        MMemFree(&outResponse->response);
        return FALSE;
    }

    if (!Http_ParseResponseHeaders(outResponse)) {
        MMemFree(&outResponse->response);
        return FALSE;
    }

    return TRUE;
}
