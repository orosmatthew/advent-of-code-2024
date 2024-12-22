#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
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
    int result = 0;
    while (is_digit(string[pos])) {
        result = result * 10 + (string[pos] - '0');
        ++pos;
    }
    return result;
}

static const std::vector<int>& parse_line(const std::string& data, int& pos)
{
    static std::vector<int> values;
    values.clear();
    while (true) {
        values.push_back(parse_int(data, pos));
        if (data[pos] == '\n') {
            break;
        }
        ++pos;
    }
    return values;
}

static bool validate_report(const std::vector<int>& values)
{
    const bool increasing = values[1] > values[0];
    for (int i = 1; i < values.size(); ++i) {
        const int prev = values[i - 1];
        const int current = values[i];
        if (const bool correct_dir = increasing ? current > prev : current < prev;
            !correct_dir || prev == current || abs(prev - current) > 3) {
            return false;
        }
    }
    return true;
}

static bool validate_report_with_tolerance(const std::vector<int>& values)
{
    static std::vector<int> values_copy;
    for (int i = 0; i < values.size(); ++i) {
        values_copy = values;
        values_copy.erase(values_copy.begin() + i);
        if (validate_report(values_copy)) {
            return true;
        }
    }
    return false;
}

static int solve(const std::string& data)
{
    int safe_count = 0;
    for (int i = 0; i < data.length(); ++i) {
        if (const std::vector<int>& values = parse_line(data, i); validate_report_with_tolerance(values)) {
            ++safe_count;
        }
    }
    return safe_count;
}

int main()
{
    const std::string data = read_data("./day02-part1/input.txt");

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