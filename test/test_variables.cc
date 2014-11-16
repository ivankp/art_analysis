#include <iostream>

#include <pthread.h>

#include <TFile.h>
#include <TH1.h>

#include <ImageMagick/Magick++.h>

using namespace std;

template<typename T> inline T sq(T x) { return x*x; }

int main(int argc, char** argv)
{
  if (argc<4) {
    cout << "Usage: " << argv[0]
         << " nthreads output.root img1 ..." << endl;
    return 0;
  }

  const short nthreads = atoi(argv[1]);
  pthread_t threads[nthreads];

  TFile *f = new TFile(argv[2],"recreate");

  TH1F* h_mean_red   = new TH1F("mean_red","",100,0,1);
  TH1F* h_mean_green = new TH1F("mean_green","",100,0,1);
  TH1F* h_mean_blue  = new TH1F("mean_blue","",100,0,1);

  for (int arg=3;arg<argc;++arg) {
    // Open image
    Magick::Image img(argv[arg]);

    cout << img.baseFilename() << endl;

    // Get image dimensions
    Magick::Geometry geom = img.size();
    size_t width  = geom.width();
    size_t height = geom.height();
    size_t num_px = width*height;

    double mean_red=0,
           mean_green=0,
           mean_blue=0;

    for (size_t h=0;h<height;++h) {
      for (size_t w=0;w<width;++w) {
        // Get pixel RGB info
        Magick::ColorRGB rgb(img.pixelColor(w,h));

        mean_red   += rgb.red();
        mean_green += rgb.green();
        mean_blue  += rgb.blue();
      }
    }

    h_mean_red  ->Fill( mean_red  /num_px );
    h_mean_green->Fill( mean_green/num_px );
    h_mean_blue ->Fill( mean_blue /num_px );
  }

  f->Write();
  f->Close();
  delete f;

  return 0;
}
