#include "hist_wrap.h"

#include <fstream>
#include <vector>

#include <boost/regex.hpp>

#include <TH1.h>

using namespace std;

#define test(var) \
  cout <<"\033[36m"<< #var <<"\033[0m"<< " = " << var << endl;

// binning ************************************************

struct binning {
  int nbins; double min, max;
  binning(): nbins(100), min(0.), max(100.) { }
  ~binning() { }
};

// acc_impl ***********************************************

struct hist::acc_impl {
  vector<const hist*> all;

  binning default_binning;
  vector<pair<boost::regex*,binning>> binnings;
  vector<pair<string,int>> btest;

  const binning& get_binning(const string& hist_name) {
    boost::smatch result;
    for (auto& x : binnings) {
      if (boost::regex_match(hist_name, result, *x.first,
                             boost::match_default | boost::match_partial))
        if(result[0].matched) return x.second;
    }
    return default_binning;
  }

  ~acc_impl() {
    for (auto& x : binnings) delete x.first;
    for (auto& h : all) delete h;
  }
};

// hist ***************************************************

void hist::read_binnings(const string& filename) {
  ifstream binsfile(filename);

  string hname;
  binning b;
  while ( binsfile >> hname ) {
    if (hname[0]=='#') {
      binsfile.ignore(numeric_limits<streamsize>::max(), '\n');
      continue;
    }

    binsfile >> b.nbins;
    binsfile >> b.min;
    binsfile >> b.max;

    pimpl->binnings.emplace_back( new boost::regex(hname), b );
  }
}

hist::hist(): h(NULL), underflow(0,0.), overflow(0,0.) { }
hist::~hist() { } // doesn't delete TH1F* because TFile will do that

TH1F& hist::operator*()  const { return *h; }
TH1F* hist::operator->() const { return  h; }
TH1F* hist::get()        const { return  h; }

hist::hist(const string& name, const string& title) {
  const binning& b = pimpl->get_binning(name);
  h = new TH1F(name.c_str(),title.c_str(),b.nbins,b.min,b.max);

  underflow = make_pair(0,b.min);
  overflow  = make_pair(0,b.max);

  pimpl->all.push_back(this);
}
void hist::Fill(double x, double w) {
  h->Fill(x,w);
  if (x<h->GetBinLowEdge(1)) {
    ++underflow.first;
    if (x<underflow.second) underflow.second = x;
  } else
  if (x>=h->GetBinLowEdge(h->GetNbinsX()+1)) {
    ++overflow.first;
    if (x>overflow.second) overflow.second = x;
  }
}

void hist::FillUnderflow(double weight) {
  Fill(0,weight);
}
void hist::FillOverflow(double weight) {
  Fill(h->GetNbinsX()+1,weight);
}

ostream& hist::print_overflow(ostream& out) {
  for (auto& h : pimpl->all) {
    if (h->underflow.first) {
      out << "Underflow in " << h->h->GetName()
          << ": N="<<h->underflow.first << " min="<<h->underflow.second << endl;
    }
    if (h->overflow.first) {
      out << "Overflow in " << h->h->GetName()
          << ": N="<<h->overflow.first << " max="<<h->overflow.second << endl;
    }
  }
  return out;
}

void hist::delete_all() {
  delete pimpl;
}

hist::acc_impl *hist::pimpl = new hist::acc_impl;
