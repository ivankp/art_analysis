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

// histograms pointers
TH1F *h_mean_red, *h_mean_green, *h_mean_blue;

//---------------------------------------------------------
// Process images and fill histograms
//---------------------------------------------------------
void process_image( deque<string>& img_files ) {

  size_t num_imgs_left; // number of images still in the deque
  for ( ; ; ) {

    // check if deque is empty
    img_files_mutex.lock();
    num_imgs_left = img_files.size();
    if ( num_imgs_left == 0 ) {
      img_files_mutex.unlock();
      break;
    }

    // pop next image of the deque
    const string img_file = img_files.front();
    img_files.pop_front();
    img_files_mutex.unlock();

    // Open image
    Magick::Image img(img_file);

    // print current image name
    cout_mutex.lock();
    cout << setw(4) << num_imgs_left << " : ";
    cout << img.baseFilename() << endl;
    cout_mutex.unlock();

    // Get image dimensions
    Magick::Geometry geom = img.size();
    size_t width  = geom.width();
    size_t height = geom.height();
    size_t num_px = width*height;

    // variables to be calculated
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

    // Fill histograms
    hist_mutex.lock();
    h_mean_red  ->Fill( mean_red  /num_px );
    h_mean_green->Fill( mean_green/num_px );
    h_mean_blue ->Fill( mean_blue /num_px );
    hist_mutex.unlock();
  }
}

//---------------------------------------------------------
// MAIN
//---------------------------------------------------------
int main(int argc, char** argv)
{
  // parse arguments
  if (argc<3) {
    cout << "Usage: " << argv[0]
         << " output.root img1 ..." << endl;
    return 0;
  }

  // add image files names to deque
  deque<string> img_files;
  for (int arg=3;arg<argc;++arg) img_files.push_back( argv[arg] );

  // validate root output file name
  string root_file(argv[1]);
  if ( root_file.substr(root_file.find_last_of('.')).compare(".root") ) {
    cout << "Output file name \'" << root_file
         << "\' does not have \'.root\' extension" << endl;
    return 1;
  }

  // open output root file
  TFile *f = new TFile(argv[1],"recreate");

  // Book histograms
  h_mean_red   = new TH1F("mean_red","",100,0,1);
  h_mean_green = new TH1F("mean_green","",100,0,1);
  h_mean_blue  = new TH1F("mean_blue","",100,0,1);

  // process images in multiple threads
  const unsigned num_threads = max(thread::hardware_concurrency(),1u);
  cout << "Using " << num_threads << " threads" << endl;
  vector<thread> threads;
  threads.reserve(num_threads);
  for(unsigned i=0; i<num_threads; ++i){
    threads.push_back(
      thread( process_image, ref(img_files) )
    );
  }

  // join threads so that program doesn't quit before they are done
  for(auto& thread : threads) thread.join();

  // write output file and close it
  f->Write();
  cout << "Wrote root file " << f->GetName() << endl;
  f->Close();
  delete f;

  return 0;
}
