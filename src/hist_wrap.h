#ifndef hist_wrap_h
#define hist_wrap_h

#include <iostream>
#include <string>

class TH1F;

class hist {
  class acc_impl; // implementation of accumulator
  static acc_impl *pimpl;

  TH1F * h;
  std::pair<int,double> underflow, overflow;

public:
  hist();
  hist(const std::string& name,const std::string& title="");
  ~hist();

  void Fill(double x, double w=1.);
  void FillOverflow(double weight);
  void FillUnderflow(double weight);

  static void read_binnings(const char* filename);
  static std::ostream& print_overflow(std::ostream& out=std::cout);
  static void delete_all();

  TH1F& operator*() const;
  TH1F* operator->() const;
  TH1F* get() const;
};

#endif
