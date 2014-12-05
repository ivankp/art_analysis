#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <set>
#include <vector>
#include <algorithm>
#include <functional>

#include <TFile.h>
#include <TH2.h>
#include <TAxis.h>
#include <TCanvas.h>

using namespace std;

void SetHOpt(TH1* h) {
  h->SetLabelSize(0.025,"XY");
  h->SetLabelSize(0.025,"Z");
  h->SetLabelOffset(0.005,"X");
  h->GetXaxis()->LabelsOption("v");
}

int main(int argc, char ** argv)
{
  if (argc!=4 && argc!=5) {
    cout << "Usage: " << argv[0] << " tmva.root priority.txt cut [draw]" << endl;
    return 0;
  }
  const bool draw = (argc==5 ? !strcmp(argv[4],"draw") : false);

  TCanvas canv("canv","",650,650);
  canv.SetLeftMargin(0.14);
  canv.SetBottomMargin(0.14);
  canv.SetTopMargin(0.09);
  canv.SetRightMargin(0.1);

  TFile *f = new TFile(argv[1],"read");
  TH2 *cms = (TH2*)f->Get("CorrelationMatrixS");
  TH2 *cmb = (TH2*)f->Get("CorrelationMatrixB");

  if (draw) {
    SetHOpt(cms);
    SetHOpt(cmb);

    cms->Draw("colz");
    canv.SaveAs("CorrelationMatrixS.pdf");
    canv.Clear();

    cmb->Draw("colz");
    canv.SaveAs("CorrelationMatrixB.pdf");
    canv.Clear();
  }

  const Int_t num_vars = cms->GetNbinsX()+1;
  cout << "Num vars: " << num_vars << endl;

  ifstream pf(argv[2]);
  vector<Int_t> p;
  p.reserve(num_vars+1);
  p.push_back(0);
  while (pf >> p.back()) p.push_back(0);

  auto pri_hist = [&p](Int_t i) {
    auto _p = find(p.begin(),p.end(),i);
    return make_pair( int(p.end()-_p), i );
  };

  TH2 *h = cms;
  const double cut = atof(argv[3]);
  set<Int_t> excluded;
  set<pair<int,Int_t>> current;
  for (Int_t i=1;i<num_vars;++i) {
    if (excluded.count(i)) continue;
    current.clear();
    current.insert(pri_hist(i));
    for (Int_t j=1;j<num_vars;++j) {
      if (j==i) continue;
      if (excluded.count(j)) continue;
      double x = abs( h->GetBinContent(i,j) );
      if (x>cut) current.insert(pri_hist(j));
    }
    pair<int,Int_t> best {0,0};
    for (auto k : current) {
      if (best.first<k.first) {
        if (best.second>0) excluded.insert(best.second);
        best = k;
      }
    }
  }

  set<Int_t> kept;
  for (Int_t i=1;i<num_vars;++i)
    if (!excluded.count(i)) kept.insert(i);

  cout << "\nKept variables" << endl;
  for (auto i : kept)
    cout << "var " << setw(2) << i << ": " << h->GetXaxis()->GetBinLabel(i) << endl;

  return 0;
}
