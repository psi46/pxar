#include "Rectangle.h"
#include <iostream>

using namespace shapes;

Rectangle::Rectangle(std::pair<uint16_t, uint16_t> x, std::pair<uint16_t, uint16_t> y)
{
  std::cout << " called (std::pair<uint16_t, uint16_t> x, std::pair<uint16_t, uint16_t> y) constructor" << std::endl;
  x0 = x.first;
  y0 = y.first;
  x1 = x.second;
  y1 = y.second;
}

Rectangle::Rectangle(std::vector<uint16_t> values)
{
  std::cout << " called (std::vector<uint16_t> values) constructor" << std::endl;
  x0 = values.at(0);
  y0 = values.at(1);
  x1 = values.at(2);
  y1 = values.at(3);
}

Rectangle::~Rectangle()
{
}

int Rectangle::getLength()
{
    return (x1 - x0);
}

int Rectangle::getHeight()
{
    return (y1 - y0);
}

int Rectangle::getArea()
{
    return (x1 - x0) * (y1 - y0);
}

void Rectangle::move(int dx, int dy)
{
    x0 += dx;
    y0 += dy;
    x1 += dx;
    y1 += dy;
}
