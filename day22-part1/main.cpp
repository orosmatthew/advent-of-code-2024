#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <ranges>
#include <span>
#include <sstream>
#include <utility>
#include <variant>
#include <vector>

static std::string read_data(const std::filesystem::path& path)
{
    const std::fstream file { path };
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static bool is_digit(const char c)
{
    return c >= '0' && c <= '9';
}

static uint64_t parse_int(const std::string& string, int& pos)
{
    uint64_t result = 0;
    do {
        result = result * 10 + (string[pos] - '0');
        ++pos;
    } while (is_digit(string[pos]) && pos < string.length());
    return result;
}

// ReSharper disable once CppDFAConstantParameter
static uint64_t predict_number_at(const uint64_t initial, const uint64_t index)
{
    uint64_t current = initial;
    auto mix = [&current](const uint64_t num) { current = num ^ current; };
    auto prune = [&current] { current %= 16777216; };
    for (uint64_t i = 0; i < index; ++i) {
        mix(current * 64);
        prune();
        mix(current / 32);
        prune();
        mix(current * 2048);
        prune();
    }
    return current;
}

static uint64_t solve(const std::string& data)
{
    uint64_t sum = 0;
    for (int pos = 0; pos < data.size(); ++pos) {
        const uint64_t initial = parse_int(data, pos);
        sum += predict_number_at(initial, 2000);
    }
    return sum;
}

int main()
{
    const std::string data = read_data("./day22-part1/input.txt");

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