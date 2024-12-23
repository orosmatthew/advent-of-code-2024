#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <ranges>
#include <set>
#include <sstream>
#include <utility>
#include <vector>

static std::string read_data(const std::filesystem::path& path)
{
    const std::fstream file { path };
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

using Computer = std::array<char, 2>;
using Connections = std::map<Computer, std::vector<Computer>>;

static Connections parse_connections(const std::string& data)
{
    Connections connections;
    auto insert_connection = [&connections](const Computer& c1, const Computer& c2) {
        if (const auto it = connections.find(c1); it != connections.end()) {
            it->second.push_back(c2);
        }
        else {
            std::vector<Computer> v;
            v.push_back(c2);
            connections[c1] = std::move(v);
        }
        if (const auto it = connections.find(c2); it != connections.end()) {
            it->second.push_back(c1);
        }
        else {
            std::vector<Computer> v;
            v.push_back(c1);
            connections[c2] = std::move(v);
        }
    };
    for (int i = 0; i < data.size(); ++i) {
        const Computer computer1 { data[i], data[i + 1] };
        i += 3; // "xx-"
        const Computer computer2 { data[i], data[i + 1] };
        i += 2; // "xx"
        insert_connection(computer1, computer2);
    }
    return connections;
}

using Network3 = std::array<Computer, 3>;

static void find_network3s( // NOLINT(*-no-recursion)
    const Connections& connections,
    std::set<Network3>& network3s,
    Network3 current = {},
    const size_t current_count = 0)
{
    if (current_count == 3) {
        if (std::ranges::find_if(current, [](const Computer c) { return c[0] == 't'; }) == current.end()) {
            return;
        }
        if (const std::vector<Computer>& first_connections = connections.at(current[0]);
            std::ranges::find(first_connections, current[2]) == first_connections.end()) {
            return;
        }
        std::ranges::sort(current);
        network3s.insert(current);
        return;
    }
    if (current_count == 0) {
        for (const Computer c : connections | std::views::keys) {
            find_network3s(connections, network3s, { c }, 1);
        }
        return;
    }
    for (const Computer c : connections.at(current[current_count - 1])) {
        Network3 next = current;
        next[current_count] = c;
        find_network3s(connections, network3s, next, current_count + 1);
    }
}

static uint64_t solve(const std::string& data)
{
    const Connections connections = parse_connections(data);
    std::set<Network3> network3s;
    find_network3s(connections, network3s);
    return network3s.size();
}

int main()
{
    const std::string data = read_data("./day23-part1/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 1000;
    double time_running_total = 0.0;

    for (int n_run = 0; n_run < n_runs; ++n_run) {
        auto start = std::chrono::high_resolution_clock::now();
        // ReSharper disable once CppDFAUnusedValue
        // ReSharper disable once CppDFAUnreadVariable
        // ReSharper disable once CppDeclaratorNeverUsed
        volatile auto result = solve(data);
        auto end = std::chrono::high_resolution_clock::now();
        time_running_total += std::chrono::duration<double, std::nano>(end - start).count();
    }
    std::printf("Average ns: %d\n", static_cast<int>(std::round(time_running_total / n_runs)));
#else
    std::printf("%llu\n", solve(data));
#endif
}