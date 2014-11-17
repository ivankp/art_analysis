#include <iostream>
#include <iomanip>
#include <deque>
#include <vector>
#include <future>
#include <mutex>
#include <functional>
#include <algorithm>
#include <ctime>

#include <TFile.h>
#include <TH1.h>
#include <TAxis.h>

#include <ImageMagick/Magick++.h>

#include "running_stat.h"
#include "hist_wrap.h"

using namespace std;

// mutexes
mutex info_mutex;
mutex img_files_mutex;
mutex hist_mutex;

// timing
time_t last_time = time(0);
int num_sec = 0; // seconds elapsed

// histograms pointers
hist *h_mean_red, *h_mean_green, *h_mean_blue, *h_mean_grey,
     *h_var_red,  *h_var_green,  *h_var_blue,  *h_var_grey;

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

    // keep track of time & print current image name
    info_mutex.lock();
    {
      time_t cur_time = time(0);
      if ( difftime(cur_time,last_time) > 1 ) {
        ++num_sec;
        last_time = cur_time;
      }

      cout << setw(3) << num_sec << "s : "
           << setw(4) << num_imgs_left << " : "
           << img.baseFilename() << endl;
    }
    info_mutex.unlock();

    // Get image dimensions
    Magick::Geometry geom = img.size();
    size_t width  = geom.width();
    size_t height = geom.height();

    // variables to be calculated
    running_stat red_stat, green_stat, blue_stat, grey_stat;

    for (size_t h=0;h<height;++h) {
      for (size_t w=0;w<width;++w) {

        // Get pixel RGB info
        Magick::ColorRGB rgb(img.pixelColor(w,h));

          red_stat.push( rgb.red  () );
        green_stat.push( rgb.green() );
         blue_stat.push( rgb.blue () );

        // Get pixel Gray Scale
        Magick::ColorGray grey(img.pixelColor(w,h));

        grey_stat.push( grey.shade() );

      }
    }

    // Fill histograms --------------------------
    // lock mutex
    hist_mutex.lock();

    // mean
    h_mean_red  ->Fill(  red_stat.mean() );
    h_mean_green->Fill( green_stat.mean() );
    h_mean_blue ->Fill(  blue_stat.mean() );
    h_mean_grey ->Fill(  grey_stat.mean() );

    //variance
    h_var_red  ->Fill(   red_stat.var() );
    h_var_green->Fill( green_stat.var() );
    h_var_blue ->Fill(  blue_stat.var() );
    h_var_grey ->Fill(  grey_stat.var() );

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
  for (int arg=2;arg<argc;++arg) img_files.push_back( argv[arg] );

  // validate root output file name
  string root_file(argv[1]);
  if ( root_file.substr(root_file.find_last_of('.')).compare(".root") ) {
    cout << "Output file name \'" << root_file
         << "\' does not have \'.root\' extension" << endl;
    return 1;
  }

  // open output root file
  TFile *f = new TFile(argv[1],"recreate");

  hist::read_binnings("config/hists.bins","^(.*)_");

  // Book histograms
  h_mean_red   = new hist("mean_red");
  h_mean_green = new hist("mean_green");
  h_mean_blue  = new hist("mean_blue");
  h_mean_grey  = new hist("mean_grey");

  h_var_red   = new hist("var_red");
  h_var_green = new hist("var_green");
  h_var_blue  = new hist("var_blue");
  h_var_grey  = new hist("var_grey");

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
