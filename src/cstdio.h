#pragma once

#include <libc_shim.h>
#include <cstdint>
#include <stdio.h>
#include "argrewrite.h"

namespace shim {

    namespace bionic {

        struct FILE {
            const char* _p;
            int	_r, _w;
#if defined(__LP64__)
            int	_flags, _file;
#else
            short _flags, _file;
#endif
            ::FILE *wrapped;

#if defined(__LP64__)
            // ?
#else
            char filler[0x54 - 0x14];
#endif
        };

#if defined(__LP64__)
        static_assert(sizeof(FILE) == 0, "FILE must be ? bytes big");
#else
        static_assert(sizeof(FILE) == 0x54, "FILE must be 0x54 bytes big");
#endif

        template <>
        inline auto to_host<FILE>(FILE const *m) { return m->wrapped; }

        extern bionic::FILE standard_files[3];
        extern int io_isthreaded;

        void init_standard_files();

    }

    bionic::FILE *fopen(const char *filename, const char *mode);

    bionic::FILE *fdopen(int fd, const char *mode);

    bionic::FILE *freopen(const char *filename, const char *mode, bionic::FILE *stream);

    bionic::FILE *tmpfile();

    bionic::FILE *popen(const char *command, const char *mode);

    int fclose(bionic::FILE *file);

    int pclose(bionic::FILE *file);

    int fprintf(bionic::FILE* fp, const char *fmt, ...);

    int fscanf(bionic::FILE* fp, const char *fmt, ...);

    void add_cstdio_shimmed_symbols(std::vector<shimmed_symbol> &list);

    namespace detail {

        template <>
        struct arg_rewrite<::FILE *> : bionic_ptr_rewriter<typename ::FILE *, bionic::FILE *> {};

    }
}