//#include <iostream>
//#include <thread>
//#include <vector>
//#include <map>
//#include <cstring>
//#include <unistd.h>
//#include <arpa/inet.h>
//#include "messages.cpp"  // Include your message definitions
//
//const int NUM_NODES = 3;
//const int BASE_PORT = 5000;
//
//// Network configuration
//struct NodeConfig {
//    int id;
//    int port;
//    sockaddr_in address;
//};
//
//class NetworkManager {
//    int sockfd;
//    std::map<int, NodeConfig> nodes;
//    int current_node_id;
//
//public:
//    NetworkManager(int node_id) : current_node_id(node_id) {
//        // Validate node ID
//        if (node_id < 0 || node_id >= NUM_NODES) {
//            throw std::runtime_error("Invalid node ID");
//        }
//
//        // Create UDP socket
//        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
//        if (sockfd < 0) {
//            perror("socket creation failed");
//            exit(EXIT_FAILURE);
//        }
//
//        // Configure all nodes (0-2)
//        for (int i = 0; i < NUM_NODES; i++) {
//            NodeConfig cfg;
//            cfg.id = i;
//            cfg.port = BASE_PORT + i;
//
//            cfg.address.sin_family = AF_INET;
//            cfg.address.sin_port = htons(cfg.port);
//            inet_pton(AF_INET, "127.0.0.1", &cfg.address.sin_addr);  // Explicit localhost
//
//            nodes[i] = cfg;
//        }
//
//        // Bind to our own port
//        sockaddr_in servaddr{};
//        servaddr.sin_family = AF_INET;
//        servaddr.sin_port = htons(nodes[current_node_id].port);
//        inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
//
//        if (bind(sockfd, (const sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
//            perror("bind failed");
//            close(sockfd);
//            exit(EXIT_FAILURE);
//        }
//
//        // Set timeout for receive
//        struct timeval tv{};
//        tv.tv_sec = 1;
//        tv.tv_usec = 0;
//        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
//    }
//
//    void send_message(int to_node_id, const std::string& data) {
//        const auto& dest = nodes[to_node_id].address;
//        sendto(sockfd, data.c_str(), data.size(), 0,
//               (const sockaddr*)&dest, sizeof(dest));
//    }
//
//    std::string receive_message() {
//        char buffer[4096];
//        sockaddr_in cliaddr{};
//        socklen_t len = sizeof(cliaddr);
//
//        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0,
//                        (sockaddr*)&cliaddr, &len);
//        if (n > 0) return std::string(buffer, n);
//        return "";
//    }
//
//    ~NetworkManager() {
//        close(sockfd);
//    }
//};
//
//class Node {
//    int node_id;
//    NetworkManager network;
//    std::thread server_thread;
//    bool running;
//
//public:
//    Node(int id) : node_id(id), network(id), running(true) {
//        server_thread = std::thread([this] { server_loop(); });
//    }
//
//    void server_loop() {
//        while (running) {
//            std::string data = network.receive_message();
//            if (!data.empty()) {
//                handle_message(data);
//            }
//        }
//    }
//
//    void handle_message(const std::string& data) {
//        try {
//            // Try to parse as RequestVote
//            auto rv = RequestVoteRequest::deserialize(data);
//            std::cout << "Node " << node_id << ": Received RequestVote from "
//                      << rv.candidate_id << std::endl;
//            return;
//        } catch (...) {}
//
//        try {
//            // Try to parse as AppendEntries
//            auto ae = AppendEntriesRequest::deserialize(data);
//            std::cout << "Node " << node_id << ": Received AppendEntries from "
//                      << ae.leader_id << " with " << ae.entries.size()
//                      << " entries" << std::endl;
//            return;
//        } catch (...) {}
//
//        try {
//            // Try to parse as ClientRequest
//            auto cr = ClientRequest::deserialize(data);
//            std::cout << "Node " << node_id << ": Received ClientRequest: "
//                      << cr.key << "=" << cr.value << std::endl;
//            return;
//        } catch (...) {}
//
//        std::cerr << "Unknown message type received" << std::endl;
//    }
//
//    void send_vote_request() {
//        RequestVoteRequest rv;
//        rv.term = 1;
//        rv.candidate_id = node_id;
//        rv.last_log_index = 0;
//        rv.last_log_term = 0;
//
//        for (int i = 0; i < NUM_NODES; i++) {
//            if (i != node_id) {
//                network.send_message(i, rv.serialize());
//            }
//        }
//    }
//
//    void send_client_request(const std::string& key, const std::string& value) {
//        ClientRequest cr;
//        cr.type = ClientRequest::Type::INSERT;
//        cr.key = key;
//        cr.value = value;
//        cr.client_id = 1;
//        cr.request_id = time(nullptr);
//
//        // In real Raft, we would send to leader, here we broadcast
//        for (int i = 0; i < NUM_NODES; i++) {
//            if (i != node_id) {
//                network.send_message(i, cr.serialize());
//            }
//        }
//    }
//
//    ~Node() {
//        running = false;
//        if (server_thread.joinable()) server_thread.join();
//    }
//};
//
//int main(int argc, char* argv[]) {
//    if (argc != 2) {
//        std::cerr << "Usage: " << argv[0] << " <node_id(0-2)>" << std::endl;
//        return 1;
//    }
//
//    int node_id = std::stoi(argv[1]);
//    if (node_id < 0 || node_id > 2) {
//        std::cerr << "Invalid node ID. Must be 0, 1, or 2" << std::endl;
//        return 1;
//    }
//
//    Node node(node_id);
//
//    std::cout << "Node " << node_id << " running at port: . Commands:\n"
//              << "  vote - send vote request\n"
//              << "  insert <key> <value> - send insert request\n"
//              << "  exit - quit\n";
//
//    std::string command;
//    while (true) {
//        std::cout << "> ";
//        std::getline(std::cin, command);
//
//        if (command == "exit") break;
//
//        if (command == "vote") {
//            node.send_vote_request();
//        }
//        else if (command.rfind("insert ", 0) == 0) {
//            size_t space1 = command.find(' ');
//            size_t space2 = command.find(' ', space1+1);
//            if (space1 != std::string::npos && space2 != std::string::npos) {
//                std::string key = command.substr(space1+1, space2-space1-1);
//                std::string value = command.substr(space2+1);
//                node.send_client_request(key, value);
//            }
//        }
//    }
//
//    return 0;
//}


