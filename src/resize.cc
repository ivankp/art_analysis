#include <iostream>
#include <iomanip>
#include <deque>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cmath>
#include <ctime>

#include <dirent.h>

#include <ImageMagick/Magick++.h>

using namespace std;

#define test(var) \
  cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << endl;

// global variables
double area;
string idir, odir;
deque<string> img_files;

// mutexes
mutex info_mutex;
mutex img_files_mutex;

// timing
time_t last_time = time(0);
int num_sec = 0; // seconds elapsed

//---------------------------------------------------------
// Process images and fill histograms
//---------------------------------------------------------
void resize_image() noexcept {

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
    Magick::Image img(idir+'/'+img_file);

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
    double sqrtA  = sqrt(area);
    double sqrtR  = sqrt(double(geom.width())/double(geom.height()));

    // Resize image
    img.zoom( Magick::Geometry(sqrtA*sqrtR,sqrtA/sqrtR) );
    img.write(odir+'/'+img_file);
  }
}

//---------------------------------------------------------
// MAIN
//---------------------------------------------------------
int main(int argc, char** argv)
{
  // parse arguments
  if (argc!=4) {
    cout << "Usage: " << argv[0]
         << " area input_dir output_dir" << endl;
    return 0;
  }

  area = atof(argv[1]);
  idir = argv[2];
  odir = argv[3];

  if (area<2) {
    cerr << "Bad area argument: " << argv[1] << endl;
    exit(1);
  }

  // add image files names to deque
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir(idir.c_str())) != NULL) {
    // print all the files and directories within directory
    while ((ent = readdir(dir)) != NULL) {
      string name(ent->d_name);
      if (name.compare(".") && name.compare("..")) {
        img_files.push_back(name);
      }
    }
    closedir(dir);
  } else { // could not open directory
    cout << "Cannot open directory " << idir << endl;
    exit(1);
  }

  // process images in multiple threads
  const unsigned num_threads = max(thread::hardware_concurrency(),1u);
  cout << "Using " << num_threads << " threads" << endl;
  vector<thread> threads;
  threads.reserve(num_threads);
  for(unsigned i=0; i<num_threads; ++i){
    threads.push_back(
      thread( resize_image )
    );
  }

  // join threads so that program doesn't quit before they are done
  for(auto& thread : threads) thread.join();

  cout << "Done!" << endl;

  return 0;
}
