#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
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

static int parse_int(const std::string& string, int& pos)
{
    constexpr std::array powers = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000 };
    std::array<int, 10> digits {};
    int digits_count = 0;
    while (is_digit(string[pos])) {
        assert(digits_count + 1 < digits.size());
        digits[digits_count++] = string[pos] - '0';
        ++pos;
    }
    int sum = 0;
    for (int d = 0; d < digits_count; ++d) {
        const int power = powers[digits_count - d - 1];
        sum += power * digits[d];
    }
    return sum;
}

static void skip_spaces(const std::string& string, int& pos)
{
    while (string[pos] == ' ') {
        ++pos;
    }
}

static int solve(const std::string& data)
{
    static std::vector<int> list1;
    static std::vector<int> list2;
    list1.clear();
    list2.clear();
    for (int i = 0; i < data.length(); ++i) {
        list1.push_back(parse_int(data, i));
        skip_spaces(data, i);
        list2.push_back(parse_int(data, i));
    }
    assert(list1.size() == list2.size());
    std::ranges::sort(list1);
    std::ranges::sort(list2);
    int dist_sum = 0;
    for (int i = 0; i < list1.size(); ++i) {
        dist_sum += std::abs(list1[i] - list2[i]);
    }
    return dist_sum;
}

int main()
{
    const std::string data = read_data("./day1-part1/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 100000;
    double time_running_total = 0.0;

    for (int n_run = 0; n_run < n_runs; ++n_run) {
        auto start = std::chrono::high_resolution_clock::now();
        // ReSharper disable once CppDFAUnusedValue
        // ReSharper disable once CppDFAUnreadVariable
        // ReSharper disable once CppDeclaratorNeverUsed
        volatile int result = solve(data);
        auto end = std::chrono::high_resolution_clock::now();
        time_running_total += std::chrono::duration<double, std::nano>(end - start).count();
    }
    std::printf("Average ns: %d\n", static_cast<int>(std::round(time_running_total / n_runs)));
#else
    std::printf("%d\n", solve(data));
#endif
}