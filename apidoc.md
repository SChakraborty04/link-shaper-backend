# URL Shortener API Documentation

## Base URL
```
http://<server_ip>:8000
current hosted <server_ip>: 34.131.130.95
```

---

## Endpoints

### 1. Register User

**POST** `/register`

**Parameters (form or JSON):**
- `email` (string, required)
- `password` (string, required, min 6 chars)

**Response:**
```json
{
  "status": "registered" | "error",
  "message": "..."
}
```

---

### 2. Login

**POST** `/login`

**Parameters:**
- `email` (string, required)
- `password` (string, required)

**Response:**
```json
{
  "status": "success" | "error",
  "message": "...",
  "token": "<JWT token>" // on success
}
```

---

### 3. Shorten URL

**POST** `/shorten`

**Headers:**
- `Authorization: Bearer <JWT token>`

**Parameters:**
- `longUrl` (string, required)
- `customParam` (string, optional, custom short URL)

**Response:**
```json
{
  "shortUrl": "<short_url>",
  "status": 201 | 409 | 401 | 500,
  "error": "..." // if any
}
```

---

### 4. Redirect/Get Long URL

**GET** `/<shortUrl>`

**Response:**
```json
{
  "url": "<long_url>",
  "status": 302 | 404 | 500
}
```

---

## Authentication

- Register and login to get a JWT token.
- Pass the token in the `Authorization` header for protected endpoints.

---

## Error Codes

- `401`: Unauthorized (missing/invalid token)
- `409`: Conflict (custom short URL exists)
- `404`: Not found (short URL does not exist)
- `500`: Internal server/database error

---

**Note:**  
All responses are in JSON format.  
For HTTPS, use `https://<server_ip>:8000` if SSL is configured (Currently not cconfigured for hosted build).