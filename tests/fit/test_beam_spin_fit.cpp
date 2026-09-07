#include "clas12dihadrons/fit/BeamSpinFit.hpp"
#include <cmath>
#include <iostream>
#include <vector>
int main(){using namespace clas12::dihadron::fit;std::vector<FitEvent> e;constexpr double P=.86,A=.08,pi=3.141592653589793;
 for(int i=0;i<24;++i)for(int j=0;j<24;++j){double h=-pi+(i+.5)*2*pi/24,r=-pi+(j+.5)*2*pi/24;double m=evaluate_modulations(h,r)[0];e.push_back({1,P,h,r,(1+P*A*m)/2});e.push_back({-1,P,h,r,(1-P*A*m)/2});}
 auto f=fit_beam_spin(e);if(!f.converged||std::abs(f.amplitudes[0]-A)>1e-5)return 1;for(int i=1;i<7;++i)if(std::abs(f.amplitudes[i])>1e-5)return 2;std::cout<<"beam-spin-fit: PASS\n";}

