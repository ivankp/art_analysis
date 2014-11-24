#include <iostream>
#include <vector>
//#include <unordered_map>
#include <memory>

#include <boost/program_options.hpp>

#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TH1.h>

#include "hist_wrap.h"

using namespace std;
namespace po = boost::program_options;

#define test(var) \
  cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << endl;

struct var_file {
  string name;
  shared_ptr<TFile> file;
  TTree *tree;
  //var_file(const var_file& other)
  //: name(other.name), file(other.file), 
};

istream& operator>>(istream& in, var_file& f) {
  string token;
  in >> token;

  size_t sep = token.find(':');
  if (sep==string::npos) sep = 0;

  f.name = token.substr(0,sep);

  f.file = shared_ptr<TFile>( new TFile(token.substr(sep+1).c_str(),"read") );
  if (f.file->IsZombie()) exit(1);

  f.tree = dynamic_cast<TTree*>( f.file->Get("variables") );
  if (!f.tree) {
    cerr << "No tree \"variables\" in file " << f.file->GetName() << endl;
    f.file->ls();
    exit(1);
  }

  return in;
}

int main(int argc, char *argv[])
{
  // START OPTIONS **************************************************
  vector<var_file> fin;
  string fout;
  bool norm, logx, logy;

  try {
    // General Options ------------------------------------
    po::options_description all_opt("Options");
    all_opt.add_options()
      ("help,h", "produce help message")
      ("input,i", po::value< vector<var_file> >(&fin),
       "label:file, input root files with histograms")
      ("output,o", po::value<string>(&fout),
       "output pdf plots file")
      ("norm,n", po::bool_switch(&norm),
       "normalize histograms to unity")
      ("logx", po::bool_switch(&logx), "")
      ("logy", po::bool_switch(&logy), "")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, all_opt), vm);
    po::notify(vm);

    // Options Properties ---------------------------------
    if (argc == 1 || vm.count("help")) {
      cout << all_opt << endl;
      return 0;
    }

    // Necessary options ----------------------------------
    vector<string> rec_opt;
    rec_opt.push_back("input");
    rec_opt.push_back("output");

    for (size_t i=0, size=rec_opt.size(); i<size; ++i) {
      if (!vm.count(rec_opt[i]))
      { cerr << "Missing command --" << rec_opt[i] << endl; return 1; }
    }
  }
  catch(exception& e) {
    cerr << "\033[31mError: " <<  e.what() <<"\033[0m"<< endl;
    return 1;
  }
  // END OPTIONS ****************************************************

  hist::read_binnings("config/hists.bins");

  vector< pair< string, vector<hist*> > > h_;
  vector<pair<Float_t,hist*>> x;

  Color_t color[] = {602,46,3,2,94};

  for (auto& f : fin) { // open root files
    static int c = 0;

    f.file->cd();
    cout << "File: " << f.name << " : "
         << f.file->GetName() << endl;
    const TObjArray *brarr = f.tree->GetListOfBranches();
    const size_t numbr = brarr->GetEntries();
    if (x.size()<numbr) x.resize(numbr);

    for (size_t i=0;i<numbr;++i) {
      TBranch *br = dynamic_cast<TBranch*>(brarr->At(i));
      string name = br->GetName();
      cout << "Branch: " << name << endl;
      br->SetAddress(&x[i].first);
      x[i].second = new hist(name,f.name);

      bool found = false;
      for (auto& p : h_) {
        if (!p.first.compare(name)) {
          found = true;
          p.second.push_back( x[i].second );
          break;
        }
      }
      if (!found) {
        h_.push_back( make_pair(name, vector<hist*>()) );
        h_.back().second.push_back( x[i].second );
      }

      TH1* h = x[i].second->get();
      h->SetLineColor(color[c]);
      h->SetMarkerColor(color[c]);
      h->SetLineWidth(2);
    }

    for (Long64_t ent=0,n=f.tree->GetEntries();ent<n;++ent) {
      f.tree->GetEntry(ent);
      for (size_t i=0;i<numbr;++i)
        x[i].second->Fill(x[i].first);
    }

    ++c;
    cout << endl;
  }

  for (auto& _h : h_) {
    auto& hv = _h.second;
    const size_t nh = hv.size();
    TH1* h = hv.front()->get();
    if (norm) h->Scale(1./h->Integral());
    double ymax = h->GetMaximum();

    for (size_t i=1;i<nh;++i) {
      h = hv[i]->get();
      if (norm) h->Scale(1./h->Integral());
      double max = h->GetMaximum();
      if (max>ymax) ymax = max;
    }

    hv.front()->get()->SetAxisRange(0.,ymax*1.05,"Y");
  }

  TCanvas canv;
  gStyle->SetOptStat(0);
  if (logx) canv.SetLogx();
  if (logy) canv.SetLogy();

  canv.SaveAs((fout+'[').c_str());

  for (auto& _h : h_) {
    auto& hv = _h.second;
    const size_t nh = hv.size();

    TLegend leg(0.75,0.92-0.06*nh,0.95,0.92);
    leg.SetFillColor(0);

    for (size_t i=0;i<nh;++i) {
      TH1 *h = hv[i]->get();
      leg.AddEntry(h,Form("%s N=%.0f",
        h->GetTitle(),
        h->GetEntries()
      ) );
      h->SetTitle(h->GetName());
      if (logy) h->SetMinimum(1e-3);
      if (i==0) h->Draw();
      else h->Draw("same");
    }

    leg.Draw();
    canv.SaveAs(fout.c_str());
  }

  canv.SaveAs((fout+']').c_str());

  hist::print_overflow();

  return 0;
}
