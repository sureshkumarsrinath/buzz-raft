#include <string>
#include <vector>
#include <nlohmann/json.hpp>

// Forward declarations
struct LogEntry;
struct AppendEntriesRequest;

//--------------------------------------------------
// Log Entry Structure
//--------------------------------------------------
struct LogEntry {
    uint64_t term;
    std::string data;
    uint64_t client_id;
    uint64_t request_id;

    // Manual equality operator for C++17 compatibility
    bool operator==(const LogEntry& other) const {
        return term == other.term &&
               data == other.data &&
               client_id == other.client_id &&
               request_id == other.request_id;
    }

    std::string serialize() const {
        nlohmann::json j;
        j["term"] = term;
        j["data"] = data;
        j["client_id"] = client_id;
        j["request_id"] = request_id;
        return j.dump();
    }

    static LogEntry deserialize(const std::string& data) {
        auto j = nlohmann::json::parse(data);
        return LogEntry{
            j["term"].get<uint64_t>(),
            j["data"].get<std::string>(),
            j["client_id"].get<uint64_t>(),
            j["request_id"].get<uint64_t>()
        };
    }
};

// Single definition of adl_serializer for LogEntry
namespace nlohmann {
    template<>
    struct adl_serializer<LogEntry> {
        static void to_json(nlohmann::json& j, const LogEntry& entry) {
            j = nlohmann::json{
                {"term", entry.term},
                {"data", entry.data},
                {"client_id", entry.client_id},
                {"request_id", entry.request_id}
            };
        }

        static void from_json(const nlohmann::json& j, LogEntry& entry) {
            j.at("term").get_to(entry.term);
            j.at("data").get_to(entry.data);
            j.at("client_id").get_to(entry.client_id);
            j.at("request_id").get_to(entry.request_id);
        }
    };
}

//--------------------------------------------------
// RequestVote RPC
//--------------------------------------------------
struct RequestVoteRequest {
    int term;
    int candidate_id;
    int last_log_index;
    int last_log_term;

    std::string serialize() const {
        nlohmann::json j;
        j["term"] = term;
        j["candidate_id"] = candidate_id;
        j["last_log_index"] = last_log_index;
        j["last_log_term"] = last_log_term;
        return j.dump();
    }

    static RequestVoteRequest deserialize(const std::string& data) {
        auto j = nlohmann::json::parse(data);
        return RequestVoteRequest{
            j["term"].get<int>(),
            j["candidate_id"].get<int>(),
            j["last_log_index"].get<int>(),
            j["last_log_term"].get<int>()
        };
    }
};

//--------------------------------------------------
// RequestVote Response
//--------------------------------------------------
struct RequestVoteResponse {
    uint64_t term;
    bool vote_granted;

    std::string serialize() const {
        nlohmann::json j;
        j["term"] = term;
        j["vote_granted"] = vote_granted;
        return j.dump();
    }

    static RequestVoteResponse deserialize(const std::string& data) {
        auto j = nlohmann::json::parse(data);
        return RequestVoteResponse{
            j["term"].get<uint64_t>(),
            j["vote_granted"].get<bool>()
        };
    }
};

//--------------------------------------------------
// AppendEntries RPC
//--------------------------------------------------
struct AppendEntriesRequest {
    uint64_t term;
    uint64_t leader_id;
    uint64_t prev_log_index;
    uint64_t prev_log_term;
    std::vector<LogEntry> entries;
    uint64_t leader_commit;

    std::string serialize() const {
        nlohmann::json j;
        j["term"] = term;
        j["leader_id"] = leader_id;
        j["prev_log_index"] = prev_log_index;
        j["prev_log_term"] = prev_log_term;
        j["entries"] = entries;
        j["leader_commit"] = leader_commit;
        return j.dump();
    }

    static AppendEntriesRequest deserialize(const std::string& data) {
        auto j = nlohmann::json::parse(data);
        AppendEntriesRequest req;
        req.term = j["term"].get<uint64_t>();
        req.leader_id = j["leader_id"].get<uint64_t>();
        req.prev_log_index = j["prev_log_index"].get<uint64_t>();
        req.prev_log_term = j["prev_log_term"].get<uint64_t>();
        req.leader_commit = j["leader_commit"].get<uint64_t>();

        for (const auto& entry : j["entries"]) {
            req.entries.push_back(entry.get<LogEntry>());
        }

        return req;
    }
};

//--------------------------------------------------
// AppendEntries Response
//--------------------------------------------------
struct AppendEntriesResponse {
    uint64_t term;
    bool success;
    uint64_t conflict_index;

    std::string serialize() const {
        nlohmann::json j;
        j["term"] = term;
        j["success"] = success;
        j["conflict_index"] = conflict_index;
        return j.dump();
    }

    static AppendEntriesResponse deserialize(const std::string& data) {
        auto j = nlohmann::json::parse(data);
        return AppendEntriesResponse{
            j["term"].get<uint64_t>(),
            j["success"].get<bool>(),
            j["conflict_index"].get<uint64_t>()
        };
    }
};

//--------------------------------------------------
// Client Request
//--------------------------------------------------
struct ClientRequest {
    enum class Type { INSERT, DELETE, UPDATE };
    Type type;
    std::string key;
    std::string value;
    uint64_t client_id;
    uint64_t request_id;

    std::string serialize() const {
        nlohmann::json j;
        j["type"] = type_to_string(type);
        j["key"] = key;
        j["value"] = value;
        j["client_id"] = client_id;
        j["request_id"] = request_id;
        return j.dump();
    }

    static std::string type_to_string(Type t) {
        switch(t) {
            case Type::INSERT: return "INSERT";
            case Type::DELETE: return "DELETE";
            case Type::UPDATE: return "UPDATE";
            default: return "UNKNOWN";
        }
    }

    static ClientRequest deserialize(const std::string& data) {
        auto j = nlohmann::json::parse(data);
        ClientRequest req;
        std::string type_str = j["type"];
        if(type_str == "INSERT") req.type = Type::INSERT;
        else if(type_str == "DELETE") req.type = Type::DELETE;
        else if(type_str == "UPDATE") req.type = Type::UPDATE;
        j.at("key").get_to(req.key);
        j.at("value").get_to(req.value);
        j.at("client_id").get_to(req.client_id);
        j.at("request_id").get_to(req.request_id);
        return req;
    }
};

//--------------------------------------------------
// Client Response
//--------------------------------------------------
struct ClientResponse {
    bool success;
    bool leader_hint;
    uint64_t leader_id;
    std::string error;

    std::string serialize() const {
        nlohmann::json j;
        j["success"] = success;
        j["leader_hint"] = leader_hint;
        j["leader_id"] = leader_id;
        j["error"] = error;
        return j.dump();
    }

    static ClientResponse deserialize(const std::string& data) {
        auto j = nlohmann::json::parse(data);
        return ClientResponse{
            j["success"].get<bool>(),
            j["leader_hint"].get<bool>(),
            j["leader_id"].get<uint64_t>(),
            j["error"].get<std::string>()
        };
    }
};