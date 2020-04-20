#pragma once

#include <libc_shim.h>
#include <cstdint>
#include <stdio.h>

namespace shim {

    namespace bionic {

        struct FILE {
            unsigned char* _p;
            int	_r, _w;
#if defined(__LP64__)
            int	_flags, _file;
#else
            short _flags, _file;
#endif
            ::FILE *wrapped;
        };

        extern bionic::FILE standard_files[3];

        void init_standard_files();

    }

    bionic::FILE *fopen(const char *filename, const char *mode);

    bionic::FILE *fdopen(int fd, const char *mode);

    void fclose(bionic::FILE *file);

    void add_cstdio_shimmed_symbols(std::vector<shimmed_symbol> &list);

}