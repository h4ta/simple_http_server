# simple_https_server

This project is a simple TLS-enabled HTTP server implemented in C using OpenSSL. The server provites a simple login page. The communication content, including the client's name and password, is encrypted. <br>
`test_http_server.c` works equally well as `https_server.c`, but the communication content is not encrypted. Using this program, you can verify that the encryption in `https_server.c`  works properly.   

## Usage
1. Generate a self-signed certificate and private key.
```
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365
```

2. Compile the server program and run.
```
gcc https_server.c -o https_server -lssl -lcrypto
./https_server
```

3. Accecs to `https://localhost:11111` by using web browser.
