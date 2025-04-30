#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <vector>
#include <iterator>

namespace fs = std::filesystem;

// En simpel map af filendelser til Content-Type
const std::unordered_map<std::string, std::string> MIME_TYPES = {
    {".html", "text/html"},
    {".htm",  "text/html"},
    {".css",  "text/css"},
    {".js",   "application/javascript"},
    {".png",  "image/png"},
    {".jpg",  "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif",  "image/gif"},
    {".txt",  "text/plain"},
    // tilføj flere efter behov
};

std::string get_mime_type(const fs::path& p) {
    auto ext = p.extension().string();
    auto it = MIME_TYPES.find(ext);
    return it != MIME_TYPES.end() ? it->second : "application/octet-stream";
}

void handle_connection(int client_fd, const fs::path& www_root) {
    constexpr int BUF_SIZE = 4096;
    char buffer[BUF_SIZE] = {0};
    ssize_t bytes = recv(client_fd, buffer, BUF_SIZE - 1, 0);
    if (bytes <= 0) { close(client_fd); return; }

    // Parse første request-linje: "GET /sti HTTP/1.1"
    std::istringstream req(buffer);
    std::string method, url, version;
    req >> method >> url >> version;

    if (method != "GET") {
        // Kun GET understøttes her
        std::string resp = 
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";
        send(client_fd, resp.c_str(), resp.size(), 0);
        close(client_fd);
        return;
    }

    // Kort sikkerheds-check: undgå "../"
    if (url.find("..") != std::string::npos) {
        std::string resp =
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Length: 0\r\n"
            "Connection: close\r\n\r\n";
        send(client_fd, resp.c_str(), resp.size(), 0);
        close(client_fd);
        return;
    }

    // Kort mapping: "/" → "/index.html"
    if (url == "/") url = "/index.html";

    fs::path file_path = www_root / url.substr(1);  // strip det indledende “/”

    if (!fs::exists(file_path) || !fs::is_regular_file(file_path)) {
        // 404 Not Found
        std::string body = "404 Not Found\n";
        std::ostringstream resp;
        resp << "HTTP/1.1 404 Not Found\r\n"
             << "Content-Type: text/plain\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << body;
        auto str = resp.str();
        send(client_fd, str.c_str(), str.size(), 0);
        close(client_fd);
        return;
    }

    // Læs filindhold
    std::ifstream file(file_path, std::ios::binary);
    std::vector<char> file_data((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());

    // Build HTTP-response
    std::string mime = get_mime_type(file_path);
    std::ostringstream resp;
    resp << "HTTP/1.1 200 OK\r\n"
         << "Content-Type: " << mime << "\r\n"
         << "Content-Length: " << file_data.size() << "\r\n"
         << "Connection: close\r\n"
         << "\r\n";
    auto header = resp.str();

    // Send header + fil
    send(client_fd, header.c_str(), header.size(), 0);
    send(client_fd, file_data.data(), file_data.size(), 0);

    close(client_fd);
}

int main() {
    constexpr int PORT    = 8080;
    constexpr int BACKLOG = 10;
    const fs::path WWW_ROOT = fs::current_path() / "www";

    // 1) Opret server-socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    // 2) Genbrug adresse
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    // 3) Bind til port PORT på alle interfaces
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);
    if (bind(server_fd,
             reinterpret_cast<sockaddr*>(&addr),
             sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    // 4) Start lytning
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    std::cout << "Server kører, serving files from " << WWW_ROOT
              << " på port " << PORT << "\n";

    // 5) Accept-loop
    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd,
                               reinterpret_cast<sockaddr*>(&client_addr),
                               &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        handle_connection(client_fd, WWW_ROOT);
    }
    
    close(server_fd);
    return 0;
}

