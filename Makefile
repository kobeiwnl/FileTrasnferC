CC = gcc
CFLAGS = -Wall -Wextra -fPIC
LDFLAGS = -L. -lftp_lib
LIB_NAME = libftp_lib.so
LIB_OBJ = ftp_lib.o
CLIENT = myFTclient
SERVER = myFTserver

.PHONY: all clean

all: $(LIB_NAME) $(CLIENT) $(SERVER)

$(LIB_NAME): ftp_lib.c
	$(CC) $(CFLAGS) -c ftp_lib.c -o $(LIB_OBJ)
	$(CC) -shared -o $(LIB_NAME) $(LIB_OBJ)

$(CLIENT): myFTclient.c
	$(CC) -o $(CLIENT) myFTclient.c $(LDFLAGS)

$(SERVER): myFTserver.c
	$(CC) -o $(SERVER) myFTserver.c $(LDFLAGS)

clean:
	rm -f $(CLIENT) $(SERVER) $(LIB_NAME) $(LIB_OBJ)
