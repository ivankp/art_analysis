#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <queue>
#include <deque>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <ctime>

#include <TFile.h>
#include <TTree.h>
#include <TH1.h>

#include <ImageMagick/Magick++.h>

using namespace std;

// mutexes
mutex info_mutex;
mutex deque_mutex;
mutex result_mutex;

// timing
time_t last_time = time(0);
int num_sec = 0; // seconds elapsed

// Tree
TTree *tree;

template<typename T>
inline T sq(const T& x) { return x*x; }
template<typename T>
inline T sumsq(const T& a, const T& b) { return sqrt(sq(a)+sq(b)); }

double Entropy(const TH1F& h) {
  double ent = 0.;
  double np = h.GetEntries();
  for (int i=1,n=h.GetNbinsX();i<=n;++i) {
    double p = h.GetBinContent(i)/np;
    if (p<=0.) continue;
    ent += -p*log(p);
  }
  return ent;
}

// Branches
struct stats {
  Float_t mean, var, skew, kurt, ent;
  stats() { }
  stats(const TH1F& h) {
    mean = h.GetMean();
    var  = sq(h.GetStdDev());
    skew = h.GetSkewness();
    kurt = h.GetKurtosis();
    ent  = Entropy(h);
  }
};

struct vars {
  stats red, green, blue, lum;
} v;

// ---------
template<typename T>
struct numbered {
  int i; T x;
  bool operator <(const numbered<T>& other) const noexcept {
    return i>other.i; // smallest on top
  }
  numbered(int i, const T& x): i(i), x(x) { }
};

// Collectors
priority_queue<numbered<vars>> output_queue;
int last_written=-1;

// Global
deque<numbered<string>> img_files;

//---------------------------------------------------------
// Process images and fill histograms
//---------------------------------------------------------
void process_image() noexcept {

  size_t num_imgs_left; // number of images still in the deque
  for ( ; ; ) {

    // check if deque is empty
    deque_mutex.lock();
    num_imgs_left = img_files.size();
    if ( num_imgs_left == 0 ) {
      deque_mutex.unlock();
      break;
    }

    // pop next image of the deque
    const auto img_file = img_files.front();
    img_files.pop_front();
    deque_mutex.unlock();

    // Open image
    Magick::Image img(img_file.x);

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
      cout.flush();
    }
    info_mutex.unlock();

    // Get image dimensions
    Magick::Geometry geom = img.size();
    size_t width  = geom.width();
    size_t height = geom.height();

    // histograms
    TH1F   h_red(Form("h_red_%d",  img_file.i),"",100,0,1),
         h_green(Form("h_green_%d",img_file.i),"",100,0,1),
          h_blue(Form("h_blue_%d", img_file.i),"",100,0,1),
           h_lum(Form("h_lum_%d",  img_file.i),"",100,0,1);

    for (size_t h=0;h<height;++h) {
      for (size_t w=0;w<width;++w) {

        // Get pixel color
        const Magick::Color c(img.pixelColor(w,h));

        // Get pixel RGB info
        const Magick::ColorRGB rgb(c);

          h_red.Fill( rgb.red  () );
        h_green.Fill( rgb.green() );
         h_blue.Fill( rgb.blue () );
          h_lum.Fill( 0.27*rgb.red() + 0.67*rgb.green() + 0.06*rgb.blue() );
      }
    }

    // Fill maps --------------------------------
    result_mutex.lock();

    output_queue.emplace( img_file.i, vars{
      h_red, h_green, h_blue, h_lum
    } );

    while (output_queue.top().i-last_written==1) {
      v = output_queue.top().x;
      last_written = output_queue.top().i;
      output_queue.pop();
      tree->Fill();
      if (output_queue.empty()) break;
    }

    result_mutex.unlock();
  }
}

//---------------------------------------------------------
// MAIN
//---------------------------------------------------------
int main(int argc, char** argv)
{
  // parse arguments
  if (argc!=5) {
    cout << "Usage: " << argv[0]
         << " output.root list.txt dir ext" << endl;
    return 0;
  }

  // add image files names to deque
  ifstream img_list(argv[2]);
  string img_name;
  while ( getline(img_list,img_name) ) {
    if (!img_name.size()) continue;

    static int i = 0;
    string dir(argv[3]);
    if (dir.back()!='/') img_name.insert(0,"/");
    img_name.insert(0,dir);
    if (argv[4][0]!='.') img_name += '.';
    img_name += argv[4];
    img_files.emplace_back(i,img_name);
    ++i;

    //cout << i <<' '<< img_files.back().second << endl;
  }
  img_list.close();

  // validate root output file name
  {
    string root_file(argv[1]);
    if ( root_file.substr(root_file.find_last_of('.')).compare(".root") ) {
      cout << "Output file name \'" << root_file
           << "\' does not have \'.root\' extension" << endl;
      return 1;
    }
  }

  // open output root file
  TFile *f = new TFile(argv[1],"recreate");
  tree = new TTree("variables","");

  // Book histograms
  tree->Branch("mean_red",&v.red.mean,"mean_red/F");
  tree->Branch("mean_green",&v.green.mean,"mean_green/F");
  tree->Branch("mean_blue",&v.blue.mean,"mean_blue/F");
  tree->Branch("mean_lum",&v.lum.mean,"mean_lum/F");

  tree->Branch("var_red",&v.red.var,"var_red/F");
  tree->Branch("var_green",&v.green.var,"var_green/F");
  tree->Branch("var_blue",&v.blue.var,"var_blue/F");
  tree->Branch("var_lum",&v.lum.var,"var_lum/F");

  tree->Branch("skew_red",&v.red.skew,"skew_red/F");
  tree->Branch("skew_green",&v.green.skew,"skew_green/F");
  tree->Branch("skew_blue",&v.blue.skew,"skew_blue/F");
  tree->Branch("skew_lum",&v.lum.skew,"skew_lum/F");

  tree->Branch("kurt_red",&v.red.kurt,"kurt_red/F");
  tree->Branch("kurt_green",&v.green.kurt,"kurt_green/F");
  tree->Branch("kurt_blue",&v.blue.kurt,"kurt_blue/F");
  tree->Branch("kurt_lum",&v.lum.kurt,"kurt_lum/F");

  tree->Branch("ent_red",&v.red.ent,"ent_red/F");
  tree->Branch("ent_green",&v.green.ent,"ent_green/F");
  tree->Branch("ent_blue",&v.blue.ent,"ent_blue/F");
  tree->Branch("ent_lum",&v.lum.ent,"ent_lum/F");

  // process images in multiple threads
  const unsigned num_threads = max(thread::hardware_concurrency(),1u);
  cout << "Using " << num_threads << " threads" << endl;
  vector<thread> threads;
  threads.reserve(num_threads);
  for(unsigned i=0; i<num_threads; ++i) {
    threads.push_back( thread(process_image) );
  }

  cout << "About to join threads" << endl;

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
