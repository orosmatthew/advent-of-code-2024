#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <sstream>
#include <unordered_set>
#include <utility>
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
        Edges edges;
        auto clear_edges = [&edges] {
            for (auto& v : edges) {
                v.clear();
            }
        };
        while (start_pos.has_value()) {
            int area = 0;
            traverse(start_pos.value(), traversed, area, edges);
            const int unique_edges = unique_edges_count(std::move(edges));
            cost += area * unique_edges;
            start_pos = untraversed_start_pos(traversed);
            clear_edges();
        }
        return cost;
    }

private:
    Farm(std::vector<char>&& data, const Vector2i& size)
        : m_data { std::move(data) }
        , m_size { size }
    {
    }

    using Edges = std::array<std::vector<Vector2i>, 4>;

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

    void traverse( // NOLINT(*-no-recursion)
        const Vector2i& start,
        std::vector<bool>& traversed,
        int& area,
        Edges& edges) const
    {
        assert(traversed.size() == m_data.size());
        assert(!traversed[index(start)]);
        const int start_index = index(start);
        traversed[start_index] = true;
        ++area;
        constexpr std::array<Vector2i, 4> offsets { { { 0, -1 }, { 1, 0 }, { 0, 1 }, { -1, 0 } } };
        for (int i = 0; i < 4; ++i) {
            const Vector2i neighbor_pos = start + offsets[i];
            if (!in_bounds(neighbor_pos)) {
                edges[i].push_back(start);
                continue;
            }
            if (const int neighbor_index = index(neighbor_pos);
                !traversed[neighbor_index] && m_data[neighbor_index] == m_data[start_index]) {
                traverse(neighbor_pos, traversed, area, edges);
            }
            else if (m_data[neighbor_index] != m_data[start_index]) {
                edges[i].push_back(start);
            }
        }
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

    static int unique_edges_count(Edges&& edges)
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
            std::vector<Vector2i>& dir_positions = edges[dir.value()];
            const Vector2i start = dir_positions[dir_positions.size() - 1];
            auto remove_along_offset = [start, &dir_positions](const Vector2i& offset) {
                Vector2i current = start + offset;
                auto it = std::ranges::find(dir_positions, current);
                while (it != dir_positions.end()) {
                    dir_positions.erase(it);
                    current += offset;
                    it = std::ranges::find(dir_positions, current);
                }
            };
            const auto [offset_first, offset_second] = edge_dir_offsets(dir.value());
            remove_along_offset(offset_first);
            remove_along_offset(offset_second);
            dir_positions.pop_back();
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