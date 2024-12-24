#include <algorithm>
#include <array>
#include <bitset>
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
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

static std::string read_data(const std::filesystem::path& path)
{
    const std::fstream file { path };
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

using WireName = std::array<char, 3>;

template <>
struct std::hash<WireName> {
    size_t operator()(const WireName& wire_name) const noexcept
    {
        const size_t hash1 = std::hash<int>()(wire_name[0]);
        const size_t hash2 = std::hash<int>()(wire_name[1]);
        const size_t hash3 = std::hash<int>()(wire_name[2]);
        return hash1 ^ hash2 << 1 ^ hash3 << 2;
    }
};

using Wires = std::unordered_map<WireName, std::optional<bool>>;

enum class GateType { and_, or_, xor_ };

struct Gate {
    GateType type;
    WireName input1;
    WireName input2;
    WireName output;
};

static Wires parse_initial_wires(const std::string& data, int& pos)
{
    Wires wires;
    do {
        const WireName name { data[pos], data[pos + 1], data[pos + 2] };
        pos += 5; // "xyz: "
        const bool value = data[pos] == '1';
        pos += 2; // "1\n"
        assert(!wires.contains(name));
        wires[name] = value;
    } while (data[pos] != '\n');
    return wires;
}

static std::vector<Gate> parse_gates(const std::string& data, int& pos, Wires& wires)
{
    std::vector<Gate> gates;
    for (; pos < data.size(); ++pos) {
        const WireName input1 { data[pos], data[pos + 1], data[pos + 2] };
        pos += 4; // "xyz "
        auto type = GateType::and_;
        switch (data[pos]) {
        case 'A':
            type = GateType::and_;
            pos += 4; // "AND "
            break;
        case 'O':
            type = GateType::or_;
            pos += 3; // "OR "
            break;
        case 'X':
            type = GateType::xor_;
            pos += 4; // "XOR "
            break;
        default:
            assert(false);
        }
        const WireName input2 { data[pos], data[pos + 1], data[pos + 2] };
        pos += 7; // "xyz -> "
        const WireName output { data[pos], data[pos + 1], data[pos + 2] };
        pos += 3; // "xyz"
        if (!wires.contains(input1)) {
            wires[input1] = std::nullopt;
        }
        if (!wires.contains(input2)) {
            wires[input2] = std::nullopt;
        }
        if (!wires.contains(output)) {
            wires[output] = std::nullopt;
        }
        gates.push_back({ type, input1, input2, output });
    }
    return gates;
}

static void evaluate_gates(const std::vector<Gate>& gates, Wires& wires)
{
    bool done;
    do {
        done = true;
        // ReSharper disable once CppUseStructuredBinding
        for (const Gate& gate : gates) {
            std::optional<bool>& output = wires.at(gate.output);
            if (output.has_value()) {
                continue;
            }
            const std::optional<bool> input1 = wires.at(gate.input1);
            const std::optional<bool> input2 = wires.at(gate.input2);
            if (!input1.has_value() || !input2.has_value()) {
                done = false;
                continue;
            }
            switch (gate.type) {
            case GateType::and_:
                output = input1.value() && input2.value();
                break;
            case GateType::or_:
                output = input1.value() || input2.value();
                break;
            case GateType::xor_:
                output = (input1.value() && !input2.value()) || (!input1.value() && input2.value());
                break;
            }
        }
    } while (!done);
}

static uint64_t solve(const std::string& data)
{
    int pos = 0;
    Wires wires = parse_initial_wires(data, pos);
    ++pos; // "\n"
    const std::vector<Gate> gates = parse_gates(data, pos, wires);
    evaluate_gates(gates, wires);
    std::vector<WireName> z_wire_names;
    for (const auto name : wires | std::views::keys) {
        if (name[0] == 'z') {
            z_wire_names.push_back(name);
        }
    }
    std::ranges::sort(z_wire_names);
    std::bitset<64> result;
    for (int i = 0; i < z_wire_names.size(); ++i) {
        result[i] = wires.at(z_wire_names[i]).value();
    }
    return result.to_ullong();
}

int main()
{
    const std::string data = read_data("./day24-part1/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 10000;
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