#include <iostream>
#include <thread>
#include <functional>
//#include "messages.cpp"
#include "network_manager.cpp"

const int BASE_PORT = 5000;

class Node {
    int node_id;
    NetworkManager network;
    bool running;

public:
    Node(int id) : node_id(id), network(id, BASE_PORT), running(true) {
        // Register message handlers
        network.set_on_request_vote([this](const RequestVoteRequest& req) {
            handle_vote_request(req);
        });

        network.set_on_vote_reply([this](const RequestVoteResponse& res) {
            handle_vote_response(res);
        });

        network.set_on_append_entries([this](const AppendEntriesRequest& req) {
            handle_append_entries(req);
        });

        network.set_on_client_request([this](const ClientRequest& req) {
            handle_client_request(req);
        });

        network.start();
    }

    void handle_vote_request(const RequestVoteRequest& req) {
        std::cout << "Node " << node_id << ": Received vote request from "
                  << req.candidate_id << " (term " << req.term << ")\n";

        // Create response
        RequestVoteResponse res;
        res.term = req.term + 1;
        res.vote_granted = true;

        // Send response
        network.send_to(req.candidate_id, res);
    }

    void handle_vote_response(const RequestVoteResponse& res) {
        std::cout << "Node " << node_id << ": Received vote response "
                  << (res.vote_granted ? "granted" : "denied")
                  << " for term " << res.term << "\n";
    }

    void handle_append_entries(const AppendEntriesRequest& req) {
        std::cout << "Node " << node_id << ": Received append entries from "
                  << req.leader_id << " with " << req.entries.size()
                  << " entries (term " << req.term << ")\n";
    }

    void handle_client_request(const ClientRequest& req) {
        std::cout << "Node " << node_id << ": Received client request: "
                  << req.key << " = " << req.value << "\n";
    }

    void send_vote_request() {
        RequestVoteRequest req;
        req.term = 1;
        req.candidate_id = node_id;
        req.last_log_index = 0;
        req.last_log_term = 0;

        // Send to all other nodes
        for(int i = 0; i < 3; i++) {
            if(i != node_id) {
                network.send_to(i, req);
            }
        }
    }

    void send_client_request(const std::string& key, const std::string& value) {
        ClientRequest req;
        req.type = ClientRequest::Type::INSERT;
        req.key = key;
        req.value = value;
        req.client_id = 1;
        req.request_id = time(nullptr);

        // Broadcast to all nodes
        for(int i = 0; i < 3; i++) {
            if(i != node_id) {
                network.send_to(i, req);
            }
        }
    }

    ~Node() {
        network.stop();
    }
};

int main(int argc, char* argv[]) {
    if(argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <node_id(0-2)>\n";
        return 1;
    }

    int node_id = std::stoi(argv[1]);
    if(node_id < 0 || node_id > 2) {
        std::cerr << "Invalid node ID. Must be 0-2\n";
        return 1;
    }

    Node node(node_id);

    std::cout << "Node " << node_id << " running. Commands:\n"
              << "1. vote - Send vote request\n"
              << "2. insert <key> <value> - Send data\n"
              << "3. exit - Quit\n";

    std::string command;
    while(true) {
        std::cout << "> ";
        std::getline(std::cin, command);

        if(command == "exit") break;

        if(command == "vote") {
            node.send_vote_request();
        }
        else if(command.find("insert ") == 0) {
            size_t space1 = command.find(' ');
            size_t space2 = command.find(' ', space1 + 1);

            if(space1 != std::string::npos && space2 != std::string::npos) {
                std::string key = command.substr(space1 + 1, space2 - space1 - 1);
                std::string value = command.substr(space2 + 1);
                node.send_client_request(key, value);
            }
        }
    }

    return 0;
}