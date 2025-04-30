#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sstream>


constexpr int PORT = 8080;
constexpr int BACKLOG = 10;
constexpr int BUF_SIZE = 4096;


int main() {

    // 1) Herunder opretter vi vores server-socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    // 2) Genbrug adressen
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3) Bind til port
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    // 4) Start med at lytte
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        return 1;
    }

    std::cout << "Lytter på port " << PORT << "...\n";

    // 5) Accepter én forbindelse
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("accept");
        close(server_fd);
        return 1;
    }

    // 6) Læs request (op til BUF_SIZE bytes)
    char buffer[BUF_SIZE] = {0};
    ssize_t bytes = recv(client_fd, buffer, BUF_SIZE - 1, 0);
    if (bytes > 0) {
        std::cout << "Modtaget request:\n" << buffer << "\n";
    }

    // 7) Send en simpel HTTP-respons
    std::string body = "Hej fra din C++ HTTP-server!";
    std::ostringstream resp;
    resp << "HTTP/1.1 200 OK\r\n"
         << "Content-Type: text/plain\r\n"
         << "Content-Length: " << body.size() << "\r\n"
         << "Connection: close\r\n"
         << "\r\n"
         << body;
    auto http_response = resp.str();
    send(client_fd, http_response.c_str(), http_response.size(), 0);
    
    // 8) Luk forbindelsen
    close(client_fd);
    close(server_fd);
    return 0;
}