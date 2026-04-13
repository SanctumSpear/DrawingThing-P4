/**
 * @file logger.h
 * @brief Defines logging codes and a simple logger class for recording client, server,
 *        authentication, and game state events.
 */

#pragma once
#include <string>
#include <fstream>
#include <cstdint>
#include <ctime>
#include <cstring>

/**
 * @enum LogCode
 * @brief Identifiers for different log events in the client/server system.
 */
enum LogCode {
    CS_JPEG,       ///< Client sent JPEG
    CR_JPEG,       ///< Client received JPEG
    CR_PRMT,       ///< Client received prompt
    CS_SEND,       ///< Client sent data
    CR_RECV,       ///< Client received data

    SS_JPEG,       ///< Server sent JPEG
    SR_JPEG,       ///< Server received JPEG
    SS_PRMT,       ///< Server sent prompt
    SS_SEND,       ///< Server sent data
    SR_RECV,       ///< Server received data

    SL_LOGIN,      ///< Login attempt
    SC_CONNECT,    ///< Client connected
    SC_DISCONNECT, ///< Client disconnected

    SC_XSTATE      ///< State transition
};

/**
 * @class Logger
 * @brief Writes timestamped log entries to a file.
 */
class Logger {
private:
    std::ofstream file; ///< Output log file stream

    /**
     * @brief Writes a formatted log entry to the file.
     * @param code Short log code string
     * @param sessionID Session ID associated with the log
     * @param success Whether the operation succeeded
     * @param msg Extra message to include
     */
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
    /**
     * @brief Opens the log file.
     * @param filename Name of the log file
     */
    Logger(const std::string& filename = "log.txt") {
        file.open(filename, std::ios::app);
    }

    /**
     * @brief Closes the log file if it is open.
     */
    ~Logger() {
        if (file.is_open())
            file.close();
    }

    /**
     * @brief Logs an event to the file.
     * @param code Log event type
     * @param sessionID Session ID
     * @param success Whether the operation succeeded
     * @param extra Optional extra message
     */
    void Log(LogCode code, uint8_t sessionID, bool success,
        const std::string& extra = "") {

        auto tag = [&](const std::string& msg) {
            return msg + (extra.empty() ? "" : " | " + extra);
            };

        switch (code) {
        case CS_JPEG:      write("CS_JPEG", sessionID, success, tag("Client sent JPEG"));        break;
        case CR_JPEG:      write("CR_JPEG", sessionID, success, tag("Client received JPEG"));    break;
        case CR_PRMT:      write("CR_PRMT", sessionID, success, tag("Client received prompt"));  break;
        case CS_SEND:      write("CS_SEND", sessionID, success, tag("Client sent data"));        break;
        case CR_RECV:      write("CR_RECV", sessionID, success, tag("Client received data"));    break;
        case SS_JPEG:      write("SS_JPEG", sessionID, success, tag("Server sent JPEG"));        break;
        case SR_JPEG:      write("SR_JPEG", sessionID, success, tag("Server received JPEG"));    break;
        case SS_PRMT:      write("SS_PRMT", sessionID, success, tag("Server sent prompt"));      break;
        case SS_SEND:      write("SS_SEND", sessionID, success, tag("Server sent data"));        break;
        case SR_RECV:      write("SR_RECV", sessionID, success, tag("Server received data"));    break;
        case SL_LOGIN:     write("SL_LOGIN", sessionID, success, "Login: " + extra);              break;
        case SC_CONNECT:   write("SC_CONNECT", sessionID, success, tag("Client connected"));        break;
        case SC_DISCONNECT:write("SC_DISCONNECT", sessionID, success, tag("Client disconnected"));     break;
        case SC_XSTATE:    write("SC_XSTATE", sessionID, success, "State -> " + extra);            break;
        default:           write("UNKNOWN", sessionID, false, "Unknown log code");             break;
        }
    }
};