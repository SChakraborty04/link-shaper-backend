#pragma once

const char* CREATE_USERS_TABLE_SQL =
    "CREATE TABLE IF NOT EXISTS users ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "email TEXT UNIQUE NOT NULL, "
    "password TEXT NOT NULL"
    ");";

const char* CREATE_URLS_TABLE_SQL =
    "CREATE TABLE IF NOT EXISTS urls ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
    "shortUrl TEXT UNIQUE NOT NULL, "
    "longUrl TEXT NOT NULL, "
    "visits INTEGER DEFAULT 0, "
    "user INTEGER NOT NULL, "
    "FOREIGN KEY(user) REFERENCES users(id)"
    ");";