#include "player.h"

#include <iostream>

using namespace std;

// TODO: implement member functions

void Player::add_points(size_t points) { this->points += points; }

void Player::subtract_points(size_t points) {
    if (this->points < points) {
        this->points = 0;
    } else {
        this->points -= points;
    }
}

size_t Player::get_points() const { return this->points; }

const string& Player::get_name() const { return this->name; }

size_t Player::count_tiles() const {
    int count = 0;
    map<TileKind, int>::iterator it;
    while (it != hand.end()) {
        count += it->second;
        it++;
    }
    return count;
}

void Player::remove_tiles(const vector<TileKind>& tiles) {
    for (size_t i = 0; i < tiles.size(); i++) {
        this->hand_value -= tiles[i].points;
        this->tiles.remove_tile(tiles[i]);
        map<TileKind, int>::iterator it;
        it = this->hand.find(tiles[i]);
        if (it != this->hand.end()) {
            it->second--;
        } else {
            this->hand.erase(tiles[i]);
        }
        this->hand_size--;
    }
}

void Player::add_tiles(const vector<TileKind>& tiles) {
    for (size_t i = 0; i < tiles.size(); i++) {
        this->hand_value += tiles[i].points;
        this->tiles.add_tile(tiles[i]);
        map<TileKind, int>::iterator it;
        it = this->hand.find(tiles[i]);
        if (it != this->hand.end()) {
            this->hand[tiles[i]] += 1;
        } else {
            this->hand[tiles[i]] = 1;
        }
        this->hand_size++;
    }
}

bool Player::has_tile(TileKind tile) const {
    map<TileKind, int>::const_iterator it;
    it = this->hand.find(tile);
    return it != this->hand.end();
}

unsigned int Player::get_hand_value() const { return this->hand_value; }

size_t Player::get_hand_size() const { return this->hand_size; }