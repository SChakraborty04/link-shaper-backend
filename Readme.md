# URL Shortener API (C++)

A simple, secure, and fast URL shortener backend built in C++ using [cpp-httplib](https://github.com/yhirose/cpp-httplib), [SQLite](https://www.sqlite.org/), JWT authentication, and password hashing. Deployable on any server or cloud VM.

---

## Features

- User registration and login with JWT authentication
- Secure password hashing (SHA-256)
- Shorten URLs (with optional custom aliases)
- Redirect to original URLs
- Persistent storage with SQLite
- RESTful API endpoints
- Easily deployable behind NGINX/Cloudflare for HTTPS

---
### Demo Website 
 - https://link-shaper-ui.lovable.app
 - Demo email: test@test.com
 - Demo Password: 123456
---
### Demo Backend
 - Hosted API: https://shortapi.sandipanchakraborty.site
---

## Build Instructions

### **Dependencies**

- g++ (C++17 or later)
Download all these and put in the root of the folder (where shortenurl.cpp lies)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [nlohmann/json](https://github.com/nlohmann/json)
- [jwt-cpp](https://github.com/Thalhammer/jwt-cpp)
- [PicoSHA2](https://github.com/okdshin/PicoSHA2)
- [SQLite3 amalgamation](https://www.sqlite.org/download.html)
- OpenSSL (for JWT HMAC)

### **Build Command**

```sh
g++ -I./ -I./sqllite -I./cpp-httplib-master/ -I./json-develop/include -I./PicoSHA2-master -I./jwt-cpp-master/include -o shortenurl shortenurl.cpp ./sqllite/sqlite3.o -lws2_32 -lssl -lcrypto
```

---

## Usage

1. **Start the server:**
   ```
   ./shortenurl
   ```
   The server listens on port 8000 by default.

2. **Recommended:**  
   Deploy behind NGINX or Cloudflare for HTTPS and custom domain support.

---

## API Endpoints

### **POST `/register`**
Register a new user.
- **Params:** `email`, `password`
- **Response:**  
  `{"status":"registered"}` or error message

### **POST `/login`**
Authenticate and receive a JWT token.
- **Params:** `email`, `password`
- **Response:**  
  `{"status":"success", "token":"..."}`

### **POST `/shorten`**
Shorten a URL (JWT required).
- **Headers:** `Authorization: Bearer <token>`
- **Params:** `longUrl`, `customParam` (optional)
- **Response:**  
  `{"shortUrl":"...", "status":201}`

### **GET `/<shortUrl>`**
Redirect or fetch the original URL.
- **Response:**  
  `{"url":"...", "status":302}` or `{"status":404}`

---

## Deployment Notes

- For production, run behind NGINX or Cloudflare for HTTPS.
- If using Cloudflare, you do **not** need SSL on your backend VM.
- Open port 8000 only to localhost or NGINX; expose port 80/443 for public access.

---

## Performance

- Tested with k6 and Grafana Cloud.
- Handles 50+ requests/sec with sub-500ms P95 latency and 0% error rate on a single VM.

---

## License

MIT

---

**Author:**  
Sandipan Chakraborty 😎