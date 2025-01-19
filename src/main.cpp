#include <iostream>
#include <ranges>
#include <unordered_set>
#include <thread>
#include <algorithm>
#include <iostream>
#include <format>
#include "hash_combine.hpp"

constexpr auto ROWS = 2;
constexpr auto COLUMNS = 2;

struct Point
{
    uint32_t row;
    uint32_t column;

    Point(uint32_t row, uint32_t column) : row(row), column(column) {}
    Point() {}
};

std::ostream &operator<<(std::ostream &os, const Point &point)
{
    return os << std::format("({}, {})", point.row, point.column);
}

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

std::ostream &operator<<(std::ostream &os, const SideState &state)
{
    return os << (state == SideState::Opponent ? "Opponent" : state == SideState::Player ? "Player"
                                                                                         : "Empty");
}

enum class Side
{
    Top,
    Left,
    Right,
    Bottom
};

struct Square
{
    Point top_left_corner;
    SideState top = SideState::Empty;
    SideState left = SideState::Empty;
    SideState right = SideState::Empty;
    SideState bottom = SideState::Empty;
    std::optional<Player> filled_by;
    std::optional<std::reference_wrapper<Square>> top_neighbor;
    std::optional<std::reference_wrapper<Square>> left_neighbor;
    std::optional<std::reference_wrapper<Square>> right_neighbor;
    std::optional<std::reference_wrapper<Square>> bottom_neighbor;

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

    void set_top(SideState player, bool update_neighbors)
    {
        fill(top, player);

        if (top_neighbor.has_value() && update_neighbors)
        {
            top_neighbor.value().get().set_bottom(player, false);
        }
    }

    void set_left(SideState player, bool update_neighbors)
    {
        fill(left, player);

        if (left_neighbor.has_value() && update_neighbors)
        {
            left_neighbor.value().get().set_right(player, false);
        }
    }

    void set_right(SideState player, bool update_neighbors)
    {
        fill(right, player);

        if (right_neighbor.has_value() && update_neighbors)
        {
            right_neighbor.value().get().set_left(player, false);
        }
    }

    void set_bottom(SideState player, bool update_neighbors)
    {
        fill(bottom, player);

        if (bottom_neighbor.has_value() && update_neighbors)
        {
            bottom_neighbor.value().get().set_top(player, false);
        }
    }

    bool is_full() const
    {
        return filled_by != std::nullopt && top != SideState::Empty && left != SideState::Empty && right != SideState::Empty && bottom != SideState::Empty;
    }
};
using Board = std::array<std::array<Square, COLUMNS>, ROWS>;

std::ostream &operator<<(std::ostream &os, const Square &square)
{
    return os << "Top left: " << square.top_left_corner << square.top << "," << square.left << "," << square.right << "," << square.bottom;
}

struct Move
{
    Square &square;
    Side side;

    Move(Square &square, Side side) : square(square), side(side) {}

    void apply_move(Player player)
    {
        switch (side)
        {
        case Side::Top:
            square.set_top(player_to_side_state(player), true);
            break;
        case Side::Left:
            square.set_left(player_to_side_state(player), true);
            break;
        case Side::Right:
            square.set_right(player_to_side_state(player), true);
            break;
        case Side::Bottom:
            square.set_bottom(player_to_side_state(player), true);
            break;
        }
    }

    void undo_move()
    {
        switch (side)
        {
        case Side::Top:
            square.set_top(SideState::Empty, true);
            break;
        case Side::Left:
            square.set_left(SideState::Empty, true);
            break;
        case Side::Right:
            square.set_right(SideState::Empty, true);
            break;
        case Side::Bottom:
            square.set_bottom(SideState::Empty, true);
            break;
        }
    }
};

std::vector<Move> get_moves(Board &board)
{
    std::vector<Move> moves{};
    for (const auto row : std::views::iota(0, ROWS))
    {
        for (const auto column : std::views::iota(0, COLUMNS))
        {
            auto &square = board[row][column];

            if (square.top == SideState::Empty)
            {
                moves.push_back(Move(square, Side::Top));
            }

            if (square.left == SideState::Empty)
            {
                moves.push_back(Move(square, Side::Left));
            }

            if (square.right == SideState::Empty)
            {
                moves.push_back(Move(square, Side::Right));
            }

            if (square.bottom == SideState::Empty)
            {
                moves.push_back(Move(square, Side::Bottom));
            }
        }
    }

    return moves;
}

void go(Board &board, Player turn, size_t &total_moves)
{
    auto all_moves = get_moves(board);

    for (auto &move : all_moves)
    {
        move.apply_move(turn);
        go(board, opposite(turn), ++total_moves);
        move.undo_move();
    }
}


struct ThreadData
{
    Board board;
    Move initial_move;
    size_t total_move_count = 0;
    std::thread thread;

    ThreadData(Board &board, Move &initial_move) : board(board), initial_move(initial_move) {}
};
void thread_entry(ThreadData* data)
{
    data->initial_move.apply_move(Player::Player);
    go(data->board, Player::Opponent, ++data->total_move_count);
    data->initial_move.undo_move();
}

Board generate_board()
{

    Board board{};

    for (const auto row : std::views::iota(0, ROWS))
    {
        for (const auto column : std::views::iota(0, COLUMNS))
        {
            board[row][column] = Square();
            board[row][column].top_left_corner = Point(row, column);
        }
    }

    for (const auto row : std::views::iota(0, ROWS))
    {
        for (const auto column : std::views::iota(0, COLUMNS))
        {
            if (row > 0)
            {
                board[row][column].top_neighbor = board[row - 1][column];
            }

            if (column > 0)
            {
                board[row][column].left_neighbor = board[row][column - 1];
            }

            if (column < COLUMNS - 1)
            {
                board[row][column].right_neighbor = board[row][column + 1];
            }

            if (row < ROWS - 1)
            {
                board[row][column].bottom_neighbor = board[row + 1][column];
            }
        }
    }

    return board;
}

int main()
{
    std::vector<ThreadData*> threads_datas{};
    Board reference_board= generate_board();
    auto move_count = get_moves(reference_board).size();
    for (size_t i = 0; i < move_count; ++i)
    {
        Board board = generate_board();
        auto moves = get_moves(board);
        auto data = new ThreadData(board, moves[i]);
        threads_datas.push_back(data);
        data->thread = std::thread(thread_entry, threads_datas[i]);
    }

    size_t total = 0;
    for (size_t i = 0; i < move_count; ++i) {
        threads_datas[i]->thread.join();
        total += threads_datas[i]->total_move_count;
    }
}