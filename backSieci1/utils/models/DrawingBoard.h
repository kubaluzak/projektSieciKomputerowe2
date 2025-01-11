#ifndef DRAWINGBOARD_H
#define DRAWINGBOARD_H

#include <vector>
#include <string>
#include <nlohmann/json.hpp>

struct DrawingBoard
{
    inline static const int width = 600;
    inline static const int height = 400;

    // Macierz pikseli reprezentowana jako wektor wektorów napisów (kolory)
    std::vector<std::vector<std::string>> pixels;

    DrawingBoard();

    void reset();                                          // Resetuje planszę do stanu początkowego
    void setPixel(int x, int y, const std::string &color); // Ustawia kolor piksela
    nlohmann::json getDrawingActions() const;

    nlohmann::json toJSON() const;                            // Zwraca planszę w formacie JSON
    nlohmann::json getChangedPixels() const;
};

#endif // DRAWINGBOARD_H
