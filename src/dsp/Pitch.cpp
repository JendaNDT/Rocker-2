#include "Pitch.h"
#include <algorithm>
#include <cmath>
namespace rocker {
double Pitch::frequency(int midi) noexcept { return 440.*std::exp2((midi-69)/12.); }
double Pitch::cents(double hz,int note) noexcept { return 1200.*std::log2(hz/frequency(note)); }
double Pitch::detect(std::span<const float> x,double rate) const noexcept {
    // YIN cumulative mean normalised difference. Refine the period using the
    // raw difference to avoid the frequency bias of the normalised parabola.
    std::array<double,513> diff{},normal{};
    const int limit=std::min(510,static_cast<int>(std::ceil(rate/40.)));
    const int count=static_cast<int>(x.size())-limit-1;
    if (count<limit || rate<=0) return 0;
    double energy=0;
    for (auto sample:x) energy+=sample*sample;
    if (energy/x.size()<1.e-7) return 0;
    double running=0;
    for (int lag=1;lag<=limit+1;++lag) {
        double distance=0;
        for (int j=0;j<count;++j) {
            const double delta=x[j]-x[j+lag];
            distance+=delta*delta;
        }
        diff[lag]=distance; running+=distance;
        normal[lag]=running>0 ? distance*lag/running : 1;
    }
    for (int lag=std::max(2,static_cast<int>(rate/1250.));lag<=limit;++lag) {
        if (normal[lag]>0.13 || normal[lag]>normal[lag-1] || normal[lag]>normal[lag+1]) continue;
        const double curvature=diff[lag-1]-2*diff[lag]+diff[lag+1];
        const double offset=curvature>0 ? 0.5*(diff[lag-1]-diff[lag+1])/curvature : 0;
        return rate/(lag+std::clamp(offset,-0.5,0.5));
    }
    return 0;
}
}
