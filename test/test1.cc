#include <iostream>

#include <ImageMagick/Magick++.h>

using namespace std;

int main(int argc, char** argv)
{
  Magick::Image img;
  img.read("img/test.jpg");

  Magick::Geometry geom = img.size();
  cout << "width: " << geom.width()
       << " height: " << geom.height() << endl;

  for(int i=1;i<10;++i) {
    Magick::ColorRGB rgb(img.pixelColor(i,0));
    cout << rgb.red() << ' '
         << rgb.green() << ' '
         << rgb.blue() << endl;
  }

  return 0;
}
