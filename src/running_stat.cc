// taken from http://www.johndcook.com/blog/standard_deviation/

#include "running_stat.h"

#include <cmath>

running_stat::running_stat(): m_n(0) {}

void running_stat::clear() {
  m_n = 0;
}

void running_stat::push(double x) {
  m_n++;

  // See Knuth TAOCP vol 2, 3rd edition, page 232
  if (m_n == 1) {
    m_oldM = m_newM = x;
    m_oldS = 0.0;
  } else {
    m_newM = m_oldM + (x - m_oldM)/m_n;
    m_newS = m_oldS + (x - m_oldM)*(x - m_newM);

    // set up for next iteration
    m_oldM = m_newM; 
    m_oldS = m_newS;
  }
}

int running_stat::num() const { return m_n; }

double running_stat::mean() const {
  return (m_n > 0) ? m_newM : 0.0;
}

double running_stat::var() const {
  return ( (m_n > 1) ? m_newS/(m_n - 1) : 0.0 );
}

double running_stat::stdev() const {
  return std::sqrt( var() );
}
