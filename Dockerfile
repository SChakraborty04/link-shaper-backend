FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    gcc \
    g++ \
    make \
    git \
    wget \
    unzip \
    libssl-dev

WORKDIR /app

# Download dependencies
RUN git clone https://github.com/yhirose/cpp-httplib.git && \
    git clone https://github.com/nlohmann/json.git && \
    git clone https://github.com/Thalhammer/jwt-cpp.git && \
    git clone https://github.com/okdshin/PicoSHA2.git

# Download SQLite amalgamation
RUN mkdir sqllite && \
    wget https://www.sqlite.org/2025/sqlite-amalgamation-3490100.zip && \
    unzip sqlite-amalgamation-3490100.zip && \
    cp sqlite-amalgamation-3490100/sqlite3.c sqllite/ && \
    cp sqlite-amalgamation-3490100/sqlite3.h sqllite/

COPY shortenurl.cpp .

# Build sqlite
RUN gcc -c -I./sqllite ./sqllite/sqlite3.c -o ./sqllite/sqlite3.o

# Build application
RUN g++ \
    -std=c++17 \
    -O2 \
    -I./ \
    -I./sqllite \
    -I./cpp-httplib \
    -I./json/include \
    -I./PicoSHA2 \
    -I./jwt-cpp/include \
    -o shortenurl \
    shortenurl.cpp \
    ./sqllite/sqlite3.o \
    -lssl \
    -lcrypto

EXPOSE 8000

CMD ["./shortenurl"]