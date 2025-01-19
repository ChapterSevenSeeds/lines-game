#include "timer.hpp"
#include <iostream>
#include <ranges>
#include <unordered_set>
#include <thread>
#include <algorithm>
#include <iostream>
#include <format>
#include <chrono>
#include <vector>
#include <limits>
#include "hash_combine.hpp"

using namespace std::chrono_literals;

constexpr auto ROWS = 15;
constexpr auto COLUMNS = 20;

class Point
{
public:
    uint32_t row;
    uint32_t column;

    Point(uint32_t row, uint32_t column) : row(row), column(column) {}
    Point() {}
};

enum class SideState
{
    Opponent,
    Player,
    Empty
};

enum class Player
{
    Opponent,
    Player
};

SideState player_to_side_state(Player player)
{
    return player == Player::Opponent ? SideState::Opponent : SideState::Player;
}

Player opposite(Player player)
{
    return player == Player::Opponent ? Player::Player : Player::Opponent;
}

enum class Side
{
    Top,
    Left,
    Right,
    Bottom
};

class Square
{
public:
    Point top_left_corner;
    SideState top = SideState::Empty;
    SideState left = SideState::Empty;
    SideState right = SideState::Empty;
    SideState bottom = SideState::Empty;
    std::optional<Player> filled_by;

    Square() {}

    void fill(SideState &side, SideState state)
    {
        side = state;
        if (top != SideState::Empty && left != SideState::Empty && right != SideState::Empty && bottom != SideState::Empty)
        {
            filled_by = state == SideState::Player ? Player::Player : Player::Opponent;
        }
        else
        {
            filled_by = std::nullopt;
        }
    }

    void set_top(SideState player)
    {
        fill(top, player);
    }

    void set_left(SideState player)
    {
        fill(left, player);
    }

    void set_right(SideState player)
    {
        fill(right, player);
    }

    void set_bottom(SideState player)
    {
        fill(bottom, player);
    }

    bool is_full() const
    {
        return filled_by != std::nullopt && top != SideState::Empty && left != SideState::Empty && right != SideState::Empty && bottom != SideState::Empty;
    }
};

struct Move
{
    uint32_t row, column;
    Side side;

    Move(uint32_t row, uint32_t column, Side side) : row(row), column(column), side(side) {}
};

bool is_out_of_time(std::chrono::steady_clock::time_point &start, std::chrono::seconds max_time)
{
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    return duration >= max_time;
}

class Board
{
    std::vector<std::vector<Square>> squares;
    uint32_t rows, columns;
    int32_t player_points, opponent_points;

public:
    Board(uint32_t rows, uint32_t columns) : rows(rows), columns(columns)
    {
        squares = std::vector<std::vector<Square>>(rows, std::vector<Square>(columns));

        for (uint32_t row : std::ranges::iota_view{0u, rows})
        {
            for (uint32_t column : std::ranges::iota_view{0u, columns})
            {
                squares[row][column].top_left_corner = Point(row, column);
            }
        }
    }

    void increment_score_if_applicable(uint32_t row, uint32_t column, std::optional<Player> previously_filled_by)
    {
        if (!squares[row][column].is_full() && previously_filled_by.has_value())
        {
            switch (previously_filled_by.value())
            {
            case Player::Player:
                --player_points;
                break;
            case Player::Opponent:
                --opponent_points;
                break;
            default:
                break;
            }
        }
        else if (squares[row][column].is_full())
        {
            switch (squares[row][column].filled_by.value())
            {
            case Player::Player:
                ++player_points;
                break;
            case Player::Opponent:
                ++opponent_points;
                break;
            default:
                break;
            }
        }
    }

    void fill_top(uint32_t row, uint32_t column, SideState player)
    {
        auto previously_filled_by = squares[row][column].filled_by;
        squares[row][column].set_top(player);
        increment_score_if_applicable(row, column, previously_filled_by);

        if (row > 0)
        {
            previously_filled_by = squares[row - 1][column].filled_by;
            squares[row - 1][column].set_bottom(player);
            increment_score_if_applicable(row - 1, column, previously_filled_by);
        }
    }

    void fill_left(uint32_t row, uint32_t column, SideState player)
    {
        auto previously_filled_by = squares[row][column].filled_by;
        squares[row][column].set_left(player);
        increment_score_if_applicable(row, column, previously_filled_by);

        if (column > 0)
        {
            previously_filled_by = squares[row][column - 1].filled_by;
            squares[row][column - 1].set_right(player);
            increment_score_if_applicable(row, column - 1, previously_filled_by);
        }
    }

    void fill_right(uint32_t row, uint32_t column, SideState player)
    {
        auto previously_filled_by = squares[row][column].filled_by;
        squares[row][column].set_right(player);
        increment_score_if_applicable(row, column, previously_filled_by);

        if (column < columns - 1)
        {
            previously_filled_by = squares[row][column + 1].filled_by;
            squares[row][column + 1].set_left(player);
            increment_score_if_applicable(row, column + 1, previously_filled_by);
        }
    }

