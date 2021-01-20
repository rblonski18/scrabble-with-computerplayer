
#include "computer_player.h"

#include <iostream>
#include <memory>
#include <stack>
#include <string>

void ComputerPlayer::left_part(
        Board::Position anchor_pos,
        std::string partial_word,
        Direction dir,
        Move partial_move,
        std::shared_ptr<Dictionary::TrieNode> node,
        size_t limit,
        TileCollection& remaining_tiles,
        std::vector<Move>& legal_moves,
        const Board& board,
        const Dictionary& dictionary) const {

    // looks for all possible prefixes we can make for a given word, should be recursing until limit == 0;
    // left/right corresponds to up/down without loss of generality; this is left/up prefix
    // base case is when limit == 0
    Board::Position consider(partial_move.row, partial_move.column);
    if (!board.is_in_bounds(consider)) // better be in bounds 
        return;

    if (dir == Direction::ACROSS) // compensate for zero-based indexing in extend right
        anchor_pos.column--;
    else
        anchor_pos.row--;

    extend_right(anchor_pos, partial_word, dir, partial_move, node, remaining_tiles, legal_moves, board, dictionary); // check for extend right 

    if (dir == Direction::ACROSS) // return indexing
        anchor_pos.column++;
    else
        anchor_pos.row++;

    if (limit == 0) // no other checks to be done
        return;

    std::shared_ptr<Dictionary::TrieNode> current_Node = node; // grab current

    std::vector<char> lettersToSearch = dictionary.next_letters(
            partial_word);  // if empty word, should reference root and return the next letters of root

    if (partial_move.direction == Direction::DOWN) // indexing
        partial_move.row--;
    else
        partial_move.column--;

    for (size_t i = 0; i < lettersToSearch.size(); i++) {
        try {

            TileKind hasTile
                    = remaining_tiles.lookup_tile(lettersToSearch[i]);  // do we have the tile?, if not, will throw
                                                                        // exception and rest of statement wont execute

            partial_word += lettersToSearch[i];  // add tile to partial word

            remaining_tiles.remove_tile(hasTile);  // temporarily remove tile from hand

            partial_move.tiles.push_back(hasTile);  // add tile to partial move.

            current_Node = dictionary.find_prefix(partial_word);  // find its node in the trie
            if (current_Node == nullptr)
                return;
            left_part(
                    anchor_pos,
                    partial_word,
                    dir,
                    partial_move,
                    current_Node,
                    limit - 1,
                    remaining_tiles,
                    legal_moves,
                    board,
                    dictionary);

            remaining_tiles.add_tile(hasTile);  // add tile back when finished with recursive call.

            partial_word.pop_back();  // remove last tile from the word

            partial_move.tiles.pop_back();  // remove tile we temporaril put in move.
        } catch (std::out_of_range& e) {
            try {
                TileKind hasBlank = remaining_tiles.lookup_tile(
                        '?');  // do we have a blank? if not, will throw exception and rest of statement wont execute

                hasBlank.assigned = lettersToSearch[i];

                partial_word += lettersToSearch[i];  // add tile to partial word

                partial_move.tiles.push_back(hasBlank);  // add tile to partial move.

                remaining_tiles.remove_tile(hasBlank);  // temporarily remove tile from hand

                current_Node = dictionary.find_prefix(partial_word);  // find its node in the trie,
                if (current_Node == nullptr)
                    return;
                left_part(
                        anchor_pos,
                        partial_word,
                        dir,
                        partial_move,
                        current_Node,
                        limit - 1,
                        remaining_tiles,
                        legal_moves,
                        board,
                        dictionary);

                remaining_tiles.add_tile(hasBlank);  // add tile back when finished with recursive call.

                partial_word.pop_back();  // remove last tile from the word

                partial_move.tiles.pop_back();
            } catch (std::out_of_range& e) {
                continue;  // we are assuming that if we don't have a tile it will throw an exception - great, just move
                           // on.
            }
        }
    }
}

