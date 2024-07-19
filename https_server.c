#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define PORT 11111
#define BUFFER_SIZE 1024

void initialize_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

void cleanup_openssl() {
    EVP_cleanup();
}

SSL_CTX* create_context() {
    const SSL_METHOD *method;
    SSL_CTX *ctx;

    method = SSLv23_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx) {
        perror("Unable to create SSL context");
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }

    return ctx;
}

void configure_context(SSL_CTX *ctx) {
    SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM);
}

void handle_client(SSL *ssl) {
    char buffer[BUFFER_SIZE];
    int bytes;

    bytes = SSL_read(ssl, buffer, sizeof(buffer));
    if (bytes < 0) {
        ERR_print_errors_fp(stderr);
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
                SSL_write(ssl, reply, strlen(reply));
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
            SSL_write(ssl, login_page, strlen(login_page));
        }
    }
}

int main(int argc, char **argv) {
    int sock;
    struct sockaddr_in addr;

    initialize_openssl();
    SSL_CTX *ctx = create_context();
    configure_context(ctx);

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
        SSL *ssl;

        int client = accept(sock, (struct sockaddr*)&addr, &len);
        if (client < 0) {
            perror("Unable to accept");
            exit(EXIT_FAILURE);
        }

        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, client);

        if (SSL_accept(ssl) <= 0) {
            ERR_print_errors_fp(stderr);
        } else {
            handle_client(ssl);
        }

        SSL_shutdown(ssl);
        SSL_free(ssl);
        close(client);
    }

    close(sock);
    SSL_CTX_free(ctx);
    cleanup_openssl();
    return 0;
}
