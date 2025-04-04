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
    const int base_port;

    // Updated handlers with sender_id parameter
    std::function<void(int, const RequestVoteRequest&)> vote_request_handler;
    std::function<void(int, const RequestVoteResponse&)> vote_response_handler;
    std::function<void(int, const AppendEntriesRequest&)> append_entries_handler;
    std::function<void(int, const AppendEntriesResponse&)> append_response_handler;
    std::function<void(int, const ClientRequest&)> client_request_handler;
    std::function<void(int, const ClientResponse&)> client_response_handler;

public:
    struct NodeConfig {
        sockaddr_in address;
        int id;
    };

    std::unordered_map<int, NodeConfig> nodes;

    NetworkManager(int node_id, int base_port = 5000) : 
        node_id(node_id), base_port(base_port), running(false) {
        
        if (node_id < 0 || node_id > 2) {
            throw std::runtime_error("Invalid node ID (0-2 allowed)");
        }

        // Create and configure UDP socket
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Socket creation failed");
        }

        // Configure all nodes
        for (int i = 0; i < 3; i++) {
            NodeConfig cfg;
            cfg.id = i;
            cfg.address.sin_family = AF_INET;
            cfg.address.sin_port = htons(base_port + i);
            inet_pton(AF_INET, "127.0.0.1", &cfg.address.sin_addr);
            nodes[i] = cfg;
        }

        // Bind to configured port
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

    // Updated message sending with better logging
    void send_to(int node_id, const RequestVoteRequest& msg) {
        std::string data = "VTEREQ" + msg.serialize();
        send_raw(node_id, data);
        std::cout << "Node " << this->node_id << " -> Node " << node_id 
                  << " | VoteRequest: term=" << msg.term << "\n";
    }

    void send_to(int node_id, const RequestVoteResponse& msg) {
        std::string data = "VTERES" + msg.serialize();
        send_raw(node_id, data);
        std::cout << "Node " << this->node_id << " -> Node " << node_id 
                  << " | VoteResponse: granted=" << msg.vote_granted << "\n";
    }

    void send_to(int node_id, const AppendEntriesRequest& msg) {
        std::string data = "APPREQ" + msg.serialize();
        send_raw(node_id, data);
        std::cout << "Node " << this->node_id << " -> Node " << node_id 
                  << " | AppendEntries: entries=" << msg.entries.size() << "\n";
    }

    void send_to(int node_id, const AppendEntriesResponse& msg) {
        std::string data = "APPRES" + msg.serialize();
        send_raw(node_id, data);
        std::cout << "Node " << this->node_id << " -> Node " << node_id 
                  << " | AppendResponse: success=" << msg.success << "\n";
    }

    void send_to(int node_id, const ClientRequest& msg) {
        std::string data = "CLIREQ" + msg.serialize();
        send_raw(node_id, data);
        std::cout << "Node " << this->node_id << " -> Node " << node_id 
                  << " | ClientRequest: " << msg.key << "=" << msg.value << "\n";
    }

    void send_to(int node_id, const ClientResponse& msg) {
        std::string data = "CLIRES" + msg.serialize();
        send_raw(node_id, data);
        std::cout << "Node " << this->node_id << " -> Node " << node_id 
                  << " | ClientResponse: " << (msg.success ? "OK" : "ERROR") << "\n";
    }

    // Updated handler registration with sender_id parameter
    void set_on_request_vote(std::function<void(int, const RequestVoteRequest&)> handler) {
        vote_request_handler = handler;
    }

    void set_on_vote_reply(std::function<void(int, const RequestVoteResponse&)> handler) {
        vote_response_handler = handler;
    }

    void set_on_append_entries(std::function<void(int, const AppendEntriesRequest&)> handler) {
        append_entries_handler = handler;
    }

    void set_on_append_reply(std::function<void(int, const AppendEntriesResponse&)> handler) {
        append_response_handler = handler;
    }

    void set_on_client_request(std::function<void(int, const ClientRequest&)> handler) {
        client_request_handler = handler;
    }

    void set_on_client_response(std::function<void(int, const ClientResponse&)> handler) {
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

            // Extract sender information
            int sender_port = ntohs(cliaddr.sin_port);
            int sender_id = sender_port - base_port;

            if (sender_id < 0 || sender_id > 2) {
                std::cerr << "Received message from invalid node: " << sender_port << "\n";
                continue;
            }

            std::string data(buffer, n);
            if (data.size() < 6) continue;

            std::string header = data.substr(0, 6);
            std::string payload = data.substr(6);

            try {
                if (header == "VTEREQ") {
                    auto msg = RequestVoteRequest::deserialize(payload);
                    std::cout << "Node " << node_id << " <- Node " << sender_id 
                              << " | VoteRequest: term=" << msg.term << "\n";
                    if (vote_request_handler) vote_request_handler(sender_id, msg);
                }
                else if (header == "VTERES") {
                    auto msg = RequestVoteResponse::deserialize(payload);
                    std::cout << "Node " << node_id << " <- Node " << sender_id 
                              << " | VoteResponse: granted=" << msg.vote_granted << "\n";
                    if (vote_response_handler) vote_response_handler(sender_id, msg);
                }
                else if (header == "APPREQ") {
                    auto msg = AppendEntriesRequest::deserialize(payload);
                    std::cout << "Node " << node_id << " <- Node " << sender_id 
                              << " | AppendEntries: entries=" << msg.entries.size() << "\n";
                    if (append_entries_handler) append_entries_handler(sender_id, msg);
                }
                else if (header == "APPRES") {
                    auto msg = AppendEntriesResponse::deserialize(payload);
                    std::cout << "Node " << node_id << " <- Node " << sender_id 
                              << " | AppendResponse: success=" << msg.success << "\n";
                    if (append_response_handler) append_response_handler(sender_id, msg);
                }
                else if (header == "CLIREQ") {
                    auto msg = ClientRequest::deserialize(payload);
                    std::cout << "Node " << node_id << " <- Node " << sender_id 
                              << " | ClientRequest: " << msg.key << "=" << msg.value << "\n";
                    if (client_request_handler) client_request_handler(sender_id, msg);
                }
                else if (header == "CLIRES") {
                    auto msg = ClientResponse::deserialize(payload);
                    std::cout << "Node " << node_id << " <- Node " << sender_id 
                              << " | ClientResponse: " << (msg.success ? "OK" : "ERROR") << "\n";
                    if (client_response_handler) client_response_handler(sender_id, msg);
                }
            } catch (const std::exception& e) {
                std::cerr << "Message processing error: " << e.what() << "\n";
            }
        }
    }
};