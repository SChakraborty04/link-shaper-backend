FROM ubuntu:22.04

# Install build tools and dependencies
RUN apt-get update && \
    apt-get install -y g++ make libssl-dev

WORKDIR /app

# Copy all source and dependency folders
COPY shortenurl.cpp .
COPY sqllite/sqlite3.c ./sqllite/sqlite3.c
COPY sqllite/sqlite3.h ./sqllite/sqlite3.h
COPY cpp-httplib-master/ ./cpp-httplib-master/
COPY json-develop/include/ ./json-develop/include/
COPY PicoSHA2-master/picosha2.h ./PicoSHA2-master/picosha2.h
COPY jwt-cpp-master/include/ ./jwt-cpp-master/include/

# Compile sqlite3.c to object file
RUN g++ -c -I./sqllite ./sqllite/sqlite3.c -o ./sqllite/sqlite3.o

# Build the application (remove -lws2_32 for Linux)
RUN g++ -I./ -I./sqllite -I./cpp-httplib-master/ -I./json-develop/include -I./PicoSHA2-master -I./jwt-cpp-master/include \
    -o shortenurl shortenurl.cpp ./sqllite/sqlite3.o -lssl -lcrypto

EXPOSE 8000

CMD ["./shortenurl"]