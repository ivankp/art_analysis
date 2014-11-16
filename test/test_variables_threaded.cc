#include <iostream>
#include <iomanip>
#include <deque>
#include <vector>
#include <future>
#include <mutex>
#include <functional>
#include <algorithm>

#include <TFile.h>
#include <TH1.h>

#include <ImageMagick/Magick++.h>

using namespace std;

mutex cout_mutex;
mutex img_files_mutex;
mutex hist_mutex;

// histograms
TH1F *h_mean_red, *h_mean_green, *h_mean_blue;

void process_image( deque<string>& img_files ) {
  size_t num_imgs_left;
  for ( ; ; ) {

    img_files_mutex.lock();
    num_imgs_left = img_files.size();
    if ( num_imgs_left == 0 ) {
      img_files_mutex.unlock();
      break;
    }

    const string img_file = img_files.front();
    img_files.pop_front();
    img_files_mutex.unlock();

    // Open image
    Magick::Image img(img_file);

    cout_mutex.lock();
    cout << setw(4) << num_imgs_left << " : ";
    cout << img.baseFilename() << endl;
    cout_mutex.unlock();

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

    hist_mutex.lock();
    h_mean_red  ->Fill( mean_red  /num_px );
    h_mean_green->Fill( mean_green/num_px );
    h_mean_blue ->Fill( mean_blue /num_px );
    hist_mutex.unlock();
  }
}


int main(int argc, char** argv)
{
  if (argc<3) {
    cout << "Usage: " << argv[0]
         << " output.root img1 ..." << endl;
    return 0;
  }

  deque<string> img_files;
  for (int arg=3;arg<argc;++arg) img_files.push_back( argv[arg] );

  string root_file(argv[1]);
  if ( root_file.substr(root_file.find_last_of('.')).compare(".root") ) {
    cout << "Output file name \'" << root_file
         << "\' does not have \'.root\' extension" << endl;
    return 1;
  }
  TFile *f = new TFile(argv[1],"recreate");

  h_mean_red   = new TH1F("mean_red","",100,0,1);
  h_mean_green = new TH1F("mean_green","",100,0,1);
  h_mean_blue  = new TH1F("mean_blue","",100,0,1);

  vector<thread> threads;
  for(unsigned i=0, n=max(thread::hardware_concurrency(),1u); i<n; ++i){
    threads.push_back(
      thread( process_image, ref(img_files) )
    );
  }

  for(auto& thread : threads) thread.join();

  f->Write();
  cout << "Wrote root file " << f->GetName() << endl;
  f->Close();
  delete f;

  return 0;
}
