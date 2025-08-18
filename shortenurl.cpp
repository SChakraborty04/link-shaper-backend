// Define Windows 10 as minimum target version
#define _WIN32_WINNT 0x0A00

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <unordered_map>
#include <winsock2.h>
#include <windows.h>
#include <regex>
#include "httplib.h"
#include "nlohmann/json.hpp"
#include "sqllite/sqlite3.h"
#include "schemas.h"
#include "PicoSHA2-master/picosha2.h"
#include "jwt-cpp-master/include/jwt-cpp/jwt.h"

using namespace std;
using namespace httplib;
using namespace picosha2;
using json = nlohmann::json;

unordered_map<string,string>shortToLong;

string shortenUrl(const string& longUrl) {
    static const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    const int length = 5;
    string shortUrl;
    do {
        shortUrl.clear();
        srand(static_cast<unsigned int>(time(nullptr)) + rand());
        for (int i = 0; i < length; ++i) {
            shortUrl += charset[rand() % (sizeof(charset) - 1)];
        }
    } while (shortToLong.find(shortUrl) != shortToLong.end()); // Regenerate if exists
    cout << "shortUrl: " << shortUrl << endl;
    return shortUrl;
}

void populate_map(sqlite3* DB){
    std::string sql = "SELECT shortUrl, longUrl FROM urls;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(DB, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string dbShortUrl = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string dbLongUrl = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            shortToLong[dbShortUrl] = dbLongUrl;
        }
        sqlite3_finalize(stmt);
    }
}


