#include <nlohmann/json.hpp>
#include <iostream>

int main() {
    nlohmann::json j;
    j["message"] = "Hello from macOS!";
    std::cout << j.dump(4) << std::endl;
    return 0;
}