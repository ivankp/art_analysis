#include <iostream>

#include "img_reader.h"

using namespace std;

int main(int argc, char** argv)
{
  img_reader img("img/test.jpg");

  cout << "width: " << img.width()
       << " height: " << img.height() << endl;

  for(int i=1;i<10;++i) {
    color c = img.pixel(i,0);
    cout << c.R << ' '
         << c.G << ' '
         << c.B << endl;
  }

  return 0;
}
