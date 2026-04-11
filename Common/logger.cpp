#include "logger.h"
#include <ctime>
#include <cstring>

// constructor
Logger::Logger(const std::string& filename)
{
    file.open(filename, std::ios::app);
}

// destructor
Logger::~Logger()
{
    if (file.is_open())
        file.close();
}

// helper write
void Logger::write(const std::string& code, const std::string& msg)
{
    if (!file.is_open())
        return;

    std::time_t now = std::time(nullptr);
    char timeStr[26];
    ctime_s(timeStr, sizeof(timeStr), &now);

    timeStr[strlen(timeStr) - 1] = '\0';

    file << "[" << timeStr << "] " << code;

    if (!msg.empty())
        file << " : " << msg;

    file << std::endl;
}

// switch logger
void Logger::Log(LogCode code, const std::string& extra)
{
    switch (code)
    {
        // Client 
    case CS_JPEG:
        write("CS_JPEG", "Client sent JPEG");
        break;

    case CR_JPEG:
        write("CR_JPEG", "Client received JPEG");
        break;

    case CR_PRMT:
        write("CR_PRMT", "Client received prompt");
        break;

        // Server 
    case SS_JPEG:
        write("SS_JPEG", "Server sent JPEG");
        break;

    case SR_JPEG:
        write("SR_JPEG", "Server received JPEG");
        break;

    case SS_PRMT:
        write("SS_PRMT", "Server sent prompt");
        break;

    case SL_LOGIN:
        write("SL_LOGIN", "Client " + extra + " logged in");
        break;

    case SC_XSTATE:
        write("SC_XSTATE", "Changed to state " + extra);
        break;

    default:
        write("UNKNOWN", "Unknown log code");
        break;
    }
}