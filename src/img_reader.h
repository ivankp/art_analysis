#ifndef img_reader_h
#define img_reader_h

#include <cstddef>

namespace Magick {
  class Image;
}

struct color {
  double R, G, B;
  color();
  color(double R, double G, double B);
  double diffsqrgb(const color& c) const;
};

class img_reader {
  Magick::Image *img;
  size_t _width, _height;

  void find_size();

public:
  img_reader(const char* filename);
  ~img_reader();

  size_t width() const;
  size_t height() const;

  void resize(size_t width, size_t height);

  color pixel(size_t col, size_t row) const;
};

#endif
