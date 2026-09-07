#include "clas12dihadrons/analysis/Kinematics.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace clas12::dihadron::analysis {
namespace {
constexpr double kProtonMass = 0.9382720813;
constexpr double kElectronMass = 0.00051099895;
struct V3 { double x{}, y{}, z{}; };
V3 v3(const FourVector& p) { return {p.px,p.py,p.pz}; }
V3 add(V3 a,V3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
V3 sub(V3 a,V3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
V3 mul(V3 a,double s){return {a.x*s,a.y*s,a.z*s};}
double dot(V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
V3 cross(V3 a,V3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
double mag(V3 a){return std::sqrt(dot(a,a));}
FourVector sub4(const FourVector&a,const FourVector&b){return {a.px-b.px,a.py-b.py,a.pz-b.pz,a.energy-b.energy};}
FourVector scale4(const FourVector&a,double s){return {a.px*s,a.py*s,a.pz*s,a.energy*s};}
double minkowski(const FourVector&a,const FourVector&b){return a.energy*b.energy-dot(v3(a),v3(b));}
double momentum(const FourVector&p){return mag(v3(p));}

FourVector boost(const FourVector& p, V3 beta) {
  const double b2=dot(beta,beta);
  if (b2==0) return p;
  if (b2>=1) throw std::domain_error("unphysical Lorentz boost");
  const double gamma=1/std::sqrt(1-b2), bp=dot(beta,v3(p));
  const double factor=(gamma-1)*bp/b2+gamma*p.energy;
  const auto spatial=add(v3(p),mul(beta,factor));
  return {spatial.x,spatial.y,spatial.z,gamma*(p.energy+bp)};
}

double transverse(const FourVector& q,const FourVector& p,const FourVector& target){
  const auto total=q+target;
  const auto beta=mul(v3(total),-1.0/total.energy);
  const auto qb=boost(q,beta), pb=boost(p,beta);
  const auto qhat=mul(v3(qb),1.0/mag(v3(qb)));
  const auto perp=sub(v3(pb),mul(qhat,dot(v3(pb),qhat)));
  return mag(perp);
}

double xf(const FourVector&q,const FourVector&p,const FourVector&target,double W){
  const auto total=q+target;
  const auto beta=mul(v3(total),-1.0/total.energy);
  const auto qb=boost(q,beta), pb=boost(p,beta);
  return 2*dot(v3(qb),v3(pb))/(mag(v3(qb))*W);
}

double signed_plane_angle(V3 q,V3 beam,V3 value){
  const auto nl=cross(q,beam), nv=cross(q,value);
  const double ml=mag(nl), mv=mag(nv);
  if (ml<1e-12||mv<1e-12) throw std::domain_error("undefined azimuthal angle");
  const double cosine=std::clamp(dot(nl,nv)/(ml*mv),-1.0,1.0);
  const double orient=dot(nl,value);
  return std::copysign(std::acos(cosine),orient);
}
}

DihadronKinematics compute_dihadron_kinematics(
    double beam_energy_gev,const FourVector& electron,
    const FourVector& h1,const FourVector& h2){
  if(beam_energy_gev<=0) throw std::invalid_argument("beam energy must be positive");
  const FourVector beam{0,0,std::sqrt(beam_energy_gev*beam_energy_gev-kElectronMass*kElectronMass),beam_energy_gev};
  const FourVector target{0,0,0,kProtonMass};
  const auto q=sub4(beam,electron), pair=h1+h2;
  DihadronKinematics o;
  o.Q2=-q.mass_squared();
  const double nu=q.energy;
  o.y=nu/beam_energy_gev;
  o.x=o.Q2/(2*minkowski(target,q));
  o.W=std::sqrt(std::max(0.0,(q+target).mass_squared()));
  o.Mh=pair.mass();
  o.z1=minkowski(target,h1)/minkowski(target,q);
  o.z2=minkowski(target,h2)/minkowski(target,q);
  o.z=o.z1+o.z2;
  o.xF1=xf(q,h1,target,o.W); o.xF2=xf(q,h2,target,o.W); o.xF=xf(q,pair,target,o.W);
  o.pT1=transverse(q,h1,target); o.pT2=transverse(q,h2,target); o.pT=transverse(q,pair,target);
  o.phi_h=signed_plane_angle(v3(q),v3(beam),v3(pair));
  const auto R=mul(sub(v3(h1),v3(h2)),0.5);
  const auto qv=v3(q);
  const auto Rperp=sub(R,mul(qv,dot(qv,R)/dot(qv,qv)));
  o.phi_R=signed_plane_angle(qv,v3(beam),Rperp);
  o.missing_mass=sub4(sub4(beam+target,electron),pair).mass();
  o.P1=momentum(h1); o.P2=momentum(h2);
  return o;
}

bool passes_pippim_analysis_cuts(const DihadronKinematics&o){
  return std::isfinite(o.phi_h)&&std::isfinite(o.phi_R)&&o.Q2>=1.0&&o.y<=0.8&&
    o.z>0&&o.z<0.95&&o.xF1>0&&o.xF2>0&&o.missing_mass>1.5&&o.P1>1.25&&o.P2>1.25;
}
}

