#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <queue>
#include <deque>
#include <vector>
#include <array>
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
  struct {
    stats red, green, blue, lum;
  } mag, ang;
  vars() { }
  vars(const array<vector<TH1F>,2>& h)
  : mag { h[0][0], h[0][1], h[0][2], h[0][3] },
    ang { h[1][0], h[1][1], h[1][2], h[1][3] }
  { }
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

const array<array<double,3>,3> Gx {{ {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} }};
const array<array<double,3>,3> Gy {{ { 1, 2, 1}, { 0, 0, 0}, {-1,-2,-1} }};

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
    array<vector<TH1F>,2> hist;
    for (short k=0;k<2;++k) {
      hist[k].reserve(4);
      for (short i=0;i<4;++i) {
        hist[k].emplace_back(Form("h_%d_%d_%d",k,i,img_file.i),"",100,0,1);
      }
    }

    deque<deque<array<double,4>>> pixdeq;

    for (size_t h=0;h<height;++h) {

      pixdeq.emplace_back();

      for (size_t w=0;w<width;++w) {

        // Get pixel color
        const Magick::Color c(img.pixelColor(w,h));

        // Get pixel RGB info
        const Magick::ColorRGB rgb(c);

        if (width-w>3) pixdeq.back().push_back( array<double,4>{{
          rgb.red(), rgb.green(), rgb.blue(),
          0.27*rgb.red() + 0.67*rgb.green() + 0.06*rgb.blue() // luminance
        }} );

        // Convolution
        if (pixdeq.size()==3 && pixdeq.back().size()==3) {

          for (size_t k=0;k<4;++k) {
            double _Gx=0., _Gy=0.;
            for (int i=0;i<3;++i) {
              for (int j=0;j<3;++j) {
                _Gx += Gx[i][j]*pixdeq[i][j][k];
                _Gy += Gy[i][j]*pixdeq[i][j][k];
              }
            }

            hist[0][k].Fill( sumsq(_Gx,_Gy) ); // mag
            hist[1][k].Fill( sumsq(_Gx,_Gy) ); // ang
          }

          pixdeq.back().pop_front();
        }
      }

      if (pixdeq.size()==3) pixdeq.pop_front();

    }

    // Fill maps --------------------------------
    result_mutex.lock();

    output_queue.emplace( img_file.i, vars(hist) );

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
    //if (argv[4][0]!='.') img_name += '.';
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
  tree->Branch("mag_mean_red",&v.mag.red.mean,"mag_mean_red/F");
  tree->Branch("mag_mean_green",&v.mag.green.mean,"mag_mean_green/F");
  tree->Branch("mag_mean_blue",&v.mag.blue.mean,"mag_mean_blue/F");
  tree->Branch("mag_mean_lum",&v.mag.lum.mean,"mag_mean_lum/F");

  tree->Branch("mag_var_red",&v.mag.red.var,"mag_var_red/F");
  tree->Branch("mag_var_green",&v.mag.green.var,"mag_var_green/F");
  tree->Branch("mag_var_blue",&v.mag.blue.var,"mag_var_blue/F");
  tree->Branch("mag_var_lum",&v.mag.lum.var,"mag_var_lum/F");

  tree->Branch("mag_skew_red",&v.mag.red.skew,"mag_skew_red/F");
  tree->Branch("mag_skew_green",&v.mag.green.skew,"mag_skew_green/F");
  tree->Branch("mag_skew_blue",&v.mag.blue.skew,"mag_skew_blue/F");
  tree->Branch("mag_skew_lum",&v.mag.lum.skew,"mag_skew_lum/F");

  tree->Branch("mag_kurt_red",&v.mag.red.kurt,"mag_kurt_red/F");
  tree->Branch("mag_kurt_green",&v.mag.green.kurt,"mag_kurt_green/F");
  tree->Branch("mag_kurt_blue",&v.mag.blue.kurt,"mag_kurt_blue/F");
  tree->Branch("mag_kurt_lum",&v.mag.lum.kurt,"mag_kurt_lum/F");

  tree->Branch("mag_ent_red",&v.mag.red.ent,"mag_ent_red/F");
  tree->Branch("mag_ent_green",&v.mag.green.ent,"mag_ent_green/F");
  tree->Branch("mag_ent_blue",&v.mag.blue.ent,"mag_ent_blue/F");
  tree->Branch("mag_ent_lum",&v.mag.lum.ent,"mag_ent_lum/F");


  tree->Branch("ang_mean_red",&v.ang.red.mean,"ang_mean_red/F");
  tree->Branch("ang_mean_green",&v.ang.green.mean,"ang_mean_green/F");
  tree->Branch("ang_mean_blue",&v.ang.blue.mean,"ang_mean_blue/F");
  tree->Branch("ang_mean_lum",&v.ang.lum.mean,"ang_mean_lum/F");

  tree->Branch("ang_var_red",&v.ang.red.var,"ang_var_red/F");
  tree->Branch("ang_var_green",&v.ang.green.var,"ang_var_green/F");
  tree->Branch("ang_var_blue",&v.ang.blue.var,"ang_var_blue/F");
  tree->Branch("ang_var_lum",&v.ang.lum.var,"ang_var_lum/F");

  tree->Branch("ang_skew_red",&v.ang.red.skew,"ang_skew_red/F");
  tree->Branch("ang_skew_green",&v.ang.green.skew,"ang_skew_green/F");
  tree->Branch("ang_skew_blue",&v.ang.blue.skew,"ang_skew_blue/F");
  tree->Branch("ang_skew_lum",&v.ang.lum.skew,"ang_skew_lum/F");

  tree->Branch("ang_kurt_red",&v.ang.red.kurt,"ang_kurt_red/F");
  tree->Branch("ang_kurt_green",&v.ang.green.kurt,"ang_kurt_green/F");
  tree->Branch("ang_kurt_blue",&v.ang.blue.kurt,"ang_kurt_blue/F");
  tree->Branch("ang_kurt_lum",&v.ang.lum.kurt,"ang_kurt_lum/F");

  tree->Branch("ang_ent_red",&v.ang.red.ent,"ang_ent_red/F");
  tree->Branch("ang_ent_green",&v.ang.green.ent,"ang_ent_green/F");
  tree->Branch("ang_ent_blue",&v.ang.blue.ent,"ang_ent_blue/F");
  tree->Branch("ang_ent_lum",&v.ang.lum.ent,"ang_ent_lum/F");

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
