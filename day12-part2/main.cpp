#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <sstream>
#include <unordered_set>
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

template <>
struct std::hash<Vector2i> {
    size_t operator()(const Vector2i& vector) const noexcept
    {
        const size_t hash1 = std::hash<int>()(vector.x);
        const size_t hash2 = std::hash<int>()(vector.y);
        return hash1 ^ hash2 << 1;
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
            auto [area, edges] = traverse(start_pos.value(), traversed);
            const int unique_edges = unique_edges_count(std::move(edges));
            cost += area * unique_edges;
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

    enum Dir { dir_north = 0, dir_east = 1, dir_south = 2, dir_west = 3 };

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
        std::array<std::unordered_set<Vector2i>, 4> edges;
    };

    [[nodiscard]] TraverseResult traverse( // NOLINT(*-no-recursion)
        const Vector2i& start, std::vector<bool>& traversed) const
    {
        assert(traversed.size() == m_data.size());
        assert(!traversed[index(start)]);
        const int start_index = index(start);
        traversed[start_index] = true;
        TraverseResult result { .area = 1, .edges = std::array<std::unordered_set<Vector2i>, 4> {} };
        constexpr std::array<Vector2i, 4> offsets { { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } } };
        for (int i = 0; i < 4; ++i) {
            const Vector2i neighbor_pos = start + offsets[i];
            if (!in_bounds(neighbor_pos)) {
                result.edges[i].insert(start);
                continue;
            }
            if (const int neighbor_index = index(neighbor_pos);
                !traversed[neighbor_index] && m_data[neighbor_index] == m_data[start_index]) {
                auto [neighbor_area, neighbor_edges] = traverse(neighbor_pos, traversed);
                result.area += neighbor_area;
                for (int j = 0; j < 4; ++j) {
                    for (const Vector2i& neighbor_edge_pos : neighbor_edges[j]) {
                        result.edges[j].insert(neighbor_edge_pos);
                    }
                }
            }
            else if (m_data[neighbor_index] != m_data[start_index]) {
                result.edges[i].insert(start);
            }
        }
        return result;
    }

    static std::pair<Vector2i, Vector2i> edge_dir_offsets(const Dir dir)
    {
        switch (dir) {
        case dir_north:
        case dir_south:
            return std::make_pair(Vector2i { -1, 0 }, Vector2i { 1, 0 });
        case dir_east:
        case dir_west:
            return std::make_pair(Vector2i { 0, -1 }, Vector2i { 0, 1 });
        }
        std::unreachable();
    }

    static int unique_edges_count(std::array<std::unordered_set<Vector2i>, 4>&& edges)
    {
        auto next_dir = [&edges]() -> std::optional<Dir> {
            for (int i = 0; i < 4; ++i) {
                if (!edges[i].empty()) {
                    return static_cast<Dir>(i);
                }
            }
            return std::nullopt;
        };
        int count = 0;
        std::optional<Dir> dir = next_dir();
        while (dir.has_value()) {
            std::unordered_set<Vector2i>& dir_positions = edges[dir.value()];
            const auto [offset_first, offset_second] = edge_dir_offsets(dir.value());
            const Vector2i start = *dir_positions.begin();
            Vector2i current = start + offset_first;
            while (dir_positions.contains(current)) {
                dir_positions.erase(current);
                current += offset_first;
            }
            current = start + offset_second;
            while (dir_positions.contains(current)) {
                dir_positions.erase(current);
                current += offset_second;
            }
            dir_positions.erase(start);
            ++count;
            dir = next_dir();
        }
        return count;
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
    const std::string data = read_data("./day12-part2/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 100;
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