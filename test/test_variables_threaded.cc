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

#include "running_stat.h"

using namespace std;

mutex cout_mutex;
mutex img_files_mutex;
mutex hist_mutex;

// histograms pointers
TH1F *h_mean_red, *h_mean_green, *h_mean_blue,
     *h_var_red,  *h_var_green,  *h_var_blue;

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

    // variables to be calculated
    running_stat red, green, blue;

    for (size_t h=0;h<height;++h) {
      for (size_t w=0;w<width;++w) {

        // Get pixel RGB info
        Magick::ColorRGB rgb(img.pixelColor(w,h));

        red  .push( rgb.red  () );
        green.push( rgb.green() );
        blue .push( rgb.blue () );

      }
    }

    // Fill histograms --------------------------
    // lock mutex
    hist_mutex.lock();

    // mean
    h_mean_red  ->Fill( red  .mean() );
    h_mean_green->Fill( green.mean() );
    h_mean_blue ->Fill( blue .mean() );

    //variance
    h_var_red  ->Fill( red  .var() );
    h_var_green->Fill( green.var() );
    h_var_blue ->Fill( blue .var() );

    // unlock mutex
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

  h_var_red   = new TH1F("var_red","",100,0,1);
  h_var_green = new TH1F("var_green","",100,0,1);
  h_var_blue  = new TH1F("var_blue","",100,0,1);

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
