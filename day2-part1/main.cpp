#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

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
    int result = 0;
    while (is_digit(string[pos])) {
        result = result * 10 + (string[pos] - '0');
        ++pos;
    }
    return result;
}

static void skip_line(const std::string& data, int& pos)
{
    while (data[pos] != '\n') {
        ++pos;
    }
}

static bool parse_line(const std::string& data, int& pos)
{
    int prev = parse_int(data, pos);
    ++pos;
    int current = parse_int(data, pos);
    const bool increasing = current > prev;
    while (true) {
        if (const bool correct_dir = increasing ? current > prev : current < prev;
            !correct_dir || prev == current || abs(prev - current) > 3) {
            skip_line(data, pos);
            return false;
        }
        if (data[pos] == '\n') {
            return true;
        }
        ++pos;
        prev = current;
        current = parse_int(data, pos);
    }
}

static int solve(const std::string& data)
{
    int safe_count = 0;
    for (int i = 0; i < data.length(); ++i) {
        if (parse_line(data, i)) {
            ++safe_count;
        }
    }
    return safe_count;
}

int main()
{
    const std::string data = read_data("./day2-part1/input.txt");

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