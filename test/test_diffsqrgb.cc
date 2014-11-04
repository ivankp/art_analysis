#include <iostream>

#include <TFile.h>
#include <TH1.h>

#include "img_reader.h"

using namespace std;

int main(int argc, char** argv)
{
  if (argc<3) {
    cout << "Usage: " << argv[0]
         << " output.root input1 ..." << endl;
    return 0;
  }

  TFile *f = new TFile(argv[1],"recreate");

  for (int ai=2;ai<argc;++ai) {
    img_reader img(argv[ai]);

    TH1F *hist = new TH1F(argv[ai],argv[ai],200,0,1);

    cout << argv[ai] << endl;
    cout << "width: " << img.width()
         << " height: " << img.height() << endl;
/*
    // Resize picture (keeps aspect ratio)
    img.resize(100,100);
    cout << "Resize: width: " << img.width()
         << " height: " << img.height() << endl;
*/
/*
    // Calculate squares of differences of RGB vectors
    // for all pairs of vectors
    for (size_t c1=0;c1<img.width();++c1)
    for (size_t r1=0;r1<img.height();++r1)
    for (size_t c2=c1+1;c2<img.width();++c2)
    for (size_t r2=r1+1;r2<img.height();++r2)
      hist->Fill( img.pixel(c1,r1).diffsqrgb(img.pixel(c2,r2)) );
*/
    // Calculate squares of differences of RGB vectors
    // for adjecent pairs of vectors
    // Note: if statements prevent double counting
    // and out of bound indices
    for (size_t c1=1;c1<img.width();c1+=2)
    for (size_t r1=1;r1<img.height();r1+=2)
    for (short i=-1;i<=1;++i) {
      if (img.width()-c1==1 && i==1) continue;
      for (short j=-1;j<=1;++j) {
        if (i==0 && j==0) continue;
        if (img.height()-r1==1 && j==1) continue;
        hist->Fill( img.pixel(c1,r1).diffsqrgb(img.pixel(c1+i,r1+j)) );
      }
    }
  }

  f->Write();
  f->Close();
  delete f;

  return 0;
}
