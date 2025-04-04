#include <iostream>
#include <thread>
#include <functional>
#include "network_manager.cpp"

class Node {
    int node_id;
    NetworkManager network;

public:
    Node(int id) : node_id(id), network(id, 5000) {
        // Register message handlers with sender IDs
        network.set_on_request_vote([this](int sender_id, const RequestVoteRequest& req) {
            handle_vote_request(sender_id, req);
        });

        network.set_on_vote_reply([this](int sender_id, const RequestVoteResponse& res) {
            handle_vote_response(sender_id, res);
        });

        network.set_on_append_entries([this](int sender_id, const AppendEntriesRequest& req) {
            handle_append_entries(sender_id, req);
        });

        network.set_on_client_request([this](int sender_id, const ClientRequest& req) {
            handle_client_request(sender_id, req);
        });

        network.start();
    }

    void handle_vote_request(int sender_id, const RequestVoteRequest& req) {
        std::cout << "[Node " << node_id << "] Received vote request from node "
                  << sender_id << " (term " << req.term << ")\n";

        RequestVoteResponse res;
        res.term = req.term + 1;  // Simplified term handling
        res.vote_granted = true;

        // Send response back to the requesting node
        network.send_to(sender_id, res);
    }

    void handle_vote_response(int sender_id, const RequestVoteResponse& res) {
        std::cout << "[Node " << node_id << "] Received vote response from node "
                  << sender_id << ": " << (res.vote_granted ? "GRANTED" : "DENIED")
                  << " for term " << res.term << "\n";
    }

    void handle_append_entries(int sender_id, const AppendEntriesRequest& req) {
        std::cout << "[Node " << node_id << "] Received append entries from node "
                  << sender_id << " (term " << req.term << ") with "
                  << req.entries.size() << " entries\n";

        // Process entries and send response
        AppendEntriesResponse res;
        res.term = req.term;
        res.success = true;
        network.send_to(sender_id, res);
    }

    void handle_client_request(int sender_id, const ClientRequest& req) {
        std::cout << "[Node " << node_id << "] Received client request from node "
                  << sender_id << ": " << req.key << " = " << req.value << "\n";

        // Process client request and send response
        ClientResponse res;
        res.success = true;
        res.leader_hint = false;
        network.send_to(sender_id, res);
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
                std::cout << "[Node " << node_id << "] Sending vote request to node " << i << "\n";
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
                std::cout << "[Node " << node_id << "] Sending client request to node " << i << "\n";
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

    std::cout << "\n=== Node " << node_id << " Operational ===\n"
              << "Commands:\n"
              << "1. vote    - Request votes from other nodes\n"
              << "2. insert <key> <value> - Store key-value pair\n"
              << "3. exit    - Shutdown node\n\n";

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
            else {
                std::cerr << "Invalid format. Use: insert <key> <value>\n";
            }
        }
        else {
            std::cerr << "Unknown command\n";
        }
    }

    return 0;
}