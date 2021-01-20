#include "board.h"

#include "board_square.h"
#include "exceptions.h"
#include "formatting.h"
#include <fstream>
#include <iomanip>
#include <stack>

using namespace std;

/* This is the file for methods related to performing operations with the board. 
It includes functions to evaluate the validity of moves and to score them, 
methods to establish anchors for the computer player to use for it's algorithm, 
and methods for display. */

bool Board::Position::operator==(const Board::Position& other) const {
    return this->row == other.row && this->column == other.column;
}

bool Board::Position::operator!=(const Board::Position& other) const {
    return this->row != other.row || this->column != other.column;
}

Board::Position Board::Position::translate(Direction direction) const { return this->translate(direction, 1); }

Board::Position Board::Position::translate(Direction direction, ssize_t distance) const {
    if (direction == Direction::DOWN) {
        return Board::Position(this->row + distance, this->column);
    } else {
        return Board::Position(this->row, this->column + distance);
    }
}

Board Board::read(const string& file_path) {
    ifstream file(file_path);
    if (!file) {
        throw FileException("cannot open board file!");
    }

    size_t rows;
    size_t columns;
    size_t starting_row;
    size_t starting_column;
    file >> rows >> columns >> starting_row >> starting_column;
    Board board(rows, columns, starting_row, starting_column);

    for (size_t i = 0; i < rows; i++) {
    	// the board is a vector we fill with board squares. 
        vector<BoardSquare> new_row;
        board.squares.push_back(new_row);
        for (size_t j = 0; j < columns; j++) {
            char character;
            file >> character;
            if (character == '.') {
                BoardSquare new_square(1, 1);
                board.squares[i].push_back(new_square);
                continue;
            } else if (character == '2') {
                BoardSquare new_square(2, 1);
                board.squares[i].push_back(new_square);
                continue;
            } else if (character == '3') {
                BoardSquare new_square(3, 1);
                board.squares[i].push_back(new_square);
                continue;
            } else if (character == 'd') {
                BoardSquare new_square(1, 2);
                board.squares[i].push_back(new_square);
                continue;
            } else if (character == 't') {
                BoardSquare new_square(1, 3);
                board.squares[i].push_back(new_square);
                continue;
            }
        }
    }

    return board;
}

size_t Board::get_move_index() const { return this->move_index; }

