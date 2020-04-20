#include "cstdio.h"

using namespace shim;

bionic::FILE bionic::standard_files[3];
int bionic::io_isthreaded = 1;

void bionic::init_standard_files() {
    standard_files[0]._p = "stdin";
    standard_files[0].wrapped = stdin;
    standard_files[1]._p = "stdout";
    standard_files[1].wrapped = stdout;
    standard_files[2]._p = "stderr";
    standard_files[2].wrapped = stderr;
}

static bionic::FILE *wrap_file(::FILE *file) {
    if (!file)
        return nullptr;

    auto ret = new bionic::FILE;
    ret->wrapped = file;
    ret->_p = "Internal";
    ret->_r = ret->_w = 0;
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

bionic::FILE* shim::freopen(const char *filename, const char *mode, bionic::FILE *stream) {
    return wrap_file(::freopen(filename, mode, stream->wrapped));
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
        {"__sF", (void*) &bionic::standard_files},
        {"__isthreaded", (void*) &bionic::io_isthreaded},

        {"fopen", fopen},
        {"fdopen", fdopen},
        {"freopen", freopen},
        {"fclose", fclose},
        {"clearerr", AutoArgRewritten(clearerr)},
        {"feof", AutoArgRewritten(feof)},
        {"ferror", AutoArgRewritten(ferror)},
        {"ferror", AutoArgRewritten(ferror)},
        {"fflush", AutoArgRewritten(fflush)},
        {"fgetc", AutoArgRewritten(fgetc)},
        {"fgets", AutoArgRewritten(fgets)},
        {"fputc", AutoArgRewritten(fputc)},
        {"fputs", AutoArgRewritten(fputs)},
        {"fread", AutoArgRewritten(fread)},
        {"fwrite", AutoArgRewritten(fwrite)},
        {"getc", AutoArgRewritten(getc)},
        {"getdelim", AutoArgRewritten(getdelim)},
        {"getline", AutoArgRewritten(getline)},
        {"putc", AutoArgRewritten(putc)},
        {"rewind", AutoArgRewritten(rewind)},
        {"setbuf", AutoArgRewritten(setbuf)},
        {"setvbuf", AutoArgRewritten(setvbuf)},
        {"setbuffer", AutoArgRewritten(setbuffer)},
        {"setlinebuf", AutoArgRewritten(setlinebuf)},
        {"ungetc", AutoArgRewritten(ungetc)},
        {"fileno", AutoArgRewritten(fileno)},
    });
}