void ComputerPlayer::extend_right(
        Board::Position square,
        std::string partial_word,
        Direction dir,
        Move partial_move,
        std::shared_ptr<Dictionary::TrieNode> node,
        TileCollection& remaining_tiles,
        std::vector<Move>& legal_moves,
        const Board& board,
        const Dictionary& dictionary) const {

    if (partial_move.direction == Direction::DOWN) {

        Board::Position leftPos = square; // grab left
        leftPos.column--;
        Board::Position rightPos = square; // grab right
        rightPos.column++;

        char backofPartial = partial_word.back();
        if (board.in_bounds_and_has_tile(leftPos) || board.in_bounds_and_has_tile(rightPos)) { // make sure in bounds
            std::string validCheck = getWord(square, board, dictionary, partial_move.direction, backofPartial, dir); // grab word
            if (!dictionary.is_word(validCheck)) { // if not valid, return
                return;
            }
        }
    } else {
        Board::Position northPos = square; // grab up
        northPos.row--;
        Board::Position southPos = square; // grab down
        southPos.row++;

        char backofPartial = partial_word.back();
        if (board.in_bounds_and_has_tile(northPos) || board.in_bounds_and_has_tile(southPos)) { // make sure in bounds
            std::string validCheck = getWord(square, board, dictionary, partial_move.direction, backofPartial, dir); // grab words
            if (!dictionary.is_word(validCheck)) { // if not valid, return
                return;
            }
        }
    }

    if (dir == Direction::ACROSS) // increment square
        square.column++;
    else
        square.row++;

    if (dictionary.is_word(partial_word)) { // if word
        if (board.test_place(partial_move).points < 1000) { // avoids heisenbug
            if (wordOnEnd(partial_move, board, dictionary, partial_word)) {
                legal_moves.push_back(partial_move);  // if we have created a word from left_part, add to legal moves.
            }
        }
    }

    if (!board.is_in_bounds(square))
        return;  // position isn't in bounds, stop searching.

    // case 1: next square is vacant
    if (!board.in_bounds_and_has_tile(square)) {

        // this function basically 'extends' our current partial word to see if we can form any words. 
        // if so, we add them to the list of possible moves. 

        std::shared_ptr<Dictionary::TrieNode> current_Node = node;
        std::vector<char> lettersToSearch = dictionary.next_letters(partial_word);

        for (size_t j = 0; j < lettersToSearch.size(); j++) {

            try {
                // test
                TileKind currentTile = remaining_tiles.lookup_tile(lettersToSearch[j]);

                partial_word += lettersToSearch[j];

                partial_move.tiles.push_back(currentTile);

                remaining_tiles.remove_tile(currentTile);

                current_Node = dictionary.find_prefix(partial_word);
                if (current_Node == nullptr)
                    return;

                extend_right(
                        square,
                        partial_word,
                        dir,
                        partial_move,
                        current_Node,
                        remaining_tiles,
                        legal_moves,
                        board,
                        dictionary); // continue to try and extend. will add words if they arise, and then continue to backtrack and try other letters. 


                //put things back to backtrack. 
                remaining_tiles.add_tile(currentTile);

                partial_word.pop_back();

                partial_move.tiles.pop_back();

            } catch (std::out_of_range& e) { // out of range exception throws on a blanktile, same method as above repeated, just with blanktile. 

                try {
                    TileKind currentTile1 = remaining_tiles.lookup_tile('?');

                    currentTile1.assigned = lettersToSearch[j];

                    partial_word += lettersToSearch[j];

                    partial_move.tiles.push_back(currentTile1);

                    remaining_tiles.remove_tile(currentTile1);

                    current_Node = dictionary.find_prefix(partial_word);
                    if (current_Node == nullptr)
                        return;
                    extend_right(
                            square,
                            partial_word,
                            dir,
                            partial_move,
                            current_Node,
                            remaining_tiles,
                            legal_moves,
                            board,
                            dictionary);

                    remaining_tiles.add_tile(currentTile1);

                    partial_word.pop_back();

                    partial_move.tiles.pop_back();

                } catch (std::out_of_range& e) { // if blanktile doesnt work, we've exhausted lookup tiles. 
                    continue;
                }
            }
        }
    } else {  // case 2: next square is occupied
        char potentialLetter = board.letter_at(square);
        if (node->nexts.find(potentialLetter) != node->nexts.end()) {  // we can make words with this letter
            partial_word += potentialLetter;
            std::shared_ptr<Dictionary::TrieNode> movingForward = dictionary.find_prefix(partial_word);
            if (movingForward == nullptr)
                return;
            extend_right(
                    square,
                    partial_word,
                    dir,
                    partial_move,
                    movingForward,
                    remaining_tiles,
                    legal_moves,
                    board,
                    dictionary);
        }
    }
}

