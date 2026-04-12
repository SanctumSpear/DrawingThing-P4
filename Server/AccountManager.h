#pragma once
#include <string>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>

// Manages player accounts stored in a plain-text file.
// File format: one account per line as   username:password
//
// Covers REQ-SVR-030 (create), REQ-SVR-040 (delete),
//         REQ-SVR-050 (update), REQ-INT-030 (authenticate).

class AccountManager {
private:
    std::string filename;
    std::map<std::string, std::string> accounts; // username -> password

    void LoadAccounts() {
        accounts.clear();
        std::ifstream in(filename);
        if (!in.is_open()) return;

        std::string line;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            auto pos = line.find(':');
            if (pos == std::string::npos) continue;
            std::string user = line.substr(0, pos);
            std::string pass = line.substr(pos + 1);
            accounts[user] = pass;
        }
    }

    void SaveAccounts() const {
        std::ofstream out(filename, std::ios::trunc);
        if (!out.is_open()) {
            std::cerr << "AccountManager: could not write to " << filename << "\n";
            return;
        }
        for (const auto& pair : accounts)
            out << pair.first << ":" << pair.second << "\n";
    }

public:
    AccountManager(const std::string& file = "accounts.txt")
        : filename(file) {
        LoadAccounts();
    }

    // Returns true if username exists and password matches.
    bool Authenticate(const std::string& username,
                      const std::string& password) const {
        auto it = accounts.find(username);
        if (it == accounts.end()) return false;
        return it->second == password;
    }

    // REQ-SVR-030: create a new account.
    // Returns false if the username already exists.
    bool CreateAccount(const std::string& username,
                       const std::string& password) {
        if (accounts.count(username)) return false;
        accounts[username] = password;
        SaveAccounts();
        return true;
    }

    // REQ-SVR-040: remove an existing account.
    // Returns false if the username does not exist.
    bool DeleteAccount(const std::string& username) {
        if (!accounts.count(username)) return false;
        accounts.erase(username);
        SaveAccounts();
        return true;
    }

    // REQ-SVR-050: change the password for an existing account.
    // Returns false if the username does not exist.
    bool UpdatePassword(const std::string& username,
                        const std::string& newPassword) {
        auto it = accounts.find(username);
        if (it == accounts.end()) return false;
        it->second = newPassword;
        SaveAccounts();
        return true;
    }

    bool AccountExists(const std::string& username) const {
        return accounts.count(username) > 0;
    }
};
