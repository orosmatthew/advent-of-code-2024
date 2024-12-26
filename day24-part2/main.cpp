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
        std::array inputs { input1, input2 };
        std::ranges::sort(inputs);
        gates.push_back({ type, inputs[0], inputs[1], output });
    }
    return gates;
}

struct InputOutput {
    std::vector<WireName> xs;
    std::vector<WireName> ys;
    std::vector<WireName> zs;
};

static std::optional<const Gate*> find_gate(
    const std::vector<Gate>& gates, const GateType type, const WireName& input1, const WireName& input2)
{
    std::array inputs { input1, input2 };
    std::ranges::sort(inputs);
    const auto it = std::ranges::find_if(gates, [type, &inputs](const Gate& g) {
        return g.type == type && g.input1 == inputs[0] && g.input2 == inputs[1];
    });
    if (it == gates.end()) {
        return std::nullopt;
    }
    return &*it;
}

struct VerifyResult {
    bool valid {};
    std::array<WireName, 2> check_wires {};
    int check_wires_count {};
    std::optional<WireName> carry;
};

using Swaps = std::array<std::array<WireName, 2>, 4>;

static VerifyResult verify_bit(
    const int bit,
    const InputOutput& io,
    const std::vector<Gate>& gates,
    const std::optional<WireName>& carry,
    const Swaps& swaps,
    const int swaps_count)
{
    auto swapped_output = [&swaps, swaps_count](const WireName& name) {
        for (int i = 0; i < swaps_count; ++i) {
            if (swaps[i][0] == name) {
                return swaps[i][1];
            }
            if (swaps[i][1] == name) {
                return swaps[i][0];
            }
        }
        return name;
    };
    const WireName& x = io.xs[bit];
    const WireName& y = io.ys[bit];
    const WireName& z = io.zs[bit];
    if (bit == 0) {
        const std::optional<const Gate*> and_ = find_gate(gates, GateType::xor_, x, y);
        assert(and_.has_value());
        const bool valid = swapped_output(and_.value()->output) == z;
        return { .valid = valid,
                 .check_wires = !valid ? std::array<WireName, 2> { z } : std::array<WireName, 2> {},
                 .check_wires_count = !valid ? 1 : 0,
                 .carry = find_gate(gates, GateType::and_, x, y).value()->output };
    }
    assert(carry.has_value());
    const std::optional<const Gate*> xor_1 = find_gate(gates, GateType::xor_, x, y);
    assert(xor_1.has_value());
    const std::optional<const Gate*> xor_2
        = find_gate(gates, GateType::xor_, swapped_output(xor_1.value()->output), swapped_output(carry.value()));
    if (!xor_2.has_value()) {
        return { .valid = false, .check_wires = { xor_1.value()->output, carry.value() }, .check_wires_count = 2 };
    }
    if (swapped_output(xor_2.value()->output) != z) {
        return { .valid = false, .check_wires = { z }, .check_wires_count = 1 };
    }
    const std::optional<const Gate*> and_1 = find_gate(gates, GateType::and_, x, y);
    assert(and_1.has_value());
    const std::optional<const Gate*> and_2
        = find_gate(gates, GateType::and_, swapped_output(carry.value()), swapped_output(xor_1.value()->output));
    if (!and_2.has_value()) {
        return { .valid = false, .check_wires = { carry.value(), xor_1.value()->output }, .check_wires_count = 2 };
    }
    return {
        .valid = true,
        .check_wires = {},
        .check_wires_count = 0,
        .carry
        = find_gate(gates, GateType::or_, swapped_output(and_1.value()->output), swapped_output(and_2.value()->output))
              .value()
              ->output
    };
}

static Swaps swaps_to_fix(const std::vector<Gate>& gates, const InputOutput& io)
{
    std::optional<WireName> carry;
    Swaps swaps {};
    int swaps_count = 0;
    const int bits = static_cast<int>(io.xs.size());
    for (int i = 0; i < bits; ++i) {
        // ReSharper disable once CppUseStructuredBinding
        if (const VerifyResult result = verify_bit(i, io, gates, carry, swaps, swaps_count); !result.valid) {
            for (int j = 0; j < result.check_wires_count; ++j) {
                for (const Gate& gate : gates) {
                    Swaps new_swaps = swaps;
                    new_swaps[swaps_count] = { result.check_wires[j], gate.output };
                    if (const VerifyResult fixed_result = verify_bit(i, io, gates, carry, new_swaps, swaps_count + 1);
                        fixed_result.valid) {
                        swaps = new_swaps;
                        ++swaps_count;
                        carry = fixed_result.carry.value();
                        goto fixed;
                    }
                }
            }
        fixed:;
        }
        else {
            carry = result.carry;
        }
    }
    assert(swaps_count == 4);
    return swaps;
}

static std::string solve(const std::string& data)
{
    int pos = 0;
    Wires wires = parse_initial_wires(data, pos);
    ++pos; // "\n"
    const std::vector<Gate> gates = parse_gates(data, pos, wires);
    std::vector<WireName> x_wire_names;
    std::vector<WireName> y_wire_names;
    std::vector<WireName> z_wire_names;
    for (const auto name : wires | std::views::keys) {
        switch (name[0]) {
        case 'x':
            x_wire_names.push_back(name);
            break;
        case 'y':
            y_wire_names.push_back(name);
            break;
        case 'z':
            z_wire_names.push_back(name);
            break;
        default:;
        }
    }
    std::ranges::sort(x_wire_names);
    std::ranges::sort(y_wire_names);
    std::ranges::sort(z_wire_names);
    assert(x_wire_names.size() == y_wire_names.size() && z_wire_names.size() == x_wire_names.size() + 1);
    const InputOutput io {
        .xs = std::move(x_wire_names), .ys = std::move(y_wire_names), .zs = std::move(z_wire_names)
    };
    const Swaps swaps = swaps_to_fix(gates, io);
    std::vector<WireName> swapped_names;
    for (const std::array<WireName, 2> swap : swaps) {
        swapped_names.push_back(swap[0]);
        swapped_names.push_back(swap[1]);
    }
    std::ranges::sort(swapped_names);
    std::string output;
    for (const WireName name : swapped_names) {
        output.append(name.begin(), name.end());
        output.push_back(',');
    }
    output.pop_back();
    return output;
}

int main()
{
    const std::string data = read_data("./day24-part2/input.txt");

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
    std::printf("%s\n", solve(data).c_str());
#endif
}