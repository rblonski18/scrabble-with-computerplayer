#include "human_player.h"

#include "exceptions.h"
#include "formatting.h"
#include "move.h"
#include "place_result.h"
#include "rang.h"
#include "tile_kind.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace std;

// This method is fully implemented.
inline string& to_upper(string& str) {
    transform(str.begin(), str.end(), str.begin(), ::toupper);
    return str;
}

Move HumanPlayer::get_move(const Board& board, const Dictionary& dictionary) const {

    print_hand(cout);
    // string playername = this->get_name();
    cout << endl << "Enter a move,  " << this->get_name() << "!" << endl;
    string movetype;
    cin >> movetype;
    try {
        if (movetype[1] == 'A') {
            return Move();
        } else if (movetype[1] == 'X') {
            // exchange in scrabble.cpp because we don't have access to TileBag here.
            vector<TileKind> exchangetiles;
            string tiles;
            cin >> tiles;
            cin.ignore();
            exchangetiles = parse_tiles(tiles);
            for (size_t i = 0; i < exchangetiles.size(); i++) {
                if (exchangetiles[i - 1].letter == '?') {
                    continue;
                }
                if (!has_tile(exchangetiles[i])) {
                    // trying to exchange a tile they don't have.
                    throw 0;
                }
            }
            Move exchangemove(exchangetiles);
            return exchangemove;
        } else if (movetype[1] == 'L') {
            char direction_;
            Move new_move;
            cin >> direction_;
            if (direction_ == '-') {
                // across;
                int ac_row = 0, ac_column = 0;
                string ac_string;
                cin >> ac_row >> ac_column >> ac_string;
                cin.ignore();
                cout << ac_string << endl;
                vector<TileKind> ac_tiles = parse_tiles(ac_string);
                Move ac_move(ac_tiles, ac_row - 1, ac_column - 1, Direction::ACROSS);
                PlaceResult testing1 = board.test_place(ac_move);
                vector<string> test_words = testing1.words;
                for (size_t j = 0; j < test_words.size(); j++) {
                    if (!dictionary.is_word(test_words[j])) {
                        // word created isn't a word in dictionary.
                        throw - 1;
                    }
                }
                return ac_move;
            } else if (direction_ == '|') {
                // down
                int do_row = 0, do_column = 0;
                string do_string;
                cin >> do_row >> do_column >> do_string;
                cin.ignore();
                vector<TileKind> do_tiles = parse_tiles(do_string);
                Move do_move(do_tiles, do_row - 1, do_column - 1, Direction::DOWN);
                PlaceResult testing2 = board.test_place(do_move);
                vector<string> test_words = testing2.words;
                for (size_t j = 0; j < test_words.size(); j++) {
                    if (!dictionary.is_word(test_words[j])) {
                        // word created isnt a word in dictionary.
                        throw - 2;
                    }
                }
                return do_move;
            } else {
                // invalid direction;
                throw - 3;
            }
        }
        throw - 4;
    } catch (int x) {
        switch (x) {
        case 0:
            cout << "Player is trying to exchange a tile they do not have." << endl;
            cout << "Please enter a valid move" << endl;
            return get_move(board, dictionary);
        case -1:
            cout << "Player has placed down a word that is not in the dictionary or has modified another word such that"
                 << endl;
            cout << "it is not in the dictionary." << endl;
            cout << "Please enter a valid move" << endl;
            return get_move(board, dictionary);
        case -2:
            cout << "Player has placed down a word that is not in the dictionary or has modified another word such that"
                 << endl;
            cout << "it is not in the dictionary." << endl;
            cout << "Please enter a valid move" << endl;
            return get_move(board, dictionary);
        case -3:
            cout << "Player has not entered a valid direction" << endl;
            cout << "Please enter a valid move" << endl;
            return get_move(board, dictionary);
        case -4:
            cout << "Please enter a valid move" << endl;
            return get_move(board, dictionary);
        }
    }
    throw "some condition not met :/";
}

vector<TileKind> HumanPlayer::parse_tiles(string& letters) const {
    // TODO: begin implementation here.
    vector<TileKind> returntiles;
    for (size_t i = 0; i < letters.size(); i++) {
        char new_letter = letters[i];
        if (letters[i] != '?') {
            TileKind new_tile = tiles.lookup_tile(new_letter);
            returntiles.push_back(new_tile);
        } else if (letters[i] == '?') {
            TileKind blank(letters[i], 0, letters[i + 1]);
            returntiles.push_back(blank);
            i++;
        }
    }
    return returntiles;
}

// Move HumanPlayer::parse_move(string& move_string) const {
// TODO: begin implementation here.
//}

void HumanPlayer::print_hand(ostream& out) const {
    const size_t tile_count = tiles.count_tiles();
    const size_t empty_tile_count = this->get_hand_size() - tile_count;
    const size_t empty_tile_width = empty_tile_count * (SQUARE_OUTER_WIDTH - 1);

    for (size_t i = 0; i < HAND_TOP_MARGIN - 2; ++i) {
        out << endl;
    }

    out << repeat(SPACE, HAND_LEFT_MARGIN) << FG_COLOR_HEADING << "Your Hand: " << endl << endl;

    // Draw top line
    out << repeat(SPACE, HAND_LEFT_MARGIN) << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE;
    print_horizontal(tile_count, L_TOP_LEFT, T_DOWN, L_TOP_RIGHT, out);
    out << repeat(SPACE, empty_tile_width) << BG_COLOR_OUTSIDE_BOARD << endl;

    // Draw middle 3 lines
    for (size_t line = 0; line < SQUARE_INNER_HEIGHT; ++line) {
        out << FG_COLOR_LABEL << BG_COLOR_OUTSIDE_BOARD << repeat(SPACE, HAND_LEFT_MARGIN);
        for (auto it = tiles.cbegin(); it != tiles.cend(); ++it) {
            out << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE << I_VERTICAL << BG_COLOR_PLAYER_HAND;

            // Print letter
            if (line == 1) {
                out << repeat(SPACE, 2) << FG_COLOR_LETTER << (char)toupper(it->letter) << repeat(SPACE, 2);

                // Print score in bottom right
            } else if (line == SQUARE_INNER_HEIGHT - 1) {
                out << FG_COLOR_SCORE << repeat(SPACE, SQUARE_INNER_WIDTH - 2) << setw(2) << it->points;

            } else {
                out << repeat(SPACE, SQUARE_INNER_WIDTH);
            }
        }
        if (tiles.count_tiles() > 0) {
            out << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE << I_VERTICAL;
            out << repeat(SPACE, empty_tile_width) << BG_COLOR_OUTSIDE_BOARD << endl;
        }
    }

    // Draw bottom line
    out << repeat(SPACE, HAND_LEFT_MARGIN) << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE;
    print_horizontal(tile_count, L_BOTTOM_LEFT, T_UP, L_BOTTOM_RIGHT, out);
    out << repeat(SPACE, empty_tile_width) << rang::style::reset << endl;
}
