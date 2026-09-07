#include "clas12dihadrons/fit/BeamSpinFit.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace clas12::dihadron::fit {
namespace {
using Matrix=std::array<std::array<double,7>,7>;
using Vector=std::array<double,7>;
Matrix inverse(Matrix a){
 Matrix inv{}; for(size_t i=0;i<7;++i) inv[i][i]=1;
 for(size_t c=0;c<7;++c){
  size_t p=c; for(size_t r=c+1;r<7;++r) if(std::abs(a[r][c])>std::abs(a[p][c]))p=r;
  if(std::abs(a[p][c])<1e-14) throw std::runtime_error("singular BSA Hessian");
  std::swap(a[p],a[c]); std::swap(inv[p],inv[c]); const double d=a[c][c];
  for(size_t j=0;j<7;++j){a[c][j]/=d;inv[c][j]/=d;}
  for(size_t r=0;r<7;++r)if(r!=c){const double f=a[r][c];for(size_t j=0;j<7;++j){a[r][j]-=f*a[c][j];inv[r][j]-=f*inv[c][j];}}
 } return inv;
}
double nll(const std::vector<FitEvent>& e,const Vector&a,double wp,double wm){
 double out=0; for(const auto&v:e){auto f=evaluate_modulations(v.phi_h,v.phi_R);double s=0;for(size_t i=0;i<7;++i)s+=a[i]*f[i];
  const double d=1+v.helicity*v.polarization*s;if(d<=0)return std::numeric_limits<double>::infinity();
  out-=(v.weight*(v.helicity>0?wp:wm))*std::log(d);
 } return out;
}
}
std::array<double,7> evaluate_modulations(double h,double r){return {
 std::sin(2*r-h),std::sin(r),std::sin(h),std::sin(h-r),std::sin(2*h-r),std::sin(2*h-2*r),std::sin(3*h-2*r)};}

FitResult fit_beam_spin(const std::vector<FitEvent>& events,const FitOptions&o){
 if(events.empty())throw std::invalid_argument("BSA fit needs events"); double plus=0,minus=0;
 for(const auto&e:events){if((e.helicity!=1&&e.helicity!=-1)||!(e.polarization>0&&e.polarization<=1)||e.weight<=0)throw std::invalid_argument("invalid BSA event");(e.helicity>0?plus:minus)+=e.weight;}
 if(plus==0||minus==0)throw std::invalid_argument("BSA fit needs both helicities");
 const double total=plus+minus,wp=o.balance_helicity?total/(2*plus):1,wm=o.balance_helicity?total/(2*minus):1;
 FitResult result; result.event_count=events.size(); Vector a{}; Matrix hessian{};
 for(size_t iter=0;iter<o.max_iterations;++iter){Vector gradient{};hessian={};
  for(const auto&e:events){const auto f=evaluate_modulations(e.phi_h,e.phi_R);double s=0;for(size_t i=0;i<7;++i)s+=a[i]*f[i];const double d=1+e.helicity*e.polarization*s;if(d<=0)throw std::runtime_error("BSA likelihood left physical domain");const double w=e.weight*(e.helicity>0?wp:wm),q=e.helicity*e.polarization;
   for(size_t i=0;i<7;++i){gradient[i]-=w*q*f[i]/d;for(size_t j=0;j<7;++j)hessian[i][j]+=w*q*q*f[i]*f[j]/(d*d);}}
  const auto cov=inverse(hessian);Vector step{};double maxstep=0;for(size_t i=0;i<7;++i){for(size_t j=0;j<7;++j)step[i]+=cov[i][j]*gradient[j];maxstep=std::max(maxstep,std::abs(step[i]));}
  const double old=nll(events,a,wp,wm);double scale=1;Vector trial{};while(scale>1e-8){for(size_t i=0;i<7;++i)trial[i]=a[i]-scale*step[i];if(nll(events,trial,wp,wm)<old)break;scale*=0.5;}a=trial;result.iterations=iter+1;
  if(maxstep*scale<o.tolerance){result.converged=true;break;}
 }
 // Re-evaluate observed Hessian at the solution.
 hessian={};for(const auto&e:events){const auto f=evaluate_modulations(e.phi_h,e.phi_R);double s=0;for(size_t i=0;i<7;++i)s+=a[i]*f[i];const double d=1+e.helicity*e.polarization*s,w=e.weight*(e.helicity>0?wp:wm),q=e.helicity*e.polarization;for(size_t i=0;i<7;++i)for(size_t j=0;j<7;++j)hessian[i][j]+=w*q*q*f[i]*f[j]/(d*d);}
 result.amplitudes=a;result.covariance=inverse(hessian);for(size_t i=0;i<7;++i)result.errors[i]=std::sqrt(std::max(0.0,result.covariance[i][i]));result.negative_log_likelihood=nll(events,a,wp,wm);return result;
}
}

