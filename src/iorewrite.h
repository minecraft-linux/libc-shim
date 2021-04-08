#pragma once
#include <libc_shim.h>
#include <cstring>

static std::string iorewrite0(const char * path) {
    if (path && !strcmp(path, shim::from_android_data_dir.data()) && shim::to_android_data_dir.rfind(shim::from_android_data_dir.data(), 0)) {
        return shim::to_android_data_dir + std::string(path + shim::from_android_data_dir.length());
    }
    return path;
}

template<class T> struct iorewrite1;
template<class R, class ... arg > struct iorewrite1<R (*) (const char * ,arg...)> {
    template<R(*org)(const char *, arg...)> static R rewrite(const char *path1, arg...a) {
        if (path1 && !strcmp(path1, shim::from_android_data_dir.data()) && shim::to_android_data_dir.rfind(shim::from_android_data_dir.data(), 0)) {
            return org((shim::to_android_data_dir + std::string(path1 + shim::from_android_data_dir.length())).data(), a...);
        }
        return org(path1, a...);
    }
};

template<class R, class ... arg > struct iorewrite1<R (*)(const char *,arg...) noexcept> : iorewrite1<R (*)(const char *,arg...)> {};
template<class R, class ... arg > struct iorewrite1<R(const char *,arg...)> : iorewrite1<R (*)(const char *,arg...)> {};
template<class R, class ... arg > struct iorewrite1<R(const char *,arg...) noexcept> : iorewrite1<R (*)(const char *,arg...)> {};

template<class T> struct iorewrite2;
template<class R, class ... arg > struct iorewrite2<R (*)(const char *,const char *,arg...)> {
    template<R(*org)(const char *,const char *,arg...)> static R rewrite(const char *path1, const char *path2, arg...a) {
        if (path1 && !memcmp(path1, shim::from_android_data_dir.data(), shim::from_android_data_dir.length()) && shim::to_android_data_dir.rfind(shim::from_android_data_dir.data(), 0)) {
            return rewrite<org>((shim::to_android_data_dir + std::string(path1 + shim::from_android_data_dir.length())).data(), path2, a...);
        }
        if (path2 && !memcmp(path2, shim::from_android_data_dir.data(), shim::from_android_data_dir.length()) && shim::to_android_data_dir.rfind(shim::from_android_data_dir.data(), 0)) {
            return org(path1, (shim::to_android_data_dir + std::string(path2 + shim::from_android_data_dir.length())).data(), a...);
        }
        return org(path1, path2, a...);
    }
};

template<class R, class ... arg > struct iorewrite2<R (*)(const char *,const char *,arg...) noexcept> : iorewrite2<R (*)(const char *,const char *,arg...)> {};
template<class R, class ... arg > struct iorewrite2<R(const char *,const char *,arg...)> : iorewrite2<R (*)(const char *,const char *,arg...)> {};
template<class R, class ... arg > struct iorewrite2<R(const char *,const char *,arg...) noexcept> : iorewrite2<R (*)(const char *,const char *,arg...)> {};

#define IOREWRITE1(func) (iorewrite1<decltype(func)>::rewrite<func>)
#define IOREWRITE2(func) (iorewrite2<decltype(func)>::rewrite<func>)
