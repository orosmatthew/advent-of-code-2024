#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <sstream>
#include <vector>

static std::string read_data(const std::filesystem::path& path)
{
    const std::fstream file { path };
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

struct Segment {
    std::optional<int64_t> id;
    int64_t size {};
};

static std::vector<Segment> parse_segments(const std::string& data)
{
    std::vector<Segment> segments;
    bool free = false;
    int64_t id_count = 0;
    for (int64_t i = 0; data[i] != '\n'; ++i) {
        if (const int64_t num = data[i] - '0'; free && num > 0) {
            segments.push_back({ .id = std::nullopt, .size = num });
        }
        else if (!free) {
            segments.push_back({ .id = id_count++, .size = num });
        }
        free = !free;
    }
    return segments;
}

static void combine_free(std::vector<Segment>& segments, const int start = 1)
{
    for (int i = start; i < segments.size(); ++i) {
        if (!segments[i - 1].id.has_value() && !segments[i].id.has_value()) {
            segments[i - 1].size += segments[i].size;
            segments.erase(segments.begin() + i);
            --i;
        }
    }
}

static void defrag_segments(std::vector<Segment>& segments)
{
    for (int64_t i = static_cast<int64_t>(segments.size()) - 1; i >= 0; --i) {
        auto& [id, size] = segments[i];
        if (!id.has_value()) {
            continue;
        }
        bool moved = false;
        for (int64_t j = 0; j < i; ++j) {
            auto& [defragged_id, defragged_size] = segments[j];
            if (defragged_id.has_value()) {
                continue;
            }
            if (defragged_size == size) {
                defragged_id = id;
                moved = true;
                break;
            }
            if (defragged_size > size) {
                defragged_size -= size;
                segments.insert(segments.begin() + j, { .id = id, .size = size });
                moved = true;
                ++i;
                break;
            }
        }
        if (moved) {
            segments[i].id.reset();
            combine_free(segments, static_cast<int>(i));
        }
    }
}

static int64_t checksum(const std::vector<Segment>& segments)
{
    int64_t sum = 0;
    int64_t count = 0;
    for (const auto& [id, size] : segments) {
        for (int64_t j = 0; j < size; ++j) {
            if (id.has_value()) {
                sum += count * id.value();
            }
            ++count;
        }
    }
    return sum;
}

static int64_t solve(const std::string& data)
{
    std::vector<Segment> segments = parse_segments(data);
    defrag_segments(segments);
    return checksum(segments);
}

int main()
{
    const std::string data = read_data("./day9-part2/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 1000;
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
    std::printf("%lld\n", solve(data));
#endif
}