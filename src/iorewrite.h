#pragma once
#include <libc_shim.h>
#include <cstring>

static std::string iorewrite0(const char * path) {
    auto len = path ? strlen(path) : 0;
    if(len > 0) {
        for(auto&& from : shim::from_android_data_dir) {
            if (len >= from.size() && !memcmp(path, from.data(), from.size()) && shim::to_android_data_dir.rfind(from.data(), 0)) {
                return shim::to_android_data_dir + std::string(path + from.length());
            }
        }
    }
    return path;
}

template<class T> struct iorewrite1;
template<class R, class ... arg > struct iorewrite1<R (*) (const char * ,arg...)> {
    template<R(*org)(const char *, arg...)> static R rewrite(const char *path1, arg...a) {
        auto len = path1 ? strlen(path1) : 0;
        if(len > 0) {
            for(auto&& from : shim::from_android_data_dir) {
                if (len >= from.size() && !memcmp(path1, from.data(), from.size()) && shim::to_android_data_dir.rfind(from.data(), 0)) {
                    return org((shim::to_android_data_dir + std::string(path1 + from.length())).data(), a...);
                }
            }
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
        auto len1 = path1 ? strlen(path1) : 0;
        auto len2 = path2 ? strlen(path2) : 0;
        if(len1 > 0 || len2 > 0) {
            for(auto&& from : shim::from_android_data_dir) {
                if (len1 >= from.length() && !memcmp(path1, from.data(), from.length()) && shim::to_android_data_dir.rfind(from.data(), 0)) {
                    return rewrite<org>((shim::to_android_data_dir + std::string(path1 + from.length())).data(), path2, a...);
                }
                if (len2 >= from.length() && !memcmp(path2, from.data(), from.length()) && shim::to_android_data_dir.rfind(from.data(), 0)) {
                    return org(path1, (shim::to_android_data_dir + std::string(path2 + from.length())).data(), a...);
                }
            }
        }
        return org(path1, path2, a...);
    }
};

template<class R, class ... arg > struct iorewrite2<R (*)(const char *,const char *,arg...) noexcept> : iorewrite2<R (*)(const char *,const char *,arg...)> {};
template<class R, class ... arg > struct iorewrite2<R(const char *,const char *,arg...)> : iorewrite2<R (*)(const char *,const char *,arg...)> {};
template<class R, class ... arg > struct iorewrite2<R(const char *,const char *,arg...) noexcept> : iorewrite2<R (*)(const char *,const char *,arg...)> {};

#define IOREWRITE1(func) (iorewrite1<decltype(func)>::rewrite<func>)
#define IOREWRITE2(func) (iorewrite2<decltype(func)>::rewrite<func>)
