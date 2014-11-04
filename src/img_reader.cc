#include "img_reader.h"

#include <cmath>

#include <ImageMagick/Magick++.h>

using namespace std;

template<typename T> inline T sq(T x) { return x*x; }

color::color(): R(0.), G(0.), B(0.) { }

color::color(double R, double G, double B)
: R(R), G(G), B(B) { }

double color::diffsqrgb(const color& c) const {
  return sqrt( sq(c.R-R)+sq(c.G-G)+sq(c.B-B) );
}

img_reader::img_reader(const char* filename)
: img( new Magick::Image(filename) )
{
  find_size();
}

img_reader::~img_reader() {
  delete img;
}

void img_reader::find_size() {
  Magick::Geometry geom = img->size();
  _width  = geom.width();
  _height = geom.height();
}

size_t img_reader::width() const { return _width; }
size_t img_reader::height() const { return _height; }

void img_reader::resize(size_t width, size_t height) {
  img->zoom( Magick::Geometry(width,height) );
  find_size();
}

color img_reader::pixel(size_t col, size_t row) const {
  Magick::ColorRGB rgb(img->pixelColor(col,row));
  return color(rgb.red(),rgb.green(),rgb.blue());
}
