#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>
#include <unordered_map>
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
    int result = 0;
    do {
        result = result * 10 + (string[pos] - '0');
        ++pos;
    } while (is_digit(string[pos]) && pos < string.length());
    return result;
}

static std::unordered_map<uint64_t, uint64_t> parse_stones(const std::string& data)
{
    std::unordered_map<uint64_t, uint64_t> stones;
    int pos = 0;
    while (true) {
        const uint64_t stone = parse_int(data, pos);
        if (auto it = stones.find(stone); it != stones.end()) {
            ++it->second;
        }
        else {
            stones[stone] = 1;
        }
        if (data[pos] == '\n') {
            break;
        }
        ++pos; // " "
    }
    return stones;
}

constexpr std::array pow10 {
    1ULL,
    10ULL,
    100ULL,
    1000ULL,
    10000ULL,
    100000ULL,
    1000000ULL,
    10000000ULL,
    100000000ULL,
    1000000000ULL,
    10000000000ULL,
    100000000000ULL,
    1000000000000ULL,
    10000000000000ULL,
    100000000000000ULL,
    1000000000000000ULL,
    10000000000000000ULL,
    100000000000000000ULL,
    1000000000000000000ULL,
};

static int digits_count(const uint64_t num)
{
    int count = 1;
    for (int i = 1; i < pow10.size(); ++i) {
        if (num >= pow10[i]) {
            ++count;
        }
        else {
            break;
        }
    }
    return count;
}

static void blink(std::unordered_map<uint64_t, uint64_t>& stones)
{
    static std::unordered_map<uint64_t, uint64_t> new_stones;
    new_stones.clear();
    auto add_stone = [](const uint64_t stone, const uint64_t count) {
        if (const auto it = new_stones.find(stone); it != new_stones.end()) {
            it->second += count;
        }
        else {
            new_stones[stone] = count;
        }
    };
    for (auto& [stone, count] : stones) {
        if (stone == 0) {
            add_stone(1, count);
        }
        else if (const int digits = digits_count(stone); digits % 2 == 0) {
            const uint64_t divisor = pow10[digits / 2];
            const uint64_t first = stone / divisor;
            const uint64_t second = stone % divisor;
            add_stone(first, count);
            add_stone(second, count);
        }
        else {
            add_stone(stone * 2024, count);
        }
    }
    std::swap(stones, new_stones);
}

static uint64_t solve(const std::string& data)
{
    std::unordered_map<uint64_t, uint64_t> stones = parse_stones(data);
    for (int i = 0; i < 75; ++i) {
        blink(stones);
    }
    uint64_t stone_count = 0;
    for (const uint64_t count : stones | std::views::values) {
        stone_count += count;
    }
    return stone_count;
}

int main()
{
    const std::string data = read_data("./day11-part2/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 100;
    double time_running_total = 0.0;

    for (int n_run = 0; n_run < n_runs; ++n_run) {
        auto start = std::chrono::high_resolution_clock::now();
        // ReSharper disable once CppDFAUnusedValue
        // ReSharper disable once CppDFAUnreadVariable
        // ReSharper disable once CppDeclaratorNeverUsed
        volatile int64_t result = solve(data);
        auto end = std::chrono::high_resolution_clock::now();
        time_running_total += std::chrono::duration<double, std::nano>(end - start).count();
    }
    std::printf("Average ns: %d\n", static_cast<int>(std::round(time_running_total / n_runs)));
#else
    std::printf("%llu\n", solve(data));
#endif
}