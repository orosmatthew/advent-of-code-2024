#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
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

struct Vector2i {
    int x;
    int y;

    Vector2i& operator+=(const Vector2i& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2i operator+(const Vector2i& other) const
    {
        return { x + other.x, y + other.y };
    }

    Vector2i& operator-=(const Vector2i& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vector2i operator-(const Vector2i& other) const
    {
        return { x - other.x, y - other.y };
    }

    bool operator==(const Vector2i& other) const
    {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Vector2i& other) const
    {
        return x != other.x || y != other.y;
    }

    bool operator<(const Vector2i& other) const
    {
        if (x == other.x) {
            return y < other.y;
        }
        return x < other.x;
    }
};

class Farm {
public:
    static Farm parse(const std::string& data)
    {
        std::vector<char> plants;
        std::optional<int> width;
        int y = 0;
        for (int i = 0; i < data.size(); ++i) {
            if (data[i] == '\n') {
                if (!width.has_value()) {
                    width = i;
                }
                ++y;
                continue;
            }
            plants.push_back(data[i]);
        }
        return { std::move(plants), { width.value(), y } };
    }

    [[nodiscard]] uint64_t fence_cost() const
    {
        std::vector<bool> traversed;
        traversed.resize(m_data.size(), false);
        std::optional<Vector2i> start_pos = untraversed_start_pos(traversed);
        uint64_t cost = 0;
        while (start_pos.has_value()) {
            auto [area, perimeter] = traverse(start_pos.value(), traversed);
            cost += area * perimeter;
            start_pos = untraversed_start_pos(traversed);
        }
        return cost;
    }

private:
    Farm(std::vector<char>&& data, const Vector2i& size)
        : m_data { std::move(data) }
        , m_size { size }
    {
    }

    [[nodiscard]] int index(const Vector2i& pos) const
    {
        return pos.y * m_size.x + pos.x;
    }

    [[nodiscard]] std::optional<Vector2i> untraversed_start_pos(const std::vector<bool>& traversed) const
    {
        assert(traversed.size() == m_data.size());
        for (int y = 0; y < m_size.y; ++y) {
            for (int x = 0; x < m_size.x; ++x) {
                if (!traversed[index({ x, y })]) {
                    return Vector2i { x, y };
                }
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] bool in_bounds(const Vector2i& pos) const
    {
        return pos.x >= 0 && pos.x < m_size.x && pos.y >= 0 && pos.y < m_size.y;
    }

    struct TraverseResult {
        int area;
        int perimeter;
    };

    [[nodiscard]] TraverseResult traverse( // NOLINT(*-no-recursion)
        const Vector2i& start, std::vector<bool>& traversed) const
    {
        assert(traversed.size() == m_data.size());
        assert(!traversed[index(start)]);
        const int start_index = index(start);
        traversed[start_index] = true;
        TraverseResult result { .area = 1, .perimeter = 0 };
        for (constexpr std::array<Vector2i, 4> offsets { { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } } };
             const Vector2i& offset : offsets) {
            const Vector2i neighbor_pos = start + offset;
            if (!in_bounds(neighbor_pos)) {
                ++result.perimeter;
                continue;
            }
            if (const int neighbor_index = index(neighbor_pos);
                !traversed[neighbor_index] && m_data[neighbor_index] == m_data[start_index]) {
                auto [neighbor_area, neighbor_perimeter] = traverse(neighbor_pos, traversed);
                result.area += neighbor_area;
                result.perimeter += neighbor_perimeter;
            }
            else if (m_data[neighbor_index] != m_data[start_index]) {
                ++result.perimeter;
            }
        }
        return result;
    }

    std::vector<char> m_data;
    Vector2i m_size;
};

static uint64_t solve(const std::string& data)
{
    const Farm farm = Farm::parse(data);
    return farm.fence_cost();
}

int main()
{
    const std::string data = read_data("./day12-part1/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 1000;
    double time_running_total = 0.0;

    for (int n_run = 0; n_run < n_runs; ++n_run) {
        auto start = std::chrono::high_resolution_clock::now();
        // ReSharper disable once CppDFAUnusedValue
        // ReSharper disable once CppDFAUnreadVariable
        // ReSharper disable once CppDeclaratorNeverUsed
        volatile uint64_t result = solve(data);
        auto end = std::chrono::high_resolution_clock::now();
        time_running_total += std::chrono::duration<double, std::nano>(end - start).count();
    }
    std::printf("Average ns: %d\n", static_cast<int>(std::round(time_running_total / n_runs)));
#else
    std::printf("%llu\n", solve(data));
#endif
}