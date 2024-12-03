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

static std::optional<int> parse_int_opt(const std::string& string, int& pos)
{
    if (!is_digit(string[pos])) {
        return std::nullopt;
    }
    int result = 0;
    do {
        result = result * 10 + (string[pos] - '0');
        ++pos;
    } while (is_digit(string[pos]) && pos < string.length());
    return result;
}

static bool substr_equals_at(const std::string& string, const std::string& value, const int pos)
{
    if (pos + value.length() >= string.length()) {
        return false;
    }
    for (int i = 0; i < value.length(); ++i) {
        if (string[pos + i] != value[i]) {
            return false;
        }
    }
    return true;
}

static std::optional<int> parse_mul(const std::string& string, int& pos)
{
    if (pos + 8 >= string.length()) {
        return std::nullopt;
    }
    if (!substr_equals_at(string, "mul(", pos)) {
        return std::nullopt;
    }
    pos += 4;
    const std::optional<int> value1 = parse_int_opt(string, pos);
    if (!value1.has_value() || pos >= string.length() || string[pos] != ',') {
        return std::nullopt;
    }
    ++pos;
    const std::optional<int> value2 = parse_int_opt(string, pos);
    if (!value2.has_value() || pos >= string.length() || string[pos] != ')') {
        return std::nullopt;
    }
    ++pos;
    return value1.value() * value2.value();
}

static bool parse_do(const std::string& string, int& pos)
{
    if (const std::string key = "do()"; substr_equals_at(string, key, pos)) {
        pos += static_cast<int>(key.length());
        return true;
    }
    return false;
}

static bool parse_dont(const std::string& string, int& pos)
{
    if (const std::string key = "don't()"; substr_equals_at(string, key, pos)) {
        pos += static_cast<int>(key.length());
        return true;
    }
    return false;
}

static int solve(const std::string& data)
{
    int total = 0;
    bool enabled = true;
    for (int pos = 0; pos < data.length(); ++pos) {
        if (!enabled && parse_do(data, pos)) {
            enabled = true;
            --pos;
        }
        else if (enabled) {
            if (parse_dont(data, pos)) {
                enabled = false;
                --pos;
            }
            else if (const std::optional<int> result = parse_mul(data, pos); result.has_value()) {
                total += result.value();
                --pos;
            }
        }
    }
    return total;
}

int main()
{
    const std::string data = read_data("./day3-part2/input.txt");

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