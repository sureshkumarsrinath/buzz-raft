#include <iostream>
#include <thread>
#include <unordered_map>
#include <functional>
#include <arpa/inet.h>
#include <unistd.h>
#include "messages.cpp"

class NetworkManager {
    int sockfd;
    int node_id;
    bool running;
    std::thread receiver_thread;

    // Message handlers
    std::function<void(const RequestVoteRequest&)> vote_request_handler;
    std::function<void(const RequestVoteResponse&)> vote_response_handler;
    std::function<void(const AppendEntriesRequest&)> append_entries_handler;
    std::function<void(const AppendEntriesResponse&)> append_response_handler;
    std::function<void(const ClientRequest&)> client_request_handler;
    std::function<void(const ClientResponse&)> client_response_handler;

public:
    struct NodeConfig {
        sockaddr_in address;
        int id;
    };

    std::unordered_map<int, NodeConfig> nodes;

    NetworkManager(int node_id, int base_port = 5000) : node_id(node_id), running(false) {
        // Create UDP socket
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Socket creation failed");
        }

        // Configure nodes (0-2)
        for (int i = 0; i < 3; i++) {
            NodeConfig cfg;
            cfg.id = i;
            cfg.address.sin_family = AF_INET;
            cfg.address.sin_port = htons(base_port + i);
            inet_pton(AF_INET, "127.0.0.1", &cfg.address.sin_addr);
            nodes[i] = cfg;
        }

        // Bind to our own port
        sockaddr_in servaddr{};
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = nodes[node_id].address.sin_port;
        servaddr.sin_addr.s_addr = nodes[node_id].address.sin_addr.s_addr;

        if (bind(sockfd, (sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
            close(sockfd);
            throw std::runtime_error("Bind failed");
        }

        // Set receive timeout
        timeval tv{.tv_sec = 1, .tv_usec = 0};
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    }

    ~NetworkManager() {
        stop();
        close(sockfd);
    }

    void start() {
        running = true;
        receiver_thread = std::thread([this] { receiver_loop(); });
    }

    void stop() {
        running = false;
        if (receiver_thread.joinable()) {
            receiver_thread.join();
        }
    }

    // Message sending methods
    void send_to(int node_id, const RequestVoteRequest& msg) {
        std::string data = "VTEREQ" + msg.serialize();
        send_raw(node_id, data);
        std::cout << "Node " << node_id << ": Sent vote request: " << data << "\n";
    }

    void send_to(int node_id, const RequestVoteResponse& msg) {
        std::string data = "VTERES" + msg.serialize();
        send_raw(node_id, data);
        std::cout << "Node " << node_id << ": Sent vote response: " << data << "\n";
    }

    void send_to(int node_id, const AppendEntriesRequest& msg) {
        std::string data = "APPREQ" + msg.serialize();
        send_raw(node_id, data);
        std::cout << "Node " << node_id << ": Sent append request: " << data << "\n";
    }

    void send_to(int node_id, const AppendEntriesResponse& msg) {
        std::string data = "APPRES" + msg.serialize();
        send_raw(node_id, data);
        std::cout << "Node " << node_id << ": Sent append response: " << data << "\n";
    }

    void send_to(int node_id, const ClientRequest& msg) {
        std::string data = "CLIREQ" + msg.serialize();
        send_raw(node_id, data);
        std::cout << "Node " << node_id << ": Sent client request: " << data << "\n";
    }

    void send_to(int node_id, const ClientResponse& msg) {
        std::string data = "CLIRES" + msg.serialize();
        send_raw(node_id, data);
        std::cout << "Node " << node_id << ": Sent client request: " << data << "\n";
    }

    // Handler registration
    void set_on_request_vote(std::function<void(const RequestVoteRequest&)> handler) {
        vote_request_handler = handler;
    }

    void set_on_vote_reply(std::function<void(const RequestVoteResponse&)> handler) {
        vote_response_handler = handler;
    }

    void set_on_append_entries(std::function<void(const AppendEntriesRequest&)> handler) {
        append_entries_handler = handler;
    }

    void set_on_append_reply(std::function<void(const AppendEntriesResponse&)> handler) {
        append_response_handler = handler;
    }

    void set_on_client_request(std::function<void(const ClientRequest&)> handler) {
        client_request_handler = handler;
    }

    void set_on_client_response(std::function<void(const ClientResponse&)> handler) {
        client_response_handler = handler;
    }

private:
    void send_raw(int node_id, const std::string& data) {
        const auto& dest = nodes.at(node_id).address;
        sendto(sockfd, data.c_str(), data.size(), 0,
              (sockaddr*)&dest, sizeof(dest));
    }

    void receiver_loop() {
        char buffer[4096];
        sockaddr_in cliaddr;
        socklen_t len = sizeof(cliaddr);

        while (running) {
            ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                (sockaddr*)&cliaddr, &len);
            if (n <= 0) continue;

            std::string data(buffer, n);
            if (data.size() < 6) continue;  // Minimum header size

            std::string header = data.substr(0, 6);
            std::string payload = data.substr(6);

            try {
                if (header == "VTEREQ") {
                    std::cout << "Node " << node_id << ": Received vote request: " << payload << "\n";
                    if (vote_request_handler) vote_request_handler(RequestVoteRequest::deserialize(payload));
                }
                else if (header == "VTERES") {
                    std::cout << "Node " << node_id << ": Received vote response: " << payload << "\n";
                    if (vote_response_handler) vote_response_handler(RequestVoteResponse::deserialize(payload));
                }
                else if (header == "APPREQ") {
                    std::cout << "Node " << node_id << ": Received append request: " << payload << "\n";
                    if (append_entries_handler) append_entries_handler(AppendEntriesRequest::deserialize(payload));
                }
                else if (header == "APPRES") {
                    std::cout << "Node " << node_id << ": Received append response: " << payload << "\n";
                    if (append_response_handler) append_response_handler(AppendEntriesResponse::deserialize(payload));
                }
                else if (header == "CLIREQ") {
                    std::cout << "Node " << node_id << ": Received client request: " << payload << "\n";
                    if (client_request_handler) client_request_handler(ClientRequest::deserialize(payload));
                }
                else if (header == "CLIRES") {
                    std::cout << "Node " << node_id << ": Received client response: " << payload << "\n";
                    if (client_response_handler) client_response_handler(ClientResponse::deserialize(payload));
                }
            } catch (const std::exception& e) {
                std::cerr << "Failed to process message: " << e.what() << std::endl;
            }
        }
    }
};