PlaceResult Board::test_place(const Move& move) const {

    /* this is the function for testing the validity of a move. It tests
    to see if the word is placed adjacent to a word (or on the starting position if move 1),
    finds the score of the word with any multipliers, and scores of words it forms with adjacent words with
    multipliers */

    vector<string> word_collection; // words we make with our move
    int wordpoints = 0; // points for the word we directly place down
    int wordpoints2 = 0; // points for adjacent words
    int mult = 1; // any multiplier is here, begins as 1
    bool adjacency = false; // assume no adjacency until found otherwise

    if (move_index == 0)
        adjacency = true; // if move 1, we don't have to check for lack of adjacency, just that we're over the starting square

    Board::Position check(move.row, move.column);
    if (move.direction == Direction::ACROSS) { // check if in bounds
        if (move.tiles.size() + check.column > 15) {
            PlaceResult error("Out of Bounds");
            return error;
        } else if (move.direction == Direction::DOWN) {
            if (move.tiles.size() + check.row > 15) {
                PlaceResult error("Out of Bounds");
                return error;
            }
        }
    }


    if (in_bounds_and_has_tile(check)) { // need to make sure move begins at an empty square
        PlaceResult error("Move begins at square that already contains tile");
        return error;
    }


    int lettercount = move.tiles.size(); 
    string currentword;
    if (get_move_index() == 0) {
    	// checking for the starting position, the direction we check depends on whether the move is across or down, 
    	// so that's the purpose of the following if statement
        if (move.row != this->start.row || move.column != this->start.column) {
            if (move.direction == Direction::ACROSS) {
                if (move.row != this->start.row || move.column + lettercount < start.column) {
                    PlaceResult error("Not starting poisition");
                    return error;
                }
            } else if (move.direction == Direction::DOWN) {
                if (move.column != this->start.column || move.row + lettercount < start.column) {
                    PlaceResult error("Not starting poisition");
                    return error;
                }
            }
        }
    }


    // this is a for loop (I should have modularized a lot of the code in here but I learned that lesson from this project) that is the main
    // check for points and adjacent words.
    int x = 0; // (poorly-named) variable that is a tally of tiles that are in between the letters of our word. Consider the word "one" is placed.
    		   // I can play the move "CS" at the space before "one" and create the word "cones". In this case, x = 3, the number of tiles passed over. 
    for (int i = 0; i < lettercount; i++) {
        // need to check first if we're out of bounds
        Board::Position current(
                move.direction == Direction::ACROSS ? move.row : move.row + i + x,
                move.direction == Direction::DOWN ? move.column : move.column + i + x);

        bool blanktile = false; // flag for blank tiles

        while (in_bounds_and_has_tile(current)) {
            // if theres a tile, we should want to move i forward one, but still consider each letter.
            adjacency = true;
            wordpoints += this->at(current).get_tile_kind().points;
            currentword += this->at(current).get_tile_kind().letter;
            move.direction == Direction::ACROSS ? current.column++ : current.row++;
            x++;
        }

        if (is_in_bounds(current)) {
            // all good, we're in bounds

            if (move.tiles[i].letter == '?') {
                blanktile = true;
            }

            if (move.direction == Direction::ACROSS) { // first checking across
                int amult = 1;

                Board::Position current_up(move.row - 1, move.column + i + x); // letter above
                Board::Position current_down(move.row + 1, move.column + i + x); // letter below

                stack<char> word_up; // since letters above will be gathered backwards, push onto a stack and back off to form actual word. 
                string new_word; // word stored here
                unsigned int new_word_points = 0; 

                while (in_bounds_and_has_tile(current_up)) { // only if letter above != null
                    word_up.push(this->at(current_up).get_tile_kind().letter);
                    new_word_points += this->at(current_up).get_tile_kind().points;
                    current_up.row--;
                }

                new_word += blanktile ? move.tiles[i].assigned : move.tiles[i].letter; // if blanktile, add on the "assigned" letter
                int temp = move.tiles[i].points; // this is to include the points of the tile that we place down that is a part of the adjacent word, 
                // and include it separately in the adjacent word also. 
                temp *= this->at(current).letter_multiplier; // any letter multiplier is gathered
                new_word_points += temp;

                if (this->at(current).word_multiplier) {
                    amult *= this->at(current).word_multiplier; // word multiplier counts for adjacent words
                }

                while (!word_up.empty()) { // pop off stack into new_word to form the actual word
                    new_word += word_up.top();
                    word_up.pop();
                }

                while (in_bounds_and_has_tile(current_down)) { // no need for stack, just gather letters below and push onto new_word
                    new_word += this->at(current_down).get_tile_kind().letter;
                    new_word_points += this->at(current_down).get_tile_kind().points;
                    current_down.row++;
                    // notice there are no multipliers checked. 
                }

                if (new_word.length() > 1) { 
                	// since new_word also tagged on "temp", the tile that we placed down (that we are checking the adjacency of), 
                	// if there isn't any adjacency, the length of new_word is one - the tile we are checking the adjacency of - any longer and we've
                	// picked up actual adjacent words. 
                    adjacency = true;
                    word_collection.push_back(new_word);
                    new_word_points *= amult;
                    wordpoints2 += new_word_points;
                }
            }
            if (move.direction == Direction::DOWN) { // now checking downwards

                int dmult = 1; // multiplier

                Board::Position current_left(move.row + i + x, move.column - 1); // left space
                Board::Position current_right(move.row + i + x, move.column + 1); // right space

                stack<char> word_left; // pushing letters onto stack because they're received backwards when going left. 
                string new_word; // any adjacent words go here. 
                unsigned int new_word_points = 0;

                while (in_bounds_and_has_tile(current_left)) { // while any left letters, push onto stack. 
                    word_left.push(this->at(current_left).get_tile_kind().letter);
                    new_word_points += this->at(current_left).get_tile_kind().points;
                    current_left.column--;
                }

                while (!word_left.empty()) { // pop back off stack into new_word
                    new_word += word_left.top();
                    word_left.pop();
                }

                new_word += blanktile ? move.tiles[i].assigned : move.tiles[i].letter; // grab letter assigned to blanktile or letter we placed
                int temp = move.tiles[i].points; // as before, assign letter points to "temp" for adjacent word
                new_word_points += temp * this->at(current).letter_multiplier; // grab any multiplier

                if (this->at(current).word_multiplier) { // also grab multiplier for the word we are directly placing down
                    dmult *= this->at(current).word_multiplier;
                }

                while (in_bounds_and_has_tile(current_right)) { // no need for stack. 
                    new_word += this->at(current_right).get_tile_kind().letter;
                    new_word_points += this->at(current_right).get_tile_kind().points;
                    current_right.column++;
                }

                if (new_word.length() > 1) { // same as before - if new_word.length = 1, just the letter we're testing the adjacency of
                    adjacency = true;
                    word_collection.push_back(new_word);
                    new_word_points *= dmult;
                    wordpoints2 += new_word_points;
                }
            }


        } else {
            PlaceResult error("Out of Bounds"); // if neither if statements trigger - we're out of bounds
            return error;
        }


        if (i == 0) { 
        	// potential for word to be tagged onto the end of something. 

        	// this doesn't need to run before the other tests - the parts of the function above are tests for adjacency, this runs before
        	// the actual formation and scoring of the word directly placed down. 

            // I want to check the letter behind the start
            Board::Position back(
                    move.direction == Direction::ACROSS ? move.row : move.row - 1,
                    move.direction == Direction::DOWN ? move.column : move.column - 1); // position behind/above our word. 

            int behindpoints = 0;
            stack<char> behind; // receiving letters in reverse so need to push onto stack. 

            if (is_in_bounds(back)) {

                while (this->at(back).get_tile_kind().letter) { // push letters onto stack. 
                    behind.push(this->at(back).get_tile_kind().letter);
                    behindpoints += this->at(back).get_tile_kind().points;
                    move.direction == Direction::DOWN ? back.row -= 1 : back.row;
                    move.direction == Direction::ACROSS ? back.column -= 1 : back.column;
                }

                string behindcurrent;

                while (!behind.empty()) {
                    behindcurrent += behind.top(); // push letters back off into new word. 
                    behind.pop();
                }

                if (behindcurrent.length() > 0) { // if any letters, we're working with adjacency behind us. 
                    adjacency = true;
                    currentword += behindcurrent; // word must build upon this first. 
                    wordpoints += behindpoints;
                }
            }

        } else if (i == lettercount - 1) {
        	// potential for adjacency after word. 
            // I want to check the letter after the end
            Board::Position front(
                    move.direction == Direction::ACROSS ? move.row : move.row + i + 1 + x,
                    move.direction == Direction::DOWN ? move.column : move.column + i + 1 + x); // letter after the end of our word. 

            if (is_in_bounds(front)) { // in bounds?

                int frontpoints = 0;
                string ahead; // no need for stack

                while (this->at(front).get_tile_kind().letter) { // push into ahead
                    ahead += this->at(front).get_tile_kind().letter;
                    frontpoints += this->at(front).get_tile_kind().points;
                    move.direction == Direction::DOWN ? front.row += 1 : front.row;
                    move.direction == Direction::ACROSS ? front.column += 1 : front.row;
                }

                if (ahead.length() > 0) {
                    adjacency = true;
                    currentword += ahead; // add onto end of our word. 
                    wordpoints += frontpoints;
                }
            }
        }


        if (this->at(current).word_multiplier == 2 || this->at(current).word_multiplier == 3) { // multipliers
            if (this->at(current).word_multiplier == 2)
                mult *= 2;
            else
                mult *= 3;
        }


        currentword += blanktile ? move.tiles[i].assigned : move.tiles[i].letter; // actual place we put current letter onto word. 


        BoardSquare lettermult = this->at(current); // grab the board square, test for letter multiplier
        if (lettermult.letter_multiplier == 2 || lettermult.letter_multiplier == 3) {
            wordpoints += move.tiles[i].points * lettermult.letter_multiplier;
        } else {
            wordpoints += move.tiles[i].points;
        }
    }

    if (mult > 1) { // multipy if any. 
        wordpoints *= mult;
    }

    wordpoints += wordpoints2; // add adjacent word points

    if (lettercount > 1) { // an adjacent word would have to be at least two letters. 
        word_collection.push_back(currentword);
    }

    if (move.tiles.size() == 7) // if you play your full hand, add 50 points
        wordpoints += 50;

    if (!adjacency) { // if not adjacent to any word - not valid. 
        PlaceResult error("Not adjacent to any word");
        return error;
    }

    // otherwise, if we're all in bounds, we should be good.

    PlaceResult allgood(word_collection, wordpoints);
    return allgood;
}

