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

static void trim_free_end(std::vector<Segment>& segments)
{
    if (auto [id, size] = segments[segments.size() - 1]; !id.has_value()) {
        segments.pop_back();
    }
}

static std::vector<Segment> defrag_segments(std::vector<Segment>&& segments)
{
    std::vector<Segment> defragged_segments;
    trim_free_end(segments);
    for (int64_t i = 0; i < segments.size(); ++i) {
        Segment& segment = segments[i];
        if (segment.id.has_value()) {
            defragged_segments.push_back(segment);
            continue;
        }
        int64_t free_size = segment.size;
        while (free_size > 0) {
            trim_free_end(segments);
            if (Segment& last_file = segments[segments.size() - 1]; last_file.size <= free_size) {
                const int64_t last_size = last_file.size;
                defragged_segments.push_back(last_file);
                free_size -= last_size;
                segments.pop_back();
                if (i == segments.size() - 1) {
                    break;
                }
            }
            else {
                defragged_segments.push_back({ .id = last_file.id, .size = free_size });
                last_file.size -= free_size;
                free_size = 0;
            }
        }
    }
    if (defragged_segments.size() >= 2) {
        const auto& [last_id, last_size] = defragged_segments[defragged_segments.size() - 1];
        if (auto& [second_last_id, second_last_size] = defragged_segments[defragged_segments.size() - 2];
            last_id == second_last_id) {
            second_last_size += last_size;
            defragged_segments.pop_back();
        }
    }
    return defragged_segments;
}

static int64_t checksum(const std::vector<Segment>& segments)
{
    int64_t sum = 0;
    int64_t count = 0;
    for (int64_t i = 0; i < segments.size() && segments[i].id.has_value(); ++i) {
        const auto& [id, size] = segments[i];
        for (int64_t j = 0; j < size; ++j) {
            sum += count * id.value();
            ++count;
        }
    }
    return sum;
}

static int64_t solve(const std::string& data)
{
    std::vector<Segment> segments = parse_segments(data);
    const std::vector<Segment> defragged_segments = defrag_segments(std::move(segments));
    return checksum(defragged_segments);
}

int main()
{
    const std::string data = read_data("./day09-part1/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 10000;
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