// Platform compatibility
#ifdef _WIN32
#include <io.h>
#ifndef dup
#define dup _dup
#endif
#ifndef dup2
#define dup2 _dup2
#endif
#ifndef fileno
#define fileno _fileno
#endif
#ifndef close
#define close _close
#endif
#endif  // _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"

extern FILE* redirect_stdout_to_temp(int* old_stdout_fd) {
    FILE* temp = tmpfile();
    if (!temp) {
        perror("tmpfile");
        return NULL;
    }
    fflush(stdout);
    *old_stdout_fd = dup(fileno(stdout));
    if (*old_stdout_fd < 0) {
        perror("dup");
        fclose(temp);
        return NULL;
    }
    if (dup2(fileno(temp), fileno(stdout)) < 0) {
        perror("dup2");
        close(*old_stdout_fd);
        fclose(temp);
        return NULL;
    }
    return temp;
}

extern void restore_stdout(int old_stdout_fd) {
    fflush(stdout);
    dup2(old_stdout_fd, fileno(stdout));
    close(old_stdout_fd);
}

extern void read_temp_file(FILE* temp, char* buffer, size_t size) {
    fflush(temp);
    rewind(temp);
    memset(buffer, 0, size);
    fread(buffer, 1, size - 1, temp);
    buffer[size - 1] = '\0';

    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }

    fclose(temp);
}