PlaceResult Board::place(const Move& move) {
	// place the word on the board. 
    PlaceResult m = this->test_place(move); // calls function above. 

    if (m.valid) {  // we can place the word.

        for (size_t i = 0; i < move.tiles.size(); i++) { // loop  that places the tiles. 

            TileKind currentTile(move.tiles[i].letter, move.tiles[i].points); // initialize TileKind to place in BoardSquare

            if (move.tiles[i].letter == '?') // blank square
                currentTile.letter = move.tiles[i].assigned;

            if (move.direction == Direction::ACROSS) { 
                Board::Position current(move.row, move.column + i); 

                while (in_bounds_and_has_tile(current)) { // move past tiles that are already placed. 
                    current.column++;
                }

                this->at(current).set_tile_kind(currentTile); // set tilekind
                continue; // just for efficiency
            }
            if (move.direction == Direction::DOWN) {
                Board::Position current(move.row + i, move.column);
                while (in_bounds_and_has_tile(current)) { // move past tiles that are already placed. 
                    current.row++;
                }
                this->at(current).set_tile_kind(currentTile); // set tilekind
                continue; // efficiency
            }
        }
    }
    if (this->move_index == 0) // if starting move, move index now increments. 
        this->move_index = 1;
    return m;
}

char Board::letter_at(Board::Position p) const { // grab letter at position
    BoardSquare current = this->squares.at(p.row).at(p.column);
    TileKind result = current.get_tile_kind();
    return result.letter;
}

