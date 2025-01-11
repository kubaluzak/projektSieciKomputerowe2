#ifndef WORDUTILS_H
#define WORDUTILS_H

#include <vector>
#include <string>

// Deklaracja bazy słów
extern std::vector<std::string> words_database;

// Funkcja losująca słowo z bazy
std::string choose_random_word(const std::vector<std::string> &words);

#endif // WORDUTILS_H
