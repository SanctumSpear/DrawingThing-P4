#pragma once
#include <string>
#include <fstream>

// log codes as enum (acts like constant ints)
enum LogCode
{
    CS_JPEG,
    CR_JPEG,
    CR_PRMT,

    SS_JPEG,
    SR_JPEG,
    SS_PRMT,

    SL_LOGIN,
    SC_XSTATE
};

class Logger
{
private:
    std::ofstream file;

    void write(const std::string& code, const std::string& msg);

public:
    Logger(const std::string& filename = "log.txt");
    ~Logger();

    // single log function
    void Log(LogCode code, const std::string& extra = "");
};