#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <time.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 8443
#define BUFFER_SIZE 1024

typedef struct {
    char traceid[33];  // 32 hex chars + null terminator
    char spanid[17];   // 16 hex chars + null terminator
} tracing_data_t;

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

int generate_tracing_data(tracing_data_t *tracing) {
    if (!tracing) {
        return -1;
    }
    
    // Generate random traceid (32 hex characters)
    unsigned char traceid_bytes[16];
    if (RAND_bytes(traceid_bytes, 16) != 1) {
        fprintf(stderr, "Failed to generate random traceid\n");
        return -1;
    }
    
    for (int i = 0; i < 16; i++) {
        sprintf(&tracing->traceid[i * 2], "%02x", traceid_bytes[i]);
    }
    tracing->traceid[32] = '\0';
    
    // Generate random spanid (16 hex characters)
    unsigned char spanid_bytes[8];
    if (RAND_bytes(spanid_bytes, 8) != 1) {
        fprintf(stderr, "Failed to generate random spanid\n");
        return -1;
    }
    
    for (int i = 0; i < 8; i++) {
        sprintf(&tracing->spanid[i * 2], "%02x", spanid_bytes[i]);
    }
    tracing->spanid[16] = '\0';
    
    return 0;
}

void print_tracing_data(const tracing_data_t *tracing) {
    if (!tracing) {
        printf("Tracing data: NULL\n");
        return;
    }
    
    printf("Tracing Data:\n");
    printf("  TraceID: %s\n", tracing->traceid);
    printf("  SpanID:  %s\n", tracing->spanid);
}

int send_tracing_data(int sockfd, const tracing_data_t *tracing) {
    if (!tracing) {
        return -1;
    }
    
    // Create JSON-like message
    char message[BUFFER_SIZE];
    snprintf(message, sizeof(message),
        "{\n"
        "  \"type\": \"tracing_data\",\n"
        "  \"traceid\": \"%s\",\n"
        "  \"spanid\": \"%s\",\n"
        "  \"timestamp\": %ld\n"
        "}\n",
        tracing->traceid, tracing->spanid, time(NULL));
    
    // Send the message
    int bytes_sent = send(sockfd, message, strlen(message), 0);
    if (bytes_sent < 0) {
        perror("Failed to send data");
        return -1;
    }
    
    printf("Sent %d bytes to server\n", bytes_sent);
    printf("Message sent:\n%s\n", message);
    
    return 0;
}

int receive_response(int sockfd) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received < 0) {
        perror("Failed to receive response");
        return -1;
    }
    
    buffer[bytes_received] = '\0';
    printf("Received response (%d bytes):\n%s\n", bytes_received, buffer);
    
    return 0;
}

int main(int argc, char *argv[]) {
    char *host = DEFAULT_HOST;
    int port = DEFAULT_PORT;
    
    // Parse command line arguments
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = atoi(argv[2]);
    }
    
    printf("Simple Tracing Client\n");
    printf("Connecting to %s:%d\n", host, port);
    
    // Initialize OpenSSL
    init_openssl();
    
    // Generate tracing data
    tracing_data_t tracing;
    if (generate_tracing_data(&tracing) != 0) {
        fprintf(stderr, "Failed to generate tracing data\n");
        cleanup_openssl();
        return 1;
    }
    
    printf("Generated tracing data:\n");
    print_tracing_data(&tracing);
    
    // Create socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Failed to create socket");
        cleanup_openssl();
        return 1;
    }
    
    // Set up server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, host, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sockfd);
        cleanup_openssl();
        return 1;
    }
    
    // Connect to server
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        cleanup_openssl();
        return 1;
    }
    
    printf("Connected to server successfully\n");
    
    // Send tracing data
    if (send_tracing_data(sockfd, &tracing) != 0) {
        fprintf(stderr, "Failed to send tracing data\n");
        close(sockfd);
        cleanup_openssl();
        return 1;
    }
    
    // Receive response
    if (receive_response(sockfd) != 0) {
        fprintf(stderr, "Failed to receive response\n");
        close(sockfd);
        cleanup_openssl();
        return 1;
    }
    
    printf("Successfully sent tracing data to server\n");
    
    // Cleanup
    close(sockfd);
    cleanup_openssl();
    
    printf("Client completed successfully\n");
    return 0;
} 