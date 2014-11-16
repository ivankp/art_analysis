// taken from http://www.johndcook.com/blog/standard_deviation/

#ifndef running_stat_h
#define running_stat_h

class running_stat {
  int m_n;
  double m_oldM, m_newM, m_oldS, m_newS;

public:
  running_stat();

  void clear();
  void push(double x);
  int  num() const;

  double mean() const;
  double var() const;
  double stdev() const;
};

#endif
