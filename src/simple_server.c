#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <time.h>

#define DEFAULT_PORT 8443
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

typedef struct {
    char traceid[33];  // 32 hex chars + null terminator
    char spanid[17];   // 16 hex chars + null terminator
} tracing_data_t;

static volatile int running = 1;
static int server_socket = -1;

void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down...\n", sig);
    running = 0;
    if (server_socket != -1) {
        close(server_socket);
    }
}

void init_openssl() {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    printf("OpenSSL initialized\n");
}

void cleanup_openssl() {
    EVP_cleanup();
    ERR_free_strings();
    printf("OpenSSL cleaned up\n");
}

void print_tracing_data(const tracing_data_t *tracing) {
    if (!tracing) {
        printf("Tracing data: NULL\n");
        return;
    }
    
    printf("Received Tracing Data:\n");
    printf("  TraceID: %s\n", tracing->traceid);
    printf("  SpanID:  %s\n", tracing->spanid);
}

int parse_tracing_data(const char *json_data, tracing_data_t *tracing) {
    if (!json_data || !tracing) {
        return -1;
    }
    
    // Simple JSON parsing (in production, use a proper JSON library)
    char *traceid_start = strstr(json_data, "\"traceid\": \"");
    char *spanid_start = strstr(json_data, "\"spanid\": \"");
    
    if (!traceid_start || !spanid_start) {
        printf("Could not find traceid or spanid in JSON data\n");
        return -1;
    }
    
    // Extract traceid
    traceid_start += 12; // Skip "\"traceid\": \""
    char *traceid_end = strchr(traceid_start, '"');
    if (!traceid_end || (traceid_end - traceid_start) != 32) {
        printf("Invalid traceid format\n");
        return -1;
    }
    strncpy(tracing->traceid, traceid_start, 32);
    tracing->traceid[32] = '\0';
    
    // Extract spanid
    spanid_start += 11; // Skip "\"spanid\": \""
    char *spanid_end = strchr(spanid_start, '"');
    if (!spanid_end || (spanid_end - spanid_start) != 16) {
        printf("Invalid spanid format\n");
        return -1;
    }
    strncpy(tracing->spanid, spanid_start, 16);
    tracing->spanid[16] = '\0';
    
    return 0;
}

int send_response(int client_socket, const char *message) {
    int bytes_sent = send(client_socket, message, strlen(message), 0);
    if (bytes_sent < 0) {
        perror("Failed to send response");
        return -1;
    }
    
    printf("Sent response (%d bytes): %s\n", bytes_sent, message);
    return 0;
}

int handle_client_connection(int client_socket) {
    printf("New client connected\n");
    
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received < 0) {
        perror("Failed to receive data");
        close(client_socket);
        return -1;
    }
    
    buffer[bytes_received] = '\0';
    printf("Received data (%d bytes):\n%s\n", bytes_received, buffer);
    
    // Parse the tracing data
    tracing_data_t tracing;
    if (parse_tracing_data(buffer, &tracing) == 0) {
        print_tracing_data(&tracing);
        
        // Process the tracing data (in a real application, you might store it in a database)
        printf("Processing tracing data...\n");
        
        // Send acknowledgment response
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response),
            "{\n"
            "  \"status\": \"success\",\n"
            "  \"message\": \"Tracing data received and processed\",\n"
            "  \"received_traceid\": \"%s\",\n"
            "  \"received_spanid\": \"%s\",\n"
            "  \"timestamp\": %ld\n"
            "}\n",
            tracing.traceid, tracing.spanid, time(NULL));
        
        send_response(client_socket, response);
        
    } else {
        printf("Failed to parse tracing data\n");
        
        // Send error response
        char error_response[BUFFER_SIZE];
        snprintf(error_response, sizeof(error_response),
            "{\n"
            "  \"status\": \"error\",\n"
            "  \"message\": \"Failed to parse tracing data\"\n"
            "}\n");
        
        send_response(client_socket, error_response);
    }
    
    close(client_socket);
    printf("Client connection closed\n");
    
    return 0;
}

int setup_server_socket(int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Failed to create socket");
        return -1;
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(sock);
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sock);
        return -1;
    }
    
    if (listen(sock, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(sock);
        return -1;
    }
    
    return sock;
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    
    // Parse command line arguments
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    printf("Simple Tracing Server\n");
    printf("Starting server on port %d\n", port);
    
    // Initialize OpenSSL
    init_openssl();
    
    // Set up signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create server socket
    server_socket = setup_server_socket(port);
    if (server_socket < 0) {
        fprintf(stderr, "Failed to setup server socket\n");
        cleanup_openssl();
        return 1;
    }
    
    printf("Server listening on port %d\n", port);
    printf("Press Ctrl+C to stop the server\n");
    
    // Main server loop
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (running) {
                perror("Accept failed");
            }
            continue;
        }
        
        printf("Client connected from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // Handle client (in a real application, you'd use threads)
        handle_client_connection(client_socket);
    }
    
    // Cleanup
    if (server_socket != -1) {
        close(server_socket);
    }
    
    cleanup_openssl();
    printf("Server stopped\n");
    
    return 0;
} 