int main(int argc, char const *argv[]){
    Server server;

    sqlite3* DB;
    int exit = 0;
    exit = sqlite3_open("short.db",&DB);
    if (exit) {
        cerr << "Error open DB " << sqlite3_errmsg(DB) << endl;
        return (-1);
    }
    else
        cout << "Opened Database Successfully!" << endl;
    
    // Create tables
    char* errMsg = nullptr;
    if (sqlite3_exec(DB, CREATE_USERS_TABLE_SQL, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        cerr << "Error creating users table: " << errMsg << endl;
        sqlite3_free(errMsg);
        return (-1);
    }
    if (sqlite3_exec(DB, CREATE_URLS_TABLE_SQL, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        cerr << "Error creating urls table: " << errMsg << endl;
        sqlite3_free(errMsg);
        return (-1);
    }

    //populate map with short-urls on server restart
    populate_map(DB);



        


    //Server functions
    server.Post("/register", [&](const Request& req, Response& res) {
        string email = req.get_param_value("email");
        string password = req.get_param_value("password");

        json jsonResponse;
        // Validate email format
        std::regex email_regex(R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");
        if (!std::regex_match(email, email_regex)) {
            jsonResponse["status"] = "error";
            jsonResponse["message"] = "Invalid email format";
            res.set_content(jsonResponse.dump(), "application/json");
            return;
        }

        // Validate password length
        if (password.length() < 6) {
            jsonResponse["status"] = "error";
            jsonResponse["message"] = "Password must be at least 6 characters";
            res.set_content(jsonResponse.dump(), "application/json");
            return;
        }

        // Check if email already exists
        std::string checkSql = "SELECT email FROM users WHERE email = ?;";
        sqlite3_stmt* checkStmt;
        bool exists = false;
        if (sqlite3_prepare_v2(DB, checkSql.c_str(), -1, &checkStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(checkStmt, 1, email.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(checkStmt) == SQLITE_ROW) {
                exists = true;
            }
            sqlite3_finalize(checkStmt);
        }
        if (exists) {
            jsonResponse["status"] = "error";
            jsonResponse["message"] = "Email already registered";
            res.set_content(jsonResponse.dump(), "application/json");
            return;
        }

        //Hash the password
        string hash = hash256_hex_string(password);

        string sql = "INSERT INTO users (email, password) VALUES (?, ?);";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(DB, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, hash.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_DONE) {
                jsonResponse["status"]="registered";
                res.set_content(jsonResponse.dump(), "application/json");
            } else {
                jsonResponse["status"]="error";
                jsonResponse["message"]="Internal server error";
                res.set_content(jsonResponse.dump(), "application/json");
            }
            sqlite3_finalize(stmt);
        } else {
            jsonResponse["status"]="error";
            jsonResponse["message"]="DB error";
            res.set_content(jsonResponse.dump(), "application/json");
        }
    });
    //Login
    server.Post("/login", [&](const Request& req, Response& res) {
        string email = req.get_param_value("email");
        string password = req.get_param_value("password");
        string hash = hash256_hex_string(password);

        json jsonResponse;

        string sql = "SELECT password FROM users WHERE email = ?;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(DB, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, email.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                string db_hash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (db_hash == hash) {
                    auto token = jwt::create()
                        .set_issuer("urlshortener")
                        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours{1})
                        .set_payload_claim("email", jwt::claim(email))
                        .sign(jwt::algorithm::hs256{"PleaseShortMyURL!"});
                    jsonResponse["token"] = token;
                    jsonResponse["status"] = "success";
                    jsonResponse["message"] = "Login successful";
                    // jsonResponse["token"] = jwt_token;
                } else {
                    jsonResponse["status"] = "error";
                    jsonResponse["message"] = "Invalid password";
                }
            } else {
                jsonResponse["status"] = "error";
                jsonResponse["message"] = "User not found";
            }
            sqlite3_finalize(stmt);
        } else {
            jsonResponse["status"] = "error";
            jsonResponse["message"] = "DB error";
        }
        res.set_content(jsonResponse.dump(), "application/json");
    });

    server.Post("/shorten",[&](const Request& req, Response& res){
        string longUrl = req.get_param_value("longUrl");
        string customShort = req.get_param_value("customParam");
        string shortUrl;
        json jsonResponse;

        // JWT authentication
        string authHeader = req.get_header_value("Authorization");
        if (authHeader.substr(0,7) != "Bearer ") {
            jsonResponse["error"] = "Missing or invalid token";
            jsonResponse["status"] = 401;
            res.set_content(jsonResponse.dump(), "application/json");
            return;
        }
        string token = authHeader.substr(7);
        string userEmail;
        try {
            auto decoded = jwt::decode(token);
            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{"PleaseShortMyURL!"})
                .with_issuer("urlshortener");
            verifier.verify(decoded);
            userEmail = decoded.get_payload_claim("email").as_string();
        } catch (const std::exception& e) {
            jsonResponse["error"] = "Invalid or expired token";
            jsonResponse["status"] = 401;
            res.set_content(jsonResponse.dump(), "application/json");
            return;
        }
        //Search userid from email
        int userId = -1;
        std::string getUserIdSql = "SELECT id FROM users WHERE email = ?;";
        sqlite3_stmt* userIdStmt;
        if (sqlite3_prepare_v2(DB, getUserIdSql.c_str(), -1, &userIdStmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(userIdStmt, 1, userEmail.c_str(), -1, SQLITE_TRANSIENT);
            if (sqlite3_step(userIdStmt) == SQLITE_ROW) {
                userId = sqlite3_column_int(userIdStmt, 0);
            }
            sqlite3_finalize(userIdStmt);
        }

        // Check for custom short link
        if (!customShort.empty()) {
            if (shortToLong.find(customShort) != shortToLong.end()) {
                jsonResponse["error"] = "Custom short link already exists";
                jsonResponse["status"] = 409;
                jsonResponse["longUrl"] = shortToLong[customShort];
                res.set_content(jsonResponse.dump(), "application/json");
                return;
            }
            shortUrl = customShort;
        } else {
            shortUrl = shortenUrl(longUrl);
        }

        // Save to in-memory map
        shortToLong[shortUrl] = longUrl;

        // Save to database
        string sql = "INSERT INTO urls (shortUrl, longUrl, user) VALUES (?, ?, ?);";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(DB, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, shortUrl.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, longUrl.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 3, userId);
            if (sqlite3_step(stmt) == SQLITE_DONE) {
                jsonResponse["shortUrl"] = shortUrl;
                jsonResponse["status"] = 201;
            } else {
                jsonResponse["error"] = "Failed to save URL";
                jsonResponse["status"] = 500;
            }
            sqlite3_finalize(stmt);
        } else {
            jsonResponse["error"] = "DB error";
            jsonResponse["status"] = 500;
        }
        res.set_content(jsonResponse.dump(), "application/json");
    });

    server.Get(R"(/(\w+))",[&](const Request& req, Response& res){
        string shortUrl = req.matches[1];

        json jsonResponse;

        if(shortToLong.find(shortUrl) != shortToLong.end()){
            jsonResponse["url"] = shortToLong[shortUrl];
            jsonResponse["status"] = 302;
            res.set_content(jsonResponse.dump(),"application/json");

            // Increment visit count in DB (after response)
            std::string sql = "UPDATE urls SET visits = visits + 1 WHERE shortUrl = ?;";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(DB, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, shortUrl.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);
            }
        }
        else{
            // Get the requested shortUrl from database and save only that to the map
            string sql = "SELECT shortUrl, longUrl FROM urls WHERE shortUrl = ?;";
            sqlite3_stmt* stmt;
            if (sqlite3_prepare_v2(DB, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, shortUrl.c_str(), -1, SQLITE_TRANSIENT);
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                    std::string dbShortUrl = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                    std::string dbLongUrl = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                    shortToLong[dbShortUrl] = dbLongUrl;
                    jsonResponse["url"] = dbLongUrl;
                    jsonResponse["status"] = 302;
                    res.set_content(jsonResponse.dump(),"application/json");
                    // Increment visit count in DB (after response)
                    std::string sql = "UPDATE urls SET visits = visits + 1 WHERE shortUrl = ?;";
                    sqlite3_stmt* stmt;
                    if (sqlite3_prepare_v2(DB, sql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
                        sqlite3_bind_text(stmt, 1, shortUrl.c_str(), -1, SQLITE_TRANSIENT);
                        sqlite3_step(stmt);
                        sqlite3_finalize(stmt);
                    }
                } else {
                    jsonResponse["url"] = "";
                    jsonResponse["status"] = 404;
                    res.set_content(jsonResponse.dump(),"application/json");
                }
                sqlite3_finalize(stmt);
            } else {
                jsonResponse["url"] = "";
                jsonResponse["status"] = 500;
                res.set_content(jsonResponse.dump(),"application/json");
            }
        }
    });
    cout<<"Server listening on port 8000\n";
    server.listen("localhost", 8000);
    sqlite3_close(DB);
    return 0;
}
