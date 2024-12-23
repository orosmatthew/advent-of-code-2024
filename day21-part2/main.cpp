#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <ranges>
#include <span>
#include <sstream>
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

struct Vector2i {
    int64_t x;
    int64_t y;

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

    Vector2i operator*(const int64_t value) const
    {
        return { x * value, y * value };
    }

    Vector2i operator/(const int64_t value) const
    {
        return { x / value, y / value };
    }

    Vector2i& operator%=(const Vector2i& other)
    {
        x %= other.x;
        y %= other.y;
        return *this;
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

enum class DirKey { up, a, left, down, right };

// clang-format off
static std::map<char, Vector2i> keypad_positions {
    { '7', { 0, 0 } }, { '8', { 1, 0 } }, { '9', { 2, 0 } },
    { '4', { 0, 1 } }, { '5', { 1, 1 } }, { '6', { 2, 1 } },
    { '1', { 0, 2 } }, { '2', { 1, 2 } }, { '3', { 2, 2 } },
                       { '0', { 1, 3 } }, { 'A', { 2, 3 } }
};

static std::map<DirKey, Vector2i> dirpad_positions {
                                    { DirKey::up,   { 1, 0 } }, { DirKey::a,     { 2, 0 } },
        { DirKey::left, { 0, 1 } }, { DirKey::down, { 1, 1 } }, { DirKey::right, { 2, 1 } }
};
// clang-format on

enum class PadType { num, dir };

class DirKeySolutions {
public:
    void push(std::vector<DirKey> keys)
    {
        m_data[m_size++] = std::move(keys);
    }

    [[nodiscard]] const std::vector<DirKey>& at(const size_t index) const
    {
        assert(index <= 1);
        return m_data[index].value();
    }

    [[nodiscard]] size_t size() const
    {
        return m_size;
    }

private:
    size_t m_size = 0;
    std::array<std::optional<std::vector<DirKey>>, 2> m_data;
};

static DirKeySolutions dirpad_offset_to_keys(const PadType pad_type, const Vector2i& from, const Vector2i& to)
{
    DirKeySolutions solutions;
    const Vector2i offset = to - from;
    if (offset == Vector2i { 0, 0 }) {
        solutions.push({ DirKey::a });
        return solutions;
    }
    const std::optional<DirKey> key_x
        = offset.x != 0 ? offset.x > 0 ? DirKey::right : DirKey::left : std::optional<DirKey> { std::nullopt };
    const std::optional<DirKey> key_y
        = offset.y != 0 ? offset.y > 0 ? DirKey::down : DirKey::up : std::optional<DirKey> { std::nullopt };
    static std::vector<DirKey> temp;
    temp.clear();
    Vector2i current = from;
    auto invalid = [pad_type](const Vector2i& pos) {
        return (pad_type == PadType::dir && pos == Vector2i { 0, 0 })
            || (pad_type == PadType::num && pos == Vector2i { 0, 3 });
    };
    auto move_x = [&]() -> bool {
        if (!key_x.has_value()) {
            return true;
        }
        for (int i = 0; i < std::abs(offset.x); ++i) {
            current.x += offset.x;
            if (invalid(current)) {
                return false;
            }
            temp.push_back(key_x.value());
        }
        return true;
    };
    auto move_y = [&]() -> bool {
        if (!key_y.has_value()) {
            return true;
        }
        for (int i = 0; i < std::abs(offset.y); ++i) {
            current.y += offset.y;
            if (invalid(current)) {
                return false;
            }
            temp.push_back(key_y.value());
        }
        return true;
    };
    auto try_x_y = [&]() -> bool {
        temp.clear();
        current = from;
        if (!move_x()) {
            return false;
        }
        if (!move_y()) {
            return false;
        }
        return true;
    };
    auto try_y_x = [&]() -> bool {
        temp.clear();
        current = from;
        if (!move_y()) {
            return false;
        }
        if (!move_x()) {
            return false;
        }
        return true;
    };
    if (try_x_y()) {
        temp.push_back(DirKey::a);
        solutions.push(temp);
    }
    if (try_y_x()) {
        temp.push_back(DirKey::a);
        solutions.push(temp);
    }
    return solutions;
}

static bool parse_code(const std::string& data, int& pos, std::vector<char>& code)
{
    if (pos >= data.size()) {
        return false;
    }
    code.clear();
    for (; data[pos] != '\n'; ++pos) {
        assert((data[pos] >= '0' && data[pos] <= '9') || data[pos] == 'A');
        code.push_back(data[pos]);
    }
    ++pos; // "\n"
    return true;
}

static bool is_digit(const char c)
{
    return c >= '0' && c <= '9';
}

static int64_t parse_int(const std::span<const char>& chars)
{
    int64_t result = 0;
    int pos = 0;
    do {
        result = result * 10 + (chars[pos] - '0');
        ++pos;
    } while (is_digit(chars[pos]) && pos < chars.size());
    return result;
}

static std::map<std::pair<std::vector<DirKey>, int>, uint64_t> dirpad_min_moves_cache;

static uint64_t dirpad_min_moves(const std::vector<DirKey>& keys, const int depth = 0) // NOLINT(*-no-recursion)
{
    if (depth >= 25) {
        return keys.size();
    }
    if (const auto it = dirpad_min_moves_cache.find({ keys, depth }); it != dirpad_min_moves_cache.end()) {
        return it->second;
    }
    auto current = DirKey::a;
    uint64_t result = 0;
    for (const DirKey key : keys) {
        DirKeySolutions solutions
            = dirpad_offset_to_keys(PadType::dir, dirpad_positions[current], dirpad_positions[key]);
        uint64_t min_moves = std::numeric_limits<uint64_t>::max();
        for (size_t i = 0; i < solutions.size(); ++i) {
            min_moves = std::min(min_moves, dirpad_min_moves(solutions.at(i), depth + 1));
        }
        result += min_moves;
        current = key;
    }
    dirpad_min_moves_cache[{ keys, depth }] = result;
    return result;
}

static uint64_t solve(const std::string& data)
{
    std::vector<char> code;
    int pos = 0;
    uint64_t result = 0;
    while (parse_code(data, pos, code)) {
        char keypad_current = 'A';
        std::vector<DirKey> dirpad1_keys;
        uint64_t count = 0;
        for (const char c : code) {
            DirKeySolutions solutions
                = dirpad_offset_to_keys(PadType::num, keypad_positions[keypad_current], keypad_positions[c]);
            uint64_t min_moves = std::numeric_limits<uint64_t>::max();
            for (size_t i = 0; i < solutions.size(); ++i) {
                min_moves = std::min(min_moves, dirpad_min_moves(solutions.at(i)));
            }
            count += min_moves;
            keypad_current = c;
        }
        result += count * parse_int(code);
    }
    return result;
}

int main()
{
    const std::string data = read_data("./day21-part2/input.txt");

#ifdef BENCHMARK
    constexpr int n_runs = 1000000;
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