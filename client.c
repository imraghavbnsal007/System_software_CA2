#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 12345                // Define the port number to connect to
#define BUFFER_SIZE 1024          // Define the buffer size for messages

int main() {
    int sock;
    struct sockaddr_in server;
    char message[BUFFER_SIZE], filename[100], directory[20], username[20], password[20];
    FILE *file;

    // Create a TCP socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Could not create socket");
        return 1;
    }
    printf("Socket created\n");

    // Configure server settings
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); // Set server IP address to localhost
    server.sin_family = AF_INET;                     // Set the address family to IPv4
    server.sin_port = htons(PORT);                   // Convert the port number from host to network byte order

    // Attempt to connect to the server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("connect failed. Error");
        return 1;
    }
    printf("Connected to server\n");

    // Prompt user for authentication details
    printf("Enter username: ");
    scanf("%s", username);
    printf("Enter password: ");
    scanf("%s", password);
    snprintf(message, BUFFER_SIZE, "%s %s", username, password); // Format username and password into message
    // Send authentication details to the server
    if (send(sock, message, strlen(message), 0) < 0) {
        puts("Send failed");
        return 1;
    }

    // Prompt for the filename and the directory
    printf("Enter the filename: ");
    scanf("%s", filename);
    printf("Enter directory (Manufacturing or Distribution): ");
    scanf("%s", directory);
    snprintf(message, BUFFER_SIZE, "%s %s", directory, filename); // Format directory and filename into message
    // Send directory and filename to the server
    if (send(sock, message, strlen(message), 0) < 0) {
        puts("Send failed");
        return 1;
    }

    // Open the file to be sent
    file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Failed to open file");
        return 1;
    }
    // Read file and send its contents to the server
    while (!feof(file)) {
        int bytes_read = fread(message, 1, BUFFER_SIZE, file);
        if (send(sock, message, bytes_read, 0) < 0) {
            puts("Failed to send file");
            break;
        }
    }
    fclose(file); // Close the file after sending its contents

    close(sock); // Close the socket
    return 0;
}