Move ComputerPlayer::get_move(const Board& board, const Dictionary& dictionary) const {
    std::vector<Move> legal_moves;
    std::vector<Board::Anchor> anchors = board.get_anchors();

    for (size_t anchorIterator = 0; anchorIterator < anchors.size(); anchorIterator++) {  // iterate through anchors
        std::vector<TileKind> partialMoveTiles = {};  // initialize vector of tiles for partial move
        std::stack<char> prev;  // initialize for string that is potentially behind or above anchor
        std::string previous;
        if (anchors[anchorIterator].limit == 0) {  // word up or to the left

            char singleChar;  // initialize single character

            Board::Position currentPos = anchors[anchorIterator].position;  // grab current position of anchor

            if (anchors[anchorIterator].direction == Direction::DOWN) {  // string above.

                currentPos.row--;  // move up to where the word begins

                while (board.in_bounds_and_has_tile(
                        currentPos)) {  // while we're looking at positions that contain tiles

                    singleChar = board.letter_at(currentPos);  // grab tile,

                    prev.push(singleChar);  // push to stack

                    currentPos.row--;  // move up
                }
            } else {

                currentPos.column--;  // to the left

                while (board.in_bounds_and_has_tile(currentPos)) {  // while there is letters

                    singleChar = board.letter_at(currentPos);  // grab them

                    prev.push(singleChar);  // push front

                    currentPos.column--;  // move to the left
                }
            }
            while (!prev.empty()) { // grab letters from stack. 
                previous += prev.top();
                prev.pop();
            }

            std::shared_ptr<Dictionary::TrieNode> prefix = dictionary.find_prefix(previous); // check if prefix leads to anything in trie. 
            Board::Position moveBack = anchors[anchorIterator].position;
            if (anchors[anchorIterator].direction == Direction::DOWN)
                moveBack.row -= previous.size();
            else
                moveBack.column -= previous.size();

            TileCollection tilesLeft = this->tiles; // check our tiles
            Move newPartialMove(
                    partialMoveTiles,
                    anchors[anchorIterator].position.row,
                    anchors[anchorIterator].position.column,
                    anchors[anchorIterator].direction); // declare new move.
            extend_right(
                    moveBack,
                    previous,
                    anchors[anchorIterator].direction,
                    newPartialMove,
                    prefix,
                    tilesLeft,
                    legal_moves,
                    board,
                    dictionary); // continue extending right
            continue;
        }

        std::shared_ptr<Dictionary::TrieNode> root = dictionary.get_root(); // otherwise grab root node in trie
        TileCollection tilesLeft = this->tiles;
        Move newPartialMove(
                partialMoveTiles,
                anchors[anchorIterator].position.row,
                anchors[anchorIterator].position.column,
                anchors[anchorIterator].direction);
        left_part(
                anchors[anchorIterator].position,
                previous,
                anchors[anchorIterator].direction,
                newPartialMove,
                root,
                anchors[anchorIterator].limit,
                tilesLeft,
                legal_moves,
                board,
                dictionary); // try left part
    }
    return get_best_move(legal_moves, board, dictionary);
}

Move ComputerPlayer::get_best_move(
        std::vector<Move> legal_moves, const Board& board, const Dictionary& dictionary) const {
    Move best_move = Move();  // Pass if no move found
    size_t bestMoveScore = 0;
    for (Move m : legal_moves) { // iterates over moves and checks point values, selecting highest one. 
        PlaceResult result = board.test_place(m);
        if (result.points > bestMoveScore && result.points < 1000) { // update highest score and compensate for heisenbug
            best_move = m;
            bestMoveScore = result.points;
        }
    }
    return best_move;
}

std::string ComputerPlayer::getWord(
        Board::Position position,
        const Board& board,
        const Dictionary& dictionary,
        Direction direction,
        char back,
        Direction dir) const {

    if (!board.is_in_bounds(position))
        return "";
    Board::Position currentPosition = position;
    std::stack<char> charStack; // stack for pushing on characters
    std::string returnString;
    // case 1: above

    if (direction == Direction::ACROSS) {
        currentPosition.row--;  // go to where the word begins.
        while (board.in_bounds_and_has_tile(currentPosition)) {
            charStack.push(board.letter_at(currentPosition));
            currentPosition.row--;
        }
        while (!charStack.empty()) { // pop off of stack onto word
            returnString += charStack.top();
            charStack.pop();
        }
        returnString += back;
        currentPosition.row = position.row + 1;  // go to where potential word below us begins
        while (board.in_bounds_and_has_tile(currentPosition)) {
            returnString += board.letter_at(currentPosition);
            currentPosition.row++;
        }
        if (returnString.size() == 1) {  // this means there wasn't actually a word formed.
            return "";
        }
    } else {
        currentPosition.column--;  // go to where the word begins.
        while (board.in_bounds_and_has_tile(currentPosition)) {
            charStack.push(board.letter_at(currentPosition));
            currentPosition.column--;
        }
        while (!charStack.empty()) { // pop off stack. 
            returnString += charStack.top();
            charStack.pop();
        }
        returnString += back;
        currentPosition.column = position.column + 1;  // go to where potential word below us begins
        while (board.in_bounds_and_has_tile(currentPosition)) {
            returnString += board.letter_at(currentPosition);
            currentPosition.column++;
        }
        if (returnString.size() == 1) {  // this means there wasn't actually a word formed.
            return "";
        }
    }
    return returnString;
}

bool ComputerPlayer::wordOnEnd(
        Move& partial_move, const Board& board, const Dictionary dictionary, std::string partial_word) const {
    // where is the end of the word?
    // go to where move begins, add partial_word size;

    Board::Position toLook(partial_move.row, partial_move.column);
    if (partial_move.direction == Direction::DOWN)
        toLook.row += partial_move.tiles.size();
    else
        toLook.column += partial_move.tiles.size();

    // toLook is now at end of word,

    if (!board.is_in_bounds(toLook))
        return true;  // if we're at the edge and looking out of bounds, no need to worry.

    while (board.in_bounds_and_has_tile(toLook)) {
        partial_word += board.letter_at(toLook);
        // down
        if (partial_move.direction == Direction::DOWN)
            toLook.row++;
        else
            toLook.column++;
    }
    if (dictionary.is_word(partial_word))
        return true;
    else
        return false;
}