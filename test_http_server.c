#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 11112
#define BUFFER_SIZE 1024

void handle_client(int sock) {
    char buffer[BUFFER_SIZE];
    int bytes;

    bytes = recv(sock, buffer, sizeof(buffer), 0);
    if (bytes < 0) {
        perror("receive");
    } else {
        buffer[bytes] = 0;
        printf("Client request:\n%s\n", buffer);

        if (strncmp(buffer, "POST", 4) == 0) {
            // Extract username and password from the POST request
            char *username = strstr(buffer, "username=");
            char *password = strstr(buffer, "password=");
            if (username && password) {
                username += 9; // move past "username="
                password += 9; // move past "password="

                // Extract the values until the next '&' or end of the string
                char *end_username = strchr(username, '&');
                char *end_password = strchr(password, '&');
                if (end_username) *end_username = 0;
                if (end_password) *end_password = 0;

                printf("Username: %s\n", username);
                printf("Password: %s\n", password);

                const char reply[] = "HTTP/1.1 200 OK\r\n"
                                     "Content-Type: text/html\r\n"
                                     "Connection: close\r\n\r\n"
                                     "<html><body><h1>Login successful!</h1></body></html>\r\n";
                if( send(sock, reply, strlen(reply), 0) < 0 ){
                    perror("send");
                };
            }
        } else {
            const char *login_page =
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Connection: close\r\n\r\n"
                "<html><body>"
                "<h1>Login Page</h1>"
                "<form method=\"post\" action=\"/\">"
                "Username: <input type=\"text\" name=\"username\"><br>"
                "Password: <input type=\"password\" name=\"password\"><br>"
                "<input type=\"submit\" value=\"Login\">"
                "</form>"
                "</body></html>";
            if(send(sock, login_page, strlen(login_page), 0) < 0){
                perror("send");
            };
        }
    }
}

int main(int argc, char **argv) {
    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Unable to create socket");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Unable to bind");
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 1) < 0) {
        perror("Unable to listen");
        exit(EXIT_FAILURE);
    }

    while (1) {
        struct sockaddr_in addr;
        uint len = sizeof(addr);

        int client_sock = accept(sock, (struct sockaddr*)&addr, &len);
        if (client_sock < 0) {
            perror("Unable to accept");
            close(sock);
            exit(EXIT_FAILURE);
        }
        printf("connect!\n");

        handle_client(client_sock);

        close(client_sock);
    }

    close(sock);
    return 0;
}