    void fill_bottom(uint32_t row, uint32_t column, SideState player)
    {
        auto previously_filled_by = squares[row][column].filled_by;
        squares[row][column].set_bottom(player);
        increment_score_if_applicable(row, column, previously_filled_by);

        if (row < rows - 1)
        {
            previously_filled_by = squares[row + 1][column].filled_by;
            squares[row + 1][column].set_top(player);
            increment_score_if_applicable(row + 1, column, previously_filled_by);
        }
    }

    void apply_move(Move move, Player player)
    {
        switch (move.side)
        {
        case Side::Top:
            fill_top(move.row, move.column, player_to_side_state(player));
            break;
        case Side::Left:
            fill_left(move.row, move.column, player_to_side_state(player));
            break;
        case Side::Right:
            fill_right(move.row, move.column, player_to_side_state(player));
            break;
        case Side::Bottom:
            fill_bottom(move.row, move.column, player_to_side_state(player));
            break;
        }
    }

    void undo_move(Move move)
    {
        switch (move.side)
        {
        case Side::Top:
            fill_top(move.row, move.column, SideState::Empty);
            break;
        case Side::Left:
            fill_left(move.row, move.column, SideState::Empty);
            break;
        case Side::Right:
            fill_right(move.row, move.column, SideState::Empty);
            break;
        case Side::Bottom:
            fill_bottom(move.row, move.column, SideState::Empty);
            break;
        }
    }

    std::vector<Move> get_moves()
    {
        std::vector<Move> moves{};
        for (uint32_t row : std::ranges::iota_view{0u, rows})
        {
            for (uint32_t column : std::ranges::iota_view{0u, columns})
            {
                auto &square = squares[row][column];

                if (square.top == SideState::Empty)
                {
                    moves.emplace_back(row, column, Side::Top);
                }

                if (square.left == SideState::Empty)
                {
                    moves.emplace_back(row, column, Side::Left);
                }

                if (square.right == SideState::Empty)
                {
                    moves.emplace_back(row, column, Side::Right);
                }

                if (square.bottom == SideState::Empty)
                {
                    moves.emplace_back(row, column, Side::Bottom);
                }
            }
        }

        return moves;
    }

    int32_t search(Player turn, uint32_t &total_moves, uint32_t max_depth, uint32_t current_depth, Timer &timer)
    {
        if (current_depth == max_depth)
        {
            return player_points - opponent_points;
        }

        auto all_moves = get_moves();

        int32_t current_score = turn == Player::Player ? std::numeric_limits<int32_t>::min() : std::numeric_limits<int32_t>::max();
        for (auto &move : all_moves)
        {
            if (timer.has_elapsed())
            {
                return current_score;
            }

            apply_move(move, turn);

            auto new_score = search(opposite(turn), ++total_moves, max_depth, current_depth + 1, timer);
            if ((turn == Player::Player && new_score > current_score) || (turn == Player::Opponent && new_score < current_score))
            {
                current_score = new_score;
            }

            undo_move(move);
        }

        return current_score;
    }
};

struct ThreadData
{
    Board board;
    std::vector<Move> initial_moves;
    uint32_t total_move_count = 0;
    std::thread thread;
    Timer &timer;
    int32_t high_score = 0;

    ThreadData(Board board, std::vector<Move> initial_moves, Timer &timer) : board(board), initial_moves(initial_moves), timer(timer) {}
};

void thread_entry(ThreadData *data)
{
    int32_t max_depth = 1;
    while (!data->timer.has_elapsed())
    {
        for (auto &move : data->initial_moves)
        {
            data->board.apply_move(move, Player::Player);
            auto new_score = data->board.search(Player::Opponent, data->total_move_count, max_depth, 0, data->timer);
            if (new_score > data->high_score)
            {
                data->high_score = new_score;
            }
            data->board.undo_move(move);
        }

        ++max_depth;
    }
}

std::vector<std::vector<Move>> divide_evenly(std::vector<Move> input, int32_t groups) {
    std::vector<std::vector<Move>> result{};
    auto items_per_group = input.size() / groups;
    if (input.size() % groups != 0) {
        ++items_per_group;
    }

    std::vector<Move> current_group{};
    for (const auto& item : input) {
        if (current_group.size() >= items_per_group) {
            result.push_back(current_group);
            current_group = std::vector<Move>();
        }

        current_group.push_back(item);
    }

    return result;
}

int main()
{
    std::vector<ThreadData *> threads_datas{};
    Board reference_board(ROWS, COLUMNS);
    auto initial_moves = reference_board.get_moves();
    auto timer = Timer(5s);

    for (auto move_group : divide_evenly(initial_moves, 1))
    {
        std::vector<Move> thread_moves(move_group.begin(), move_group.end());
        auto thread_data = new ThreadData(reference_board, thread_moves, timer);
        thread_data->thread = std::thread(thread_entry, thread_data);
        threads_datas.push_back(thread_data);
    }

    int index = 0;
    for (const auto &thread : threads_datas)
    {
        thread->thread.join();

        std::cout << std::format("Thread {} score: {}", index, thread->high_score) << std::endl;
        std::cout << std::format("Thread {} moved search: {}", index++, thread->total_move_count) << std::endl;
    }
}