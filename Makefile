CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = -lssl -lcrypto

# Targets
TARGETS = simple_server simple_client

# Default target
all: $(TARGETS)

# Simple server
simple_server: src/simple_server.c
	$(CC) $(CFLAGS) -o simple_server src/simple_server.c $(LDFLAGS)

# Simple client
simple_client: src/simple_client.c
	$(CC) $(CFLAGS) -o simple_client src/simple_client.c $(LDFLAGS)

# Clean
clean:
	rm -f $(TARGETS)

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt update
	sudo apt install -y build-essential libssl-dev

# Run server
run-server: simple_server
	./simple_server

# Run client
run-client: simple_client
	./simple_client

# Test (run server in background, then client)
test: simple_server simple_client
	@echo "Starting server..."
	@./simple_server &
	@sleep 2
	@echo "Running client..."
	@./simple_client
	@echo "Stopping server..."
	@pkill -f simple_server

.PHONY: all clean install-deps run-server run-client test 