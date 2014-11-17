#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>

#include <TFile.h>
#include <TKey.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TH1.h>

using namespace std;

#define test(var) \
  cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << endl;

string base_name(string name) {
  string base = name;
  const size_t slash = name.find_last_of('/');
  if (slash!=string::npos) name = name.substr(slash);
  const size_t ext   = name.rfind(".root");
  if (ext!=string::npos) return name.substr(0,ext);
  else return string();
}

int main(int argc, char *argv[])
{
  if ( argc!=4 ) {
    cout << "Usage: " << argv[0]
         << " out.pdf in1.root in2.root ..." << endl;
    return 0;
  }

  vector< unique_ptr<TFile> > f;
  unordered_map< string, vector<TH1*> > h_;

  for (int i=2; i<argc; ++i) {
    string name = base_name(argv[i]);
    if (name.size()==0) {
      cout << "Bad root file name: " << argv[i] << endl;
      exit(1);
    }

    f.push_back( unique_ptr<TFile>(new TFile(argv[i],"read")) );
    if ( f.back()->IsZombie() ) exit(1);

    static TKey *key;
    TIter nextkey(f.back()->GetListOfKeys());
    while (( key = (TKey*)nextkey() )) {
      TH1 *hist = dynamic_cast<TH1*>( key->ReadObj() );
      hist->SetLineColor(i);
      hist->SetMarkerColor(i);
      hist->SetLineWidth(2);
      h_[hist->GetName()].push_back(hist);
      hist->SetTitle(name.c_str());
    }
  }

  for (auto it=h_.begin(), end=h_.end(); it!=end; ++it) {
    const size_t nh = it->second.size();
    TH1* h = it->second[0];
    h->Scale(1./h->Integral());
    double ymax = h->GetMaximum();

    for (size_t i=1;i<nh;++i) {
      h = it->second[i];
      h->Scale(1./h->Integral());
      double max = h->GetMaximum();
      if (max>ymax) ymax = max;
    }

    it->second[0]->SetAxisRange(0.,ymax*1.05,"Y");
  }

  TCanvas canv;
  gStyle->SetOptStat(0);

  string out(argv[1]);
  canv.SaveAs((out+'[').c_str());

  for (auto it=h_.begin(), end=h_.end(); it!=end; ++it) {
    auto& hv = it->second;
    const size_t nh = hv.size();

    TLegend leg(0.75,0.92-0.06*nh,0.95,0.92);
    leg.SetFillColor(0);

    for (size_t i=0;i<nh;++i) {
      TH1* h = hv[i];
      leg.AddEntry(h,Form("%s N=%.0f",h->GetTitle(),h->GetEntries()));
      if (i==0) {
        h->SetTitle(Form("%s;%s;",h->GetName(),h->GetName()));
        h->Draw();
      } else h->Draw("same");
    }

    leg.Draw();
    canv.SaveAs(out.c_str());
  }

  canv.SaveAs((out+']').c_str());

  return 0;
}
