#include "scrabble.h"

#include "formatting.h"
#include <iomanip>
#include <iostream>
#include <map>

using namespace std;


Scrabble::Scrabble(const ScrabbleConfig& config)
        : hand_size(config.hand_size),
          minimum_word_length(config.minimum_word_length),
          tile_bag(TileBag::read(config.tile_bag_file_path, config.seed)),
          board(Board::read(config.board_file_path)),
          dictionary(Dictionary::read(config.dictionary_file_path)) {}

void Scrabble::add_players() { // self-explanatory
    int num_players = 0;
    char comp; // computer

    cout << "Please enter in the number of players" << endl;
    cin >> num_players;

    num_human_players = num_players;
    for (int i = 0; i < num_players; i++) { // grab names
        string name;
        cout << "Please enter in the name of player " << i << endl;
        cin >> name;
        cout << "Is this player a computer player? (y/n)" << endl;
        cin >> comp;

        if (comp == 'y') {
            num_human_players--;
            shared_ptr<Player> playerpointer = make_shared<ComputerPlayer>(name, 0); // initialize computer player
            cout << "Player " << name << " has been added. " << endl;
            vector<TileKind> new_hand = tile_bag.remove_random_tiles(7);
            playerpointer->add_tiles(new_hand);
            players.push_back(playerpointer);
        } else {
            shared_ptr<Player> playerpointer = make_shared<HumanPlayer>(name, 0);
            cout << "Player " << name << " has been added. " << endl;
            vector<TileKind> new_hand = tile_bag.remove_random_tiles(7);
            playerpointer->add_tiles(new_hand);
            players.push_back(playerpointer);
        }
    }
    cout << "Press [enter] to continue";
    cin.ignore();
}
// Game Loop should cycle through players and get and execute that players move
// until the game is over.
size_t num_passes = 0;

void Scrabble::game_loop() {
    // game loop
    size_t i = 0;
    while (1) {             // loop should run arbitrarily long
        board.print(cout);  // print the board so the player can see.
        Move playermove = players[i]->get_move(board, dictionary);
        if (playermove.kind == MoveKind::EXCHANGE) {  // player wants to exchange tiles.
            if (players[i]->is_human())
                num_passes = 0;                          // set num of consecutive passes to zero.
            players[i]->remove_tiles(playermove.tiles);  // remove their tiles
            cout << "You exchanged tiles and got new ones" << endl;
            for (size_t a = 0; a < playermove.tiles.size(); a++) {
                tile_bag.add_tile(playermove.tiles[a]);  // put them back in the bag
            }
            vector<TileKind> new_tiles = tile_bag.remove_random_tiles(
                    playermove.tiles.size());  // now remove the same amount of tiles from the bag
            players[i]->add_tiles(new_tiles);  // put them back in.
            cout << endl << "Press [enter] to continue.";
            cin.ignore();

        } else if (playermove.kind == MoveKind::PLACE) {
            if (players[i]->is_human())
                num_passes = 0;  // set num of consecutive passes to zero.
            PlaceResult result = board.place(
                    playermove);  // I believe all I need to do is place it, but this will return a PlaceResult.
            players[i]->remove_tiles(playermove.tiles);
            for (size_t a = 0; a < playermove.tiles.size(); a++) {
                tile_bag.add_tile(playermove.tiles[a]);  // put them back in the bag
            }
            vector<TileKind> new_tiles = tile_bag.remove_random_tiles(
                    playermove.tiles.size());  // now remove the same amount of tiles from the bag
            players[i]->add_tiles(new_tiles);  // put them back in.
            board.print(cout);                 // show the board after the move.
            players[i]->add_points(result.points); // yay
            cout << "You gained " << SCORE_COLOR << result.points << rang::style::reset << " points!" << endl;
            cout << "Your current score: " << SCORE_COLOR << players[i]->get_points() << rang::style::reset << endl;
            cout << endl << "Press [enter] to continue.";
            cin.ignore();
            if (players[i]->get_hand_size() == 0 && tile_bag.count_tiles() == 0) {
                return;
            }
        } else if (playermove.kind == MoveKind::PASS) { // wuss
            cout << "You decided to pass! A coward!" << endl;
            if (players[i]->is_human()) {
                num_passes++;
                if (num_passes == num_human_players)
                    return;
            }
            cout << "Press [Enter] to continue";
            cin.ignore();
        }
        i++;  // change the player we're looking at.
        if (i == players.size())
            i = 0;  // if we're at last player in players list, go back to start.
    }

    // Useful cout expressions with fancy colors. Expressions in curly braces, indicate values you supply.
    // cout << "You gained " << SCORE_COLOR << {points} << rang::style::reset << " points!" << endl;
    // cout << "Your current score: " << SCORE_COLOR << {points} << rang::style::reset << endl;
    // cout << endl << "Press [enter] to continue.";
}

// Performs final score subtraction. Players lose points for each tile in their
// hand. The player who cleared their hand receives all the points lost by the
// other players.
void Scrabble::final_subtraction(vector<shared_ptr<Player>>& plrs) {

    for (size_t i = 0; i < plrs.size(); i++) {
        if (plrs[i]->get_hand_size() == 0) {
            unsigned int points_added = 0;
            for (size_t j = 0; j < plrs.size(); j++) {
                if (j == i)
                    continue;
                else {
                    points_added += plrs[j]->get_hand_value();
                }
            }
            plrs[i]->add_points(points_added);
        } else {
            plrs[i]->subtract_points(plrs[i]->get_hand_value());
        }
    }
}


void Scrabble::print_result() {
    // Determine highest score
    size_t max_points = 0;
    for (auto player : this->players) {
        if (player->get_points() > max_points) {
            max_points = player->get_points();
        }
    }

    // Determine the winner(s) indexes
    vector<shared_ptr<Player>> winners;
    for (auto player : this->players) {
        if (player->get_points() >= max_points) {
            winners.push_back(player);
        }
    }

    cout << (winners.size() == 1 ? "Winner:" : "Winners: ");
    for (auto player : winners) {
        cout << SPACE << PLAYER_NAME_COLOR << player->get_name();
    }
    cout << rang::style::reset << endl;

    // now print score table
    cout << "Scores: " << endl;
    cout << "---------------------------------" << endl;

    // Justify all integers printed to have the same amount of character as the high score, left-padding with spaces
    cout << setw(static_cast<uint32_t>(floor(log10(max_points) + 1)));

    for (auto player : this->players) {
        cout << SCORE_COLOR << player->get_points() << rang::style::reset << " | " << PLAYER_NAME_COLOR
             << player->get_name() << rang::style::reset << endl;
    }
}

// You should not need to change this.
void Scrabble::main() {
    add_players();
    game_loop();
    final_subtraction(this->players);
    print_result();
}
