#include "WordUtils.h"
#include <cstdlib>
#include <ctime>

// Definicja bazy słów
std::vector<std::string> words_database = {
    "apple", "banana", "cat", "dog", "elephant", "flower", "guitar", "house", "ice", "jungle",
    "king", "lion", "mountain", "night", "ocean", "piano", "queen", "rainbow", "sun", "tree",
    "umbrella", "vampire", "wizard", "xylophone", "yellow", "zebra", "airplane", "ball", "car",
    "doghouse", "elephant", "frog", "grape", "honey", "iceberg", "jelly", "kite", "lemon", "mouse",
    "ninja", "orange", "penguin", "quilt", "robot", "snake", "tiger", "unicorn", "vulture", "whale",
    "x-ray", "yarn", "zucchini", "backpack", "clown", "dinosaur", "ear", "football", "giraffe", "helicopter",
    "insect", "jacket", "ketchup", "lamp", "magnet", "notebook", "octopus", "pencil", "quicksand", "rocket",
    "scissors", "television", "uniform", "volcano", "window", "xmas", "yellowstone", "zeppelin", "autumn", "balloon",
    "champagne", "dove", "eagle", "festival", "grass", "hockey", "incubator", "joke", "karate", "lighthouse",
    "mushroom", "nature", "octopus", "parrot", "quicksand", "racecar", "scorpion", "tornado", "universe", "volleyball",
    "whistle", "xenon", "yoga", "zodiac"};

// Funkcja losująca jedno słowo z bazy
std::string choose_random_word(const std::vector<std::string> &words)
{
    static bool initialized = false;
    if (!initialized)
    {
        srand(static_cast<unsigned int>(time(0))); // Inicjalizacja generatora liczb losowych
        initialized = true;
    }

    int index = rand() % words.size(); // Losowanie indeksu
    return words[index];               // Zwrócenie losowego słowa
}
