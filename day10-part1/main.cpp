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

class Map {
public:
    static Map parse(const std::string& data)
    {
        std::vector<int> heights;
        std::vector<Vector2i> trailheads;
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
            const int height = data[i] - '0';
            heights.push_back(height);
            if (height == 0) {
                const int x = width.has_value() ? i % (width.value() + 1) : i;
                trailheads.push_back({ x, y });
            }
        }
        return { std::move(heights), std::move(trailheads), { width.value(), y } };
    }

    [[nodiscard]] int64_t trailhead_scores_sum() const
    {
        std::unordered_set<Vector2i> max_heights;
        int64_t score_sum = 0;
        for (const Vector2i& trailhead : m_trailheads) {
            max_heights.clear();
            reachable_max_heights_from(trailhead, max_heights);
            score_sum += static_cast<int64_t>(max_heights.size());
        }
        return score_sum;
    }

private:
    Map(std::vector<int>&& data, std::vector<Vector2i>&& trailheads, const Vector2i& size)
        : m_data { std::move(data) }
        , m_trailheads { std::move(trailheads) }
        , m_size { size }
    {
    }

    [[nodiscard]] bool in_bounds(const Vector2i& pos) const
    {
        return pos.x >= 0 && pos.x < m_size.x && pos.y >= 0 && pos.y < m_size.y;
    }

    [[nodiscard]] int index(const Vector2i& pos) const
    {
        return pos.y * m_size.x + pos.x;
    }

    [[nodiscard]] std::optional<int> height_at_opt(const Vector2i& pos) const
    {
        if (!in_bounds(pos)) {
            return std::nullopt;
        }
        return m_data[index(pos)];
    }

    void reachable_max_heights_from( // NOLINT(*-no-recursion)
        const Vector2i& pos, std::unordered_set<Vector2i>& max_heights) const
    {
        const int current_height = m_data[index(pos)];
        if (current_height == 9) {
            max_heights.insert(pos);
            return;
        }
        for (constexpr std::array<Vector2i, 4> offsets { { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } } };
             const Vector2i& offset : offsets) {
            const Vector2i neighbor_pos = pos + offset;
            if (const std::optional<int> neighbor_height = height_at_opt(neighbor_pos);
                neighbor_height.has_value() && neighbor_height.value() == current_height + 1) {
                reachable_max_heights_from(neighbor_pos, max_heights);
            }
        }
    }

    std::vector<int> m_data;
    std::vector<Vector2i> m_trailheads;
    Vector2i m_size;
};

static int64_t solve(const std::string& data)
{
    const Map map = Map::parse(data);
    return map.trailhead_scores_sum();
}

int main()
{
    const std::string data = read_data("./day10-part1/input.txt");

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