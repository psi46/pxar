#include <utility>
#include <stdint.h>
#include <vector>
namespace shapes {
  class Rectangle {
  public:
    int x0, y0, x1, y1;
    Rectangle(std::pair<uint16_t, uint16_t> x, std::pair<uint16_t, uint16_t> y);
    Rectangle(std::vector<uint16_t > values);
    ~Rectangle();
    int getLength();
    int getHeight();
    int getArea();
    void move(int dx, int dy);
  };
}
