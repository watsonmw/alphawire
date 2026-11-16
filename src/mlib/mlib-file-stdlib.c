#include "mlib.h"

#include <limits.h>
#include <stdio.h>

MReadFileResult MFileReadWithOffset(MAllocator* allocator, const char* filePath, size_t offset, size_t readSize) {
    MReadFileResult ret = {0};

    FILE *file = fopen(filePath, "rb");
    if (file == NULL) {
        return ret;
    }

    size_t size = readSize;
    if (!readSize) {
        if (fseek(file, 0L, SEEK_END) != 0) {
            fclose(file);
            return ret;
        }
        long fileSize = ftell(file);
        if (fileSize < 0 || fseek(file, 0L, SEEK_SET) != 0) {
            fclose(file);
            return ret;
        }
        size = fileSize;
    }

    ret.data = (u8*)MMalloc(allocator, size);
    if (size && !ret.data) {
        fclose(file);
        return ret;
    }

    if (offset) {
        if (fseek(file, (long)offset, SEEK_SET) != 0) {
            MFree(allocator, ret.data, size);
            fclose(file);
            return (MReadFileResult){0};
        }
    }

    size_t bytesRead = fread(ret.data, 1, size, file);
    ret.size = bytesRead;

    fclose(file);

    return ret;
}

MFile MFileWriteOpen(const char* filePath) {
    MFile fileData = {0};

    FILE *file = fopen(filePath, "wb");
    if (file == NULL) {
        fileData.open = 0;
        return fileData;
    } else {
        fileData.handle = file;
        fileData.open = 1;
    }

    return fileData;
}

MWriteFileResult MFileWriteData(MFile* file, u8* data, size_t size) {
    if (!file->open) {
        return M_STRUCT(MWriteFileResult){-1, 0};
    }

    size_t bytesWritten = fwrite(data, 1, size, (FILE*)file->handle);
    return M_STRUCT(MWriteFileResult){0, bytesWritten};
}

void MFileClose(MFile* file) {
    if (file->open) {
        fclose((FILE*)file->handle);
        file->open = 0;
    }
}
