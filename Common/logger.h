#pragma once
#include <string>
#include <fstream>
#include <cstdint>
#include <ctime>
#include <cstring>

enum LogCode {
    // Client events
    CS_JPEG,       // client sent JPEG
    CR_JPEG,       // client received JPEG
    CR_PRMT,       // client received prompt
    CS_SEND,       // client sent data
    CR_RECV,       // client received data

    // Server events
    SS_JPEG,       // server sent JPEG
    SR_JPEG,       // server received JPEG
    SS_PRMT,       // server sent prompt
    SS_SEND,       // server sent data
    SR_RECV,       // server received data

    // Auth / connection
    SL_LOGIN,      // login attempt
    SC_CONNECT,    // client connected
    SC_DISCONNECT, // client disconnected

    // State machine
    SC_XSTATE      // state transition
};

class Logger {
private:
    std::ofstream file;

    void write(const std::string& code,
               uint8_t sessionID,
               bool success,
               const std::string& msg) {
        if (!file.is_open()) return;

        std::time_t now = std::time(nullptr);
        char timeStr[26];
        ctime_s(timeStr, sizeof(timeStr), &now);
        timeStr[strlen(timeStr) - 1] = '\0'; 

        file << "[" << timeStr << "]"
             << " [SID:" << (int)sessionID << "]"
             << " [" << (success ? "OK " : "ERR") << "]"
             << " " << code;

        if (!msg.empty())
            file << " : " << msg;

        file << "\n";
    }

public:
    Logger(const std::string& filename = "log.txt") {
        file.open(filename, std::ios::app);
    }

    ~Logger() {
        if (file.is_open())
            file.close();
    }

    void Log(LogCode code, uint8_t sessionID, bool success,
             const std::string& extra = "") {

        auto tag = [&](const std::string& msg) {
            return msg + (extra.empty() ? "" : " | " + extra);
        };

        switch (code) {
            case CS_JPEG:      write("CS_JPEG",       sessionID, success, tag("Client sent JPEG"));        break;
            case CR_JPEG:      write("CR_JPEG",       sessionID, success, tag("Client received JPEG"));    break;
            case CR_PRMT:      write("CR_PRMT",       sessionID, success, tag("Client received prompt"));  break;
            case CS_SEND:      write("CS_SEND",       sessionID, success, tag("Client sent data"));        break;
            case CR_RECV:      write("CR_RECV",       sessionID, success, tag("Client received data"));    break;
            case SS_JPEG:      write("SS_JPEG",       sessionID, success, tag("Server sent JPEG"));        break;
            case SR_JPEG:      write("SR_JPEG",       sessionID, success, tag("Server received JPEG"));    break;
            case SS_PRMT:      write("SS_PRMT",       sessionID, success, tag("Server sent prompt"));      break;
            case SS_SEND:      write("SS_SEND",       sessionID, success, tag("Server sent data"));        break;
            case SR_RECV:      write("SR_RECV",       sessionID, success, tag("Server received data"));    break;
            case SL_LOGIN:     write("SL_LOGIN",      sessionID, success, "Login: " + extra);              break;
            case SC_CONNECT:   write("SC_CONNECT",    sessionID, success, tag("Client connected"));        break;
            case SC_DISCONNECT:write("SC_DISCONNECT", sessionID, success, tag("Client disconnected"));     break;
            case SC_XSTATE:    write("SC_XSTATE",     sessionID, success, "State -> " + extra);            break;
            default:           write("UNKNOWN",       sessionID, false,   "Unknown log code");             break;
        }
    }
};
