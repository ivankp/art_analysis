#include <iostream>
#include <iomanip>
#include <fstream>
#include <deque>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <ctime>

#include <TFile.h>
#include <TTree.h>

#include <ImageMagick/Magick++.h>

#include "running_stat.h"

using namespace std;

// mutexes
mutex info_mutex;
mutex img_files_mutex;
mutex tree_mutex;

// timing
time_t last_time = time(0);
int num_sec = 0; // seconds elapsed

// Tree
TTree *tree;

// Branches
Float_t mean_red, mean_green, mean_blue, mean_grey, mean_lum,
        var_red,  var_green,  var_blue,  var_grey, var_lum;

//---------------------------------------------------------
// Process images and fill histograms
//---------------------------------------------------------
void process_image( deque<string>& img_files ) noexcept {

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
    running_stat red_stat, green_stat, blue_stat, grey_stat, lum_stat;

    for (size_t h=0;h<height;++h) {
      for (size_t w=0;w<width;++w) {

        // Get pixel color
        const Magick::Color c(img.pixelColor(w,h));

        // Get pixel RGB info
        Magick::ColorRGB rgb(c);

          red_stat.push( rgb.red  () );
        green_stat.push( rgb.green() );
         blue_stat.push( rgb.blue () );

          lum_stat.push( 0.27*rgb.red() + 0.67*rgb.green() + 0.06*rgb.blue() );

        // Get pixel Gray Scale
        Magick::ColorGray grey(c);

        grey_stat.push( grey.shade() );

      }
    }

    // Fill histograms --------------------------
    // lock mutex
    tree_mutex.lock();

    // mean
    mean_red   =   red_stat.mean();
    mean_green = green_stat.mean();
    mean_blue  =  blue_stat.mean();
    mean_grey  =  grey_stat.mean();
    mean_lum   =   lum_stat.mean();

    //variance
    var_red   =   red_stat.var();
    var_green = green_stat.var();
    var_blue  =  blue_stat.var();
    var_grey  =  grey_stat.var();
    var_lum   =   lum_stat.var();

    tree->Fill();

    // unlock mutex
    tree_mutex.unlock();
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
  tree = new TTree("variables","");

  // Book histograms
  tree->Branch("mean_red",&mean_red,"mean_red/F");
  tree->Branch("mean_green",&mean_green,"mean_green/F");
  tree->Branch("mean_blue",&mean_blue,"mean_blue/F");
  tree->Branch("mean_grey",&mean_grey,"mean_grey/F");
  tree->Branch("mean_lum",&mean_lum,"mean_lum/F");

  tree->Branch("var_red",&var_red,"var_red/F");
  tree->Branch("var_green",&var_green,"var_green/F");
  tree->Branch("var_blue",&var_blue,"var_blue/F");
  tree->Branch("var_grey",&var_grey,"var_grey/F");
  tree->Branch("var_lum",&var_lum,"var_lum/F");

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

  // write output file
  f->Write();
  cout << "Wrote root file " << f->GetName() << endl;

  // close output file
  f->Close();
  delete f;

  return 0;
}
