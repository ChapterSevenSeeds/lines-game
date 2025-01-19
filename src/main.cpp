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

class Board
{
    std::vector<std::vector<Square>> squares;
    uint32_t rows, columns;

public:
    Board(uint32_t rows, uint32_t columns) : rows(rows), columns(columns)
    {
        squares = std::vector<std::vector<Square>>(rows, std::vector<Square>(columns, {}));

        for (uint32_t row : std::ranges::iota_view{0u, rows})
        {
            for (uint32_t column : std::ranges::iota_view{0u, columns})
            {
                squares[row][column].top_left_corner = Point(row, column);
            }
        }
    }

    void fill_top(uint32_t row, uint32_t column, SideState player)
    {
        squares[row][column].set_top(player);

        if (row > 0)
        {
            squares[row - 1][column].set_bottom(player);
        }
    }

    void fill_left(uint32_t row, uint32_t column, SideState player)
    {
        squares[row][column].set_left(player);

        if (column > 0)
        {
            squares[row][column - 1].set_right(player);
        }
    }

    void fill_right(uint32_t row, uint32_t column, SideState player)
    {
        squares[row][column].set_right(player);

        if (column < columns - 1)
        {
            squares[row][column + 1].set_left(player);
        }
    }

    void fill_bottom(uint32_t row, uint32_t column, SideState player)
    {
        squares[row][column].set_bottom(player);

        if (row < rows - 1)
        {
            squares[row + 1][column].set_top(player);
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
                    moves.push_back(Move(row, column, Side::Top));
                }

                if (square.left == SideState::Empty)
                {
                    moves.push_back(Move(row, column, Side::Left));
                }

                if (square.right == SideState::Empty)
                {
                    moves.push_back(Move(row, column, Side::Right));
                }

                if (square.bottom == SideState::Empty)
                {
                    moves.push_back(Move(row, column, Side::Bottom));
                }
            }
        }

        return moves;
    }

    void go(Player turn, uint32_t &total_moves)
    {
        auto all_moves = get_moves();

        for (auto &move : all_moves)
        {
            apply_move(move, turn);
            go(opposite(turn), ++total_moves);
            undo_move(move);
        }
    }
};

struct ThreadData
{
    Board board;
    Move initial_move;
    uint32_t total_move_count = 0;
    std::thread thread;

    ThreadData(Board board, Move initial_move) : board(board), initial_move(initial_move) {}
};

void thread_entry(ThreadData *data)
{
    data->board.apply_move(data->initial_move, Player::Player);
    data->board.go(Player::Opponent, ++data->total_move_count);
    data->board.undo_move(data->initial_move);
}

int main()
{
    std::vector<ThreadData *> threads_datas{};
    Board reference_board(ROWS, COLUMNS);
    auto initial_moves = reference_board.get_moves();
    for (uint32_t i = 0; i < initial_moves.size(); ++i)
    {
        auto data = new ThreadData(reference_board, initial_moves[i]);
        threads_datas.push_back(data);
        data->thread = std::thread(thread_entry, threads_datas[i]);
    }

    uint32_t total = 0;
    for (uint32_t i = 0; i < initial_moves.size(); ++i)
    {
        threads_datas[i]->thread.join();
        total += threads_datas[i]->total_move_count;
    }

    std::cout << total << std::endl;
}