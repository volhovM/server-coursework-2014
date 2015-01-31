#include "logger.h"
#include <string>
#include <iostream>

void mlog(std::string pr, std::string msg)
{
    std::cout << pr << ": " << msg << std::endl;
}

void vm::log_m(std::string msg) {
    //    mlog("MM", msg);
}
void vm::log_d(std::string msg) {
    //    mlog("DB", msg);
}
void vm::log_w(std::string msg) {
    mlog("WW", msg);
}
void vm::log_e(std::string msg) {
    mlog("EE", msg);
}
void vm::log_ex(std::string msg) {
    //    mlog("EXTRA", msg);
}
