#include <iostream>
#include <iomanip>
#include <fstream>
#include <deque>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <ctime>

#include <boost/regex.hpp>

#include <TFile.h>
#include <TTree.h>

#include "running_stat.h"

using namespace std;

#define test(var) \
  cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << endl;

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
Float_t mean_slope, var_slope, num_lines;

template<typename T>
inline T sq(const T& x) { return x*x; }
template<typename T>
inline T sumsq(const T& a, const T& b) { return sqrt(sq(a)+sq(b)); }

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

    // Open file
    ifstream mvg(img_file);

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
           << img_file << endl;
      cout.flush();
    }
    info_mutex.unlock();

    // variables to be calculated
    running_stat slope_stat;

    // regex to parce lines
    static const boost::regex patern(
      "^line[[:space:]]*([0-9.-]*),([0-9.-]*)[[:space:]]*([0-9.-]*),([0-9.-]*)"
    );
    boost::smatch result;

    string line;
    double x1,y1,x2,y2,slope;
    int _num_lines = 0;

    while ( getline(mvg,line) ) {
      if (boost::regex_search(line, result, patern)) {
        x1 = stod( string(result[1].first, result[1].second) );
        y1 = stod( string(result[2].first, result[2].second) );
        x2 = stod( string(result[3].first, result[3].second) );
        y2 = stod( string(result[4].first, result[4].second) );

        slope = (y2-y1)/(x2-x1);
        if (std::isinf(slope)) slope = 100.;

        slope_stat.push(slope);

        ++_num_lines;
      }
    }

    // Fill histograms --------------------------
    // lock mutex
    tree_mutex.lock();

    // mean
    mean_slope = slope_stat.mean();

    //variance
    var_slope = slope_stat.var();

    // int
    num_lines = _num_lines;

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
  tree->Branch("mean_slope",&mean_slope,"mean_slope/F");
  tree->Branch("var_slope",&var_slope,"var_slope/F");
  tree->Branch("num_lines",&num_lines,"num_lines/F");

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
