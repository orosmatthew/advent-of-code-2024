#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <sstream>
#include <vector>
#include <iostream>

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

static std::vector<uint64_t> parse_stones(const std::string& data)
{
    std::vector<uint64_t> stones;
    int pos = 0;
    while (true) {
        stones.push_back(parse_int(data, pos));
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

static void blink(std::vector<uint64_t>& stones)
{
    for (size_t i = 0; i < stones.size(); ++i) {
        if (stones[i] == 0) {
            stones[i] = 1;
        }
        else if (const int digits = digits_count(stones[i]); digits % 2 == 0) {
            const uint64_t divisor = pow10[digits / 2];
            const uint64_t first = stones[i] / divisor;
            const uint64_t second = stones[i] % divisor;
            stones[i] = second;
            stones.insert(stones.begin() + static_cast<int64_t>(i), first);
            ++i;
        }
        else {
            stones[i] *= 2024;
        }
    }
}

static uint64_t solve(const std::string& data)
{
    std::vector<uint64_t> stones = parse_stones(data);
    for (int i = 0; i < 25; ++i) {
        std::cout << i << std::endl;
        blink(stones);
    }
    return stones.size();
}

int main()
{
    const std::string data = read_data("./day11-part1/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 100000;
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