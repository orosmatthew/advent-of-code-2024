#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <ranges>
#include <span>
#include <sstream>
#include <unordered_map>
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

static int64_t parse_int(const std::string& string, int& pos)
{
    int64_t result = 0;
    do {
        result = result * 10 + (string[pos] - '0');
        ++pos;
    } while (is_digit(string[pos]) && pos < string.length());
    return result;
}

using ChangesPrice = std::unordered_map<uint32_t, int64_t>;

static uint32_t pack_changes(const std::array<int8_t, 4>& changes)
{
    uint32_t result = 0;
    for (const int8_t change : changes) {
        result <<= 8;
        result |= static_cast<uint8_t>(change);
    }
    return result;
}

// ReSharper disable once CppDFAConstantParameter
static ChangesPrice predict_prices(const int64_t initial_secret, const uint64_t count)
{
    ChangesPrice changes_price;
    int64_t prev_price = initial_secret % 10;
    int64_t current = initial_secret;
    auto mix = [&current](const int64_t num) { current = num ^ current; };
    auto prune = [&current] { current %= 16777216; };
    std::array<int8_t, 4> changes {};
    auto shift_changes = [&changes] {
        for (size_t i = 0; i < 3; ++i) {
            changes[i] = changes[i + 1];
        }
    };
    for (uint64_t i = 0; i < count; ++i) {
        mix(current * 64);
        prune();
        mix(current / 32);
        prune();
        mix(current * 2048);
        prune();
        const int64_t price = current % 10;
        shift_changes();
        changes[changes.size() - 1] = static_cast<int8_t>(price - prev_price);
        if (const uint32_t packed_changes = pack_changes(changes); i >= 3 && !changes_price.contains(packed_changes)) {
            changes_price[packed_changes] = price;
        }
        prev_price = price;
    }
    return changes_price;
}

static uint64_t solve(const std::string& data)
{
    std::vector<ChangesPrice> changes_prices;
    for (int pos = 0; pos < data.size(); ++pos) {
        const int64_t initial = parse_int(data, pos);
        changes_prices.push_back(predict_prices(initial, 2000));
    }
    std::unordered_map<uint32_t, uint64_t> price_with_changes_cache;
    auto price_with_changes = [&price_with_changes_cache, &changes_prices](const uint32_t packed_changes) -> uint64_t {
        if (const auto it = price_with_changes_cache.find(packed_changes); it != price_with_changes_cache.end()) {
            return it->second;
        }
        uint64_t price = 0;
        for (const ChangesPrice& buyer_changes_price : changes_prices) {
            if (const auto it = buyer_changes_price.find(packed_changes); it != buyer_changes_price.end()) {
                price += it->second;
            }
        }
        price_with_changes_cache[packed_changes] = price;
        return price;
    };
    uint64_t max_price = std::numeric_limits<uint64_t>::min();
    for (const ChangesPrice& buyer_changes_price : changes_prices) {
        for (const uint32_t packed_changes : buyer_changes_price | std::views::keys) {
            // std::cout << "trying: " << packed_changes << "\n";
            max_price = std::max(max_price, price_with_changes(packed_changes));
        }
    }
    return max_price;
}

int main()
{
    const std::string data = read_data("./day22-part2/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 4;
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