bool Board::is_anchor_spot(Board::Position p) const {
	// method to determine if position is an "anchor spot" (positions of adjacency) on current board. 
	// this is essential for algorithm for computer player. 

    if (p.row > this->squares.size() || p.row < 0) 
        return false;

    else if (p.column > this->squares[0].size() || p.column < 0) 
        return false;
    // it is in bounds

    if (in_bounds_and_has_tile(p))
        return false;
    // in bounds and has no tile.

    // up
    if (p.row > 0) {
        Board::Position north(p.row - 1, p.column);
        if (in_bounds_and_has_tile(north))
            return true;
    }
    // right
    if (p.column < this->squares[0].size() - 1) {
        Board::Position east(p.row, p.column + 1);
        if (in_bounds_and_has_tile(east))
            return true;
    }
    // down
    if (p.row < this->squares.size() - 1) {
        Board::Position south(p.row + 1, p.column);
        if (in_bounds_and_has_tile(south))
            return true;
    }
    // left
    if (p.column > 0) {
        Board::Position west(p.row, p.column - 1);
        if (in_bounds_and_has_tile(west))
            return true;
    }
    return false;
}

vector<Board::Anchor> Board::get_anchors() const {
	// calls function above to return vector of anchors for computer player. 

    vector<Board::Anchor> anchorVec;
    if (get_move_index() == 0) { // if computer gets first move, can move down or across, so make two anchors.
        Board::Position starting(start.row, start.column);
        Board::Anchor startAnchorD(starting, Direction::DOWN, start.row); 
        Board::Anchor startAnchorA(starting, Direction::ACROSS, start.column);
        anchorVec.push_back(startAnchorD);
        anchorVec.push_back(startAnchorA);
    }

    for (size_t i = 0; i < this->squares.size(); i++) {         // rows
        for (size_t j = 0; j < this->squares[0].size(); j++) {  // columns
            Board::Position current(i, j);                      // Position at current square.
            if (is_anchor_spot(current)) {                      // determine if we're at an anchor spot
                // determine distance from above;
                Board::Position temp(i, j);
                int countUp = 0;
                temp.row--;
                while (is_in_bounds(temp) && !in_bounds_and_has_tile(temp)
                       && !is_anchor_spot(temp)) {  // go up until we're out of bounds
                    countUp++;
                    temp.row--;
                }
                Board::Anchor anchorDown(current, Direction::DOWN, countUp);

                temp.row = i;
                temp.column = j;

                int countLeft = 0;
                temp.column--;
                while (is_in_bounds(temp) && !in_bounds_and_has_tile(temp)
                       && !is_anchor_spot(temp)) {  // go left until out of bounds
                    countLeft++;
                    temp.column--;
                }
                Board::Anchor anchorAcross(current, Direction::ACROSS, countLeft);

                anchorVec.push_back(anchorDown); // push anchors.
                anchorVec.push_back(anchorAcross);
            }
        }
    }
    return anchorVec;
}

// boardsquare getter
BoardSquare& Board::at(const Board::Position& position) { return this->squares.at(position.row).at(position.column); }

const BoardSquare& Board::at(const Board::Position& position) const {
    return this->squares.at(position.row).at(position.column);
}

// in bounds for position
bool Board::is_in_bounds(const Board::Position& position) const {
    return position.row < this->rows && position.column < this->columns;
}

