#include "cstdio.h"

using namespace shim;

void bionic::init_standard_files() {
    standard_files[0].wrapped = stdin;
    standard_files[1].wrapped = stdout;
    standard_files[2].wrapped = stderr;
}

static bionic::FILE *wrap_file(::FILE *file) {
    if (!file)
        return nullptr;

    auto ret = new bionic::FILE;
    ret->wrapped = file;
#if defined(__LP64__)
    ret->_file = fileno(file);
#else
    ret->_file = (short) fileno(file);
#endif
    ret->_flags = 0;
    return ret;
}

bionic::FILE *shim::fopen(const char *filename, const char *mode) {
    return wrap_file(::fopen(filename, mode));
}

bionic::FILE* shim::fdopen(int fd, const char *mode) {
    return wrap_file(::fdopen(fd, mode));
}

void shim::fclose(bionic::FILE *file) {
    fclose(file->wrapped);
    if (file == &bionic::standard_files[0] ||
        file == &bionic::standard_files[1] ||
        file == &bionic::standard_files[2])
        return;
    delete file;
}

void shim::add_cstdio_shimmed_symbols(std::vector<shim::shimmed_symbol> &list) {
    bionic::init_standard_files();

    list.insert(list.end(), {
        {"fopen", fopen},
        {"fdopen", fdopen},
        {"fclose", fclose},
    });
}