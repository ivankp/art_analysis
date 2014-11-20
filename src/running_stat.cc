// taken from http://www.johndcook.com/blog/standard_deviation/

#include "running_stat.h"

#include <cmath>

running_stat::running_stat(): n(0) {}

void running_stat::clear() {
  n = 0;
}

void running_stat::push(double x) {
  n++;

  // See Knuth TAOCP vol 2, 3rd edition, page 232
  if (n == 1) {
    oldM = newM = x;
    oldS = 0.;
  } else {
    newM = oldM + (x - oldM)/n;
    newS = oldS + (x - oldM)*(x - newM);

    // set up for next iteration
    oldM = newM; 
    oldS = newS;
  }
}

int running_stat::num() const { return n; }

double running_stat::mean() const {
  return (n > 0) ? newM : 0.;
}

double running_stat::var() const {
  return ( (n > 1) ? newS/(n - 1) : 0. );
}

double running_stat::stdev() const {
  return std::sqrt( var() );
}