// in bounds, filled
bool Board::in_bounds_and_has_tile(const Position& position) const {
    return is_in_bounds(position) && at(position).has_tile();
}

void Board::print(ostream& out) const {
    // Draw horizontal number labels
    for (size_t i = 0; i < BOARD_TOP_MARGIN - 2; ++i) {
        out << std::endl;
    }
    out << FG_COLOR_LABEL << repeat(SPACE, BOARD_LEFT_MARGIN);
    const size_t right_number_space = (SQUARE_OUTER_WIDTH - 3) / 2;
    const size_t left_number_space = (SQUARE_OUTER_WIDTH - 3) - right_number_space;
    for (size_t column = 0; column < this->columns; ++column) {
        out << repeat(SPACE, left_number_space) << std::setw(2) << column + 1 << repeat(SPACE, right_number_space);
    }
    out << std::endl;

    // Draw top line
    out << repeat(SPACE, BOARD_LEFT_MARGIN);
    print_horizontal(this->columns, L_TOP_LEFT, T_DOWN, L_TOP_RIGHT, out);
    out << endl;

    // Draw inner board
    for (size_t row = 0; row < this->rows; ++row) {
        if (row > 0) {
            out << repeat(SPACE, BOARD_LEFT_MARGIN);
            print_horizontal(this->columns, T_RIGHT, PLUS, T_LEFT, out);
            out << endl;
        }

        // Draw insides of squares
        for (size_t line = 0; line < SQUARE_INNER_HEIGHT; ++line) {
            out << FG_COLOR_LABEL << BG_COLOR_OUTSIDE_BOARD;

            // Output column number of left padding
            if (line == 1) {
                out << repeat(SPACE, BOARD_LEFT_MARGIN - 3);
                out << std::setw(2) << row + 1;
                out << SPACE;
            } else {
                out << repeat(SPACE, BOARD_LEFT_MARGIN);
            }

            // Iterate columns
            for (size_t column = 0; column < this->columns; ++column) {
                out << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE << I_VERTICAL;
                const BoardSquare& square = this->squares.at(row).at(column);
                bool is_start = this->start.row == row && this->start.column == column;

                // Figure out background color
                if (square.word_multiplier == 2) {
                    out << BG_COLOR_WORD_MULTIPLIER_2X;
                } else if (square.word_multiplier == 3) {
                    out << BG_COLOR_WORD_MULTIPLIER_3X;
                } else if (square.letter_multiplier == 2) {
                    out << BG_COLOR_LETTER_MULTIPLIER_2X;
                } else if (square.letter_multiplier == 3) {
                    out << BG_COLOR_LETTER_MULTIPLIER_3X;
                } else if (is_start) {
                    out << BG_COLOR_START_SQUARE;
                }

                // Text
                if (line == 0 && is_start) {
                    out << "  \u2605  ";
                } else if (line == 0 && square.word_multiplier > 1) {
                    out << FG_COLOR_MULTIPLIER << repeat(SPACE, SQUARE_INNER_WIDTH - 2) << 'W' << std::setw(1)
                        << square.word_multiplier;
                } else if (line == 0 && square.letter_multiplier > 1) {
                    out << FG_COLOR_MULTIPLIER << repeat(SPACE, SQUARE_INNER_WIDTH - 2) << 'L' << std::setw(1)
                        << square.letter_multiplier;
                } else if (line == 1 && square.has_tile()) {
                    char l = square.get_tile_kind().letter == TileKind::BLANK_LETTER ? square.get_tile_kind().assigned
                                                                                     : ' ';
                    out << repeat(SPACE, 2) << FG_COLOR_LETTER << square.get_tile_kind().letter << l
                        << repeat(SPACE, 1);
                } else if (line == SQUARE_INNER_HEIGHT - 1 && square.has_tile()) {
                    out << repeat(SPACE, SQUARE_INNER_WIDTH - 1) << FG_COLOR_SCORE << square.get_points();
                } else {
                    out << repeat(SPACE, SQUARE_INNER_WIDTH);
                }
            }

            // Add vertical line
            out << FG_COLOR_LINE << BG_COLOR_NORMAL_SQUARE << I_VERTICAL << BG_COLOR_OUTSIDE_BOARD << std::endl;
        }
    }

    // Draw bottom line
    out << repeat(SPACE, BOARD_LEFT_MARGIN);
    print_horizontal(this->columns, L_BOTTOM_LEFT, T_UP, L_BOTTOM_RIGHT, out);
    out << endl << rang::style::reset << std::endl;
}
