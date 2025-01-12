#include "DrawingBoard.h"

#include <iostream>

DrawingBoard::DrawingBoard()
    : pixels(width, std::vector<std::string>(height, "")) {}

void DrawingBoard::reset()
{
    // for (auto &row : pixels)
    // {
    //     std::fill(row.begin(), row.end(), "");
    // }
    changed_pixels.clear();
}

void DrawingBoard::setPixel(int x, int y, const std::string &color)
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        pixels[x][y] = color;
        changed_pixels.emplace_back(x, y, color); // Rejestracja zmienionego piksela
    }
    else
    {
        std::cerr << "setPixel: Invalid index (" << x << ", " << y << ") for size (" << width << ", " << height << ")" << std::endl;
    }
}

nlohmann::json DrawingBoard::toJSON() const
{
    nlohmann::json j = {
        {"width", width},
        {"height", height},
        {"pixels", pixels}};
    return j;
}
nlohmann::json DrawingBoard::getChangedPixels() const
{
    nlohmann::json changes = nlohmann::json::array();
    for (const auto &[x, y, color] : changed_pixels)
    {
        changes.push_back({{"x", x}, {"y", y}, {"color", color}});
    }
    return changes;
}
