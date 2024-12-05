#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
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

std::unordered_map<int, std::vector<int>> parse_rules(const std::string& data, int& pos)
{
    std::unordered_map<int, std::vector<int>> rules;
    while (true) {
        std::optional<int> page1 = parse_int_opt(data, pos);
        if (!page1.has_value()) {
            return rules;
        }
        ++pos; // |
        std::optional<int> page2 = parse_int_opt(data, pos);
        ++pos; // \n
        if (auto it = rules.find(page1.value()); it != rules.end()) {
            it->second.push_back(page2.value());
        }
        else {
            std::vector<int> v;
            v.push_back(page2.value());
            rules[page1.value()] = std::move(v);
        }
    }
}

const std::vector<int>& parse_update(const std::string& data, int& pos)
{
    static std::vector<int> update;
    update.clear();
    while (true) {
        std::optional<int> page = parse_int_opt(data, pos);
        update.push_back(page.value());
        if (data[pos] == '\n') {
            ++pos; // \n
            return update;
        }
        ++pos; // ,
    }
}

static bool update_valid(const std::unordered_map<int, std::vector<int>>& rules, const std::vector<int>& update)
{
    for (int i = 0; i < update.size() - 1; ++i) {
        const int page = update[i];
        auto it = rules.find(page);
        if (it == rules.end()) {
            return false;
        }
        const std::vector<int>& after = it->second;
        for (int j = i + 1; j < update.size(); ++j) {
            if (std::ranges::find(after, update[j]) == after.end()) {
                return false;
            }
        }
    }
    return true;
}

static const std::vector<int>& fix_update(
    const std::unordered_map<int, std::vector<int>>& rules, const std::vector<int>& update)
{
    static std::vector<int> fixed_update;
    fixed_update.clear();
    fixed_update.resize(update.size());
    for (int i = 0; i < update.size(); ++i) {
        int count = 0;
        for (int j = 0; j < update.size(); ++j) {
            if (i == j) {
                continue;
            }
            if (auto it = rules.find(update[i]); it != rules.end()) {
                if (const std::vector<int>& after = rules.at(update[i]);
                    std::ranges::find(after, update[j]) != after.end()) {
                    ++count;
                }
            }
        }
        fixed_update[update.size() - count - 1] = update[i];
    }
    return fixed_update;
}

static int solve(const std::string& data)
{
    int pos = 0;
    const std::unordered_map<int, std::vector<int>> rules = parse_rules(data, pos);
    ++pos; // \n
    int result = 0;
    while (pos < data.length()) {
        if (const std::vector<int>& update = parse_update(data, pos); !update_valid(rules, update)) {
            const std::vector<int>& fixed_update = fix_update(rules, update);
            const int middle = fixed_update[fixed_update.size() / 2];
            result += middle;
        }
    }
    return result;
}

int main()
{
    const std::string data = read_data("./day5-part2/input.txt");

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