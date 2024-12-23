#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
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

using Network = std::vector<Computer>;

static void find_largest_network( // NOLINT(*-no-recursion)
    const Connections& connections,
    std::optional<Network>& largest_network,
    std::set<Network>& checked,
    Network current = {})
{
    if (checked.contains(current)) {
        return;
    }
    checked.insert(current);
    if (!largest_network.has_value() || largest_network.value().size() < current.size()) {
        largest_network = current;
    }
    if (current.empty()) {
        for (const Computer c : connections | std::views::keys) {
            find_largest_network(connections, largest_network, checked, { c });
        }
        return;
    }
    for (const Computer c : connections.at(current[current.size() - 1])) {
        if (std::ranges::find(current, c) != current.end()) {
            continue;
        }
        bool valid = true;
        for (const Computer& network_computer : current) {
            if (const std::vector<Computer>& connected = connections.at(network_computer);
                std::ranges::find(connected, c) == connected.end()) {
                valid = false;
                break;
            }
        }
        if (!valid) {
            continue;
        }
        Network next = current;
        next.push_back(c);
        std::ranges::sort(next);
        find_largest_network(connections, largest_network, checked, std::move(next));
    }
}

static std::string solve(const std::string& data)
{
    const Connections connections = parse_connections(data);
    std::optional<Network> largest_network_opt;
    std::set<Network> checked;
    find_largest_network(connections, largest_network_opt, checked);
    Network largest_network = largest_network_opt.value();
    std::ranges::sort(largest_network);
    std::string str;
    for (const Computer& c : largest_network) {
        str.push_back(c[0]);
        str.push_back(c[1]);
        str.push_back(',');
    }
    str.pop_back();
    return str;
}

int main()
{
    const std::string data = read_data("./day23-part2/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 10;
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
    std::printf("%s\n", solve(data).c_str());
#endif
}