#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <time.h>

#define PORT 12345                // Port number for the server
#define BUFFER_SIZE 1024          // Buffer size for data transfer
#define LOG_FILE "transfer.log"   // Log file name

struct User {
    char username[20];             // Username
    char password[20];             // Password
};

struct User users[] = {            // Array of users for authentication
    {"admin1", "password1"},
    {"admin2", "password2"},
    {"admin3", "password3"}
};

// Function to authenticate a user
int authenticate(char *username, char *password) {
    for (int i = 0; i < sizeof(users)/sizeof(users[0]); i++) {
        if (strcmp(username, users[i].username) == 0 && strcmp(password, users[i].password) == 0)
            return 1; // Return 1 if authentication is successful
    }
    return 0; // Return 0 if authentication fails
}

// Function to log file transfers
void log_transfer(const char *username, const char *filename, const char *directory) {
    FILE *log = fopen(LOG_FILE, "a");
    if (log == NULL) {
        perror("Failed to open log file");
        return;
    }
    time_t now = time(NULL);
    char *dt = ctime(&now);
    fprintf(log, "%s: %s transferred '%s' to '%s'\n", dt, username, filename, directory);
    fclose(log);
}

// Thread function to handle client requests
void *client_handler(void *socket_desc) {
    int sock = *(int*)socket_desc;
    char client_message[BUFFER_SIZE], username[20], password[20], filename[100], directory[20], filepath[150];
    FILE *file;
    int read_size;

    memset(client_message, 0, BUFFER_SIZE); // Initialize buffer

    // Authenticate user
    if ((read_size = recv(sock, client_message, BUFFER_SIZE - 1, 0)) > 0) {
        client_message[read_size] = '\0'; // Ensure string is null-terminated
        sscanf(client_message, "%s %s", username, password);
        if (!authenticate(username, password)) {
            printf("Authentication failed for user %s\n", username);
            close(sock);
            free(socket_desc);
            return 0;
        }
        printf("User %s authenticated successfully.\n", username);
    } else {
        close(sock);
        free(socket_desc);
        return 0; // Exit if error occurs
    }

    memset(client_message, 0, BUFFER_SIZE); // Clear buffer after authentication

    // Receive and process file transfer request
    if ((read_size = recv(sock, client_message, BUFFER_SIZE - 1, 0)) > 0) {
        client_message[read_size] = '\0';
        sscanf(client_message, "%s %s", directory, filename);
        snprintf(filepath, sizeof(filepath), "%s/%s", directory, filename);
        file = fopen(filepath, "wb");
        while ((read_size = recv(sock, client_message, BUFFER_SIZE, 0)) > 0) {
            fwrite(client_message, 1, read_size, file); // Write to file
        }
        fclose(file);
        log_transfer(username, filename, directory);  // Log the file transfer
    }

    close(sock);
    free(socket_desc);
    return 0;
}

// Main function to set up the server
int main() {
    int socket_desc, client_sock, c;
    struct sockaddr_in server, client;

    // Create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_desc == -1) {
        perror("Could not create socket");
        return 1;
    }

    // Bind socket to all local addresses and set port to 12345
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("bind failed");
        return 1;
    }

    listen(socket_desc, 3); // Listen for up to 3 connections
    printf("Waiting for incoming connections...\n");
    c = sizeof(struct sockaddr_in);

    while (1) {
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock < 0) {
            perror("accept failed");
            continue;
        }

        pthread_t sniffer_thread;
        int *new_sock = malloc(sizeof(int));
        *new_sock = client_sock;

        // Create a new thread for each connection
        if(pthread_create(&sniffer_thread, NULL, client_handler, (void*) new_sock) < 0) {
            perror("could not create thread");
            continue;
        }
        pthread_detach(sniffer_thread); // Detach the thread so it cleans up after itself
    }

    return 0;
}
