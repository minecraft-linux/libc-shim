#include "iorewrite.h"

std::string shim::to_android_data_dir = "/data/data";

std::string shim::from_android_data_dir = "/data/data";

// int test(const char * path, int n) {
//     return iorewrite<decltype(&test)>::rewrite<&test>("/data/data/Zest", n);
// }

// int test2(const char * path) {
//     return iorewrite<decltype(&test2)>::rewrite<&test2>("/data/data/Zest");
// }