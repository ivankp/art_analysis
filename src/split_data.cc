#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>

#include <TRandom3.h>

using namespace std;

int main (int argc, char **argv)
{
  if (argc<4) {
    cout << "Usage: " << argv[0] << " frac str1 str2 ..." << endl;
    return 0;
  }

  const float frac = atof(argv[1]);
  if (frac<=0. || frac>1.) {
    cout << "Fraction " << argv[1] << " is not in (0,1)" << endl;
    return 1;
  }

  TRandom3 rnd(time(0));

  const size_t n = (argc-2)*frac;

  vector<string> x;
  for (int i=2;i<argc;++i) x.push_back(argv[i]);

  size_t size;
  while ((size=x.size())>n)
    x.erase(x.begin()+rnd.Integer(size));

  for (auto& s : x) cout << s << endl;

  return 0;
}
