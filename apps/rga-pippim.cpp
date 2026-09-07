#include "clas12dihadrons/Modulation.hpp"
#include "clas12dihadrons/analysis/Kinematics.hpp"
#include "clas12dihadrons/config/AnalysisConfig.hpp"
#include "clas12dihadrons/fit/BeamSpinFit.hpp"

#include <HipoChain.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TGraphErrors.h>
#include <TH1D.h>
#include <TMatrixDSym.h>
#include <TStyle.h>
#include <TTree.h>
#include <glob.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using namespace clas12::dihadron;
namespace fs=std::filesystem;
namespace {
constexpr double pi=3.14159265358979323846,mpi=0.13957039;
struct Row { int run{},event{},helicity{}; double polarization{},Q2{},x{},y{},W{},Mh{},z{},z1{},z2{},xF{},xF1{},xF2{},pT{},phi_h{},phi_R{},Mx{},P1{},P2{}; };
struct RawParticle { int index{},pid{},status{},sector{}; double chi2{},p{},theta{},phi{},vx{},vy{},vz{},pcal{},ecin{},ecout{},lv{},lw{}; analysis::FourVector four; };

std::string expand_env(std::string value){
 for(size_t pos=0;(pos=value.find("${",pos))!=std::string::npos;){const auto end=value.find('}',pos+2);if(end==std::string::npos)throw std::runtime_error("unterminated environment variable in path");const auto key=value.substr(pos+2,end-pos-2);const char* v=std::getenv(key.c_str());if(!v)throw std::runtime_error("environment variable is not set: "+key);value.replace(pos,end-pos+1,v);pos+=std::char_traits<char>::length(v);}return value;
}
std::vector<std::string> discover(const config::Dataset& d,size_t limit){std::vector<std::string> out;for(auto pattern:d.source_globs){glob_t g{};const auto p=expand_env(pattern);const int rc=glob(p.c_str(),GLOB_TILDE,nullptr,&g);if(rc!=0&&rc!=GLOB_NOMATCH){globfree(&g);throw std::runtime_error("glob failed: "+p);}for(size_t i=0;i<g.gl_pathc;++i)out.emplace_back(g.gl_pathv[i]);globfree(&g);}std::sort(out.begin(),out.end());out.erase(std::unique(out.begin(),out.end()),out.end());if(limit&&out.size()>limit)out.resize(limit);if(out.empty())throw std::runtime_error("no HIPO files matched dataset "+d.id);return out;}
const config::Dataset& dataset(const config::AnalysisConfig& c,std::string_view id){for(const auto&d:c.datasets)if(d.id==id)return d;throw std::runtime_error("unknown dataset: "+std::string(id));}
bool flip_helicity(int run){return (run>=5032&&run<=5666)||(run>=6616&&run<=6783);}
double polarization(int run){if(run>=5032&&run<=5332)return .8592;if(run>=5333&&run<=5666)return .8922;if(run>=6616&&run<=6783)return .8453;throw std::runtime_error("no RG-A polarization for run "+std::to_string(run));}
double energy_for_pid(int pid,double p){const double m=pid==11?.00051099895:mpi;return std::sqrt(p*p+m*m);}
bool sampling_fraction(const RawParticle&e){
 static constexpr double mu0[6]={.2531,.2550,.2514,.2494,.2528,.2521},mu1[6]={-.6502,-.7472,-.7674,-.4913,-.3988,-.703},mu2[6]={4.939,5.350,5.102,6.440,6.149,4.957};
 static constexpr double s0[6]={.002726,.004157,.005222,.005398,.008453,.006553},s1[6]={1.062,.859,.5564,.6576,.3242,.4423},s2[6]={-4.089,-3.318,-2.078,-2.565,-.8223,-1.274};
 if(e.sector<1||e.sector>6||e.p<=0)return false;const int s=e.sector-1;const double sf=(e.pcal+e.ecin+e.ecout)/e.p,mean=mu0[s]+mu1[s]/1000*std::pow(e.p-mu2[s],2),sigma=s0[s]+s1[s]/(10*(e.p-s2[s]));return std::abs(sf-mean)<3.5*sigma&&(e.p<4.5||e.ecin/e.p>.2-e.pcal/e.p);
}
bool electron_cuts(const RawParticle&e,int run){const double deg=e.theta*180/pi;const bool in=run<5333||run>=6616;return e.status>-3000&&e.status<=-2000&&deg>=5&&deg<=35&&e.p>=2&&e.pcal>.07&&e.lv>=9&&e.lw>=9&&(in?(e.vz>=-8&&e.vz<=3):(e.vz>=-10&&e.vz<=2.5))&&sampling_fraction(e);}
bool pion_cuts(const RawParticle&p,const RawParticle&e){const double deg=p.theta*180/pi,C=p.pid==211?.88:.93;double upper=C*3;if(p.p>=2.44)upper=C*(.00869+14.98587*std::exp(-p.p/1.18236)+1.81751*std::exp(-p.p/4.86394));return !(p.status>=4000&&p.status<5000)&&deg>=5&&deg<=35&&p.chi2>=-3*C&&p.chi2<upper&&std::abs(p.vz-e.vz)<20;}

void attach_calo(clas12::clas12reader* r,int bank,int ipindex,int ilayer,int isector,int ienergy,int ilv,int ilw,RawParticle&p){for(int row=0;row<r->getBank(bank)->getRows();++row){if(r->getBank(bank)->getInt(ipindex,row)!=p.index)continue;const int layer=r->getBank(bank)->getInt(ilayer,row);const double e=r->getBank(bank)->getFloat(ienergy,row);if(layer==1){p.pcal=e;p.sector=r->getBank(bank)->getInt(isector,row);p.lv=r->getBank(bank)->getFloat(ilv,row);p.lw=r->getBank(bank)->getFloat(ilw,row);}else if(layer==4)p.ecin=e;else if(layer==7)p.ecout=e;}}

void make_tree(const config::AnalysisConfig& cfg,const config::Dataset& data,const fs::path& output,size_t max_files,size_t max_events){
 const auto files=discover(data,max_files);fs::create_directories(output.parent_path());const auto temporary=output.string()+".partial";
 const bool mc=data.kind==config::DatasetKind::monte_carlo;if(!mc)if(const char* rcdb=std::getenv("CLAS12_RCDB_ROOT"))clas12::clas12databases::SetRCDBRootConnection(rcdb);
 clas12root::HipoChain chain;for(const auto&f:files)chain.Add(f);auto* setup=chain.GetC12Reader();if(mc)setup->db()->turnOffQADB();setup->addAtLeastPid(11,1);setup->addAtLeastPid(211,1);setup->addAtLeastPid(-211,1);auto&reader=chain.C12ref();if(!mc)reader->db()->qadb_requireOkForAsymmetry(true);
 const int br=reader->addBank("RUN::config"),ir=reader->getBankOrder(br,"run"),ie=reader->getBankOrder(br,"event");const int bc=reader->addBank("REC::Calorimeter"),ip=reader->getBankOrder(bc,"pindex"),il=reader->getBankOrder(bc,"layer"),is=reader->getBankOrder(bc,"sector"),ien=reader->getBankOrder(bc,"energy"),ilv=reader->getBankOrder(bc,"lv"),ilw=reader->getBankOrder(bc,"lw");
 TFile file(temporary.c_str(),"RECREATE");TTree tree("dihadron","fit-ready pi+pi- candidates");Row o;
 tree.Branch("run",&o.run);tree.Branch("event",&o.event);tree.Branch("helicity",&o.helicity);tree.Branch("polarization",&o.polarization);tree.Branch("Q2",&o.Q2);tree.Branch("x",&o.x);tree.Branch("y",&o.y);tree.Branch("W",&o.W);tree.Branch("Mh",&o.Mh);tree.Branch("z",&o.z);tree.Branch("z1",&o.z1);tree.Branch("z2",&o.z2);tree.Branch("xF",&o.xF);tree.Branch("xF1",&o.xF1);tree.Branch("xF2",&o.xF2);tree.Branch("pT",&o.pT);tree.Branch("phi_h",&o.phi_h);tree.Branch("phi_R",&o.phi_R);tree.Branch("Mx",&o.Mx);tree.Branch("P1",&o.P1);tree.Branch("P2",&o.P2);
 size_t read=0,selected=0,candidates=0;while((max_events==0||read<max_events)&&chain.Next()){++read;o.run=reader->getBank(br)->getInt(ir,0);o.event=reader->getBank(br)->getInt(ie,0);if(!mc&&!reader->db()->qa()->isOkForAsymmetry(o.run,o.event))continue;o.helicity=reader->event()->getHelicity();if(flip_helicity(o.run))o.helicity*=-1;if(!mc&&o.helicity==0)continue;o.polarization=mc?1.0:polarization(o.run);
  std::vector<RawParticle> particles;for(const auto&p:reader->getDetParticles()){RawParticle q;q.index=p->getIndex();q.pid=p->getPid();q.status=p->getStatus();q.chi2=p->getChi2Pid();q.p=p->getP();q.theta=p->getTheta();q.phi=p->getPhi();q.vx=p->par()->getVx();q.vy=p->par()->getVy();q.vz=p->par()->getVz();q.four={q.p*std::sin(q.theta)*std::cos(q.phi),q.p*std::sin(q.theta)*std::sin(q.phi),q.p*std::cos(q.theta),energy_for_pid(q.pid,q.p)};attach_calo(reader.get(),bc,ip,il,is,ien,ilv,ilw,q);particles.push_back(q);}
  auto electron=particles.end();for(auto it=particles.begin();it!=particles.end();++it)if(it->pid==11&&(electron==particles.end()||it->four.energy>electron->four.energy))electron=it;if(electron==particles.end()||!electron_cuts(*electron,o.run))continue;
  const double Q2=2*data.beam_energy_gev*electron->four.energy*(1-std::cos(electron->theta)),y=(data.beam_energy_gev-electron->four.energy)/data.beam_energy_gev;if(Q2<1||y>.8)continue;++selected;
  std::vector<const RawParticle*> plus,minus;for(const auto&p:particles)if((p.pid==211||p.pid==-211)&&pion_cuts(p,*electron))(p.pid==211?plus:minus).push_back(&p);
  for(const auto*a:plus)for(const auto*b:minus){try{const auto k=analysis::compute_dihadron_kinematics(data.beam_energy_gev,electron->four,a->four,b->four);if(!analysis::passes_pippim_analysis_cuts(k))continue;o.Q2=k.Q2;o.x=k.x;o.y=k.y;o.W=k.W;o.Mh=k.Mh;o.z=k.z;o.z1=k.z1;o.z2=k.z2;o.xF=k.xF;o.xF1=k.xF1;o.xF2=k.xF2;o.pT=k.pT;o.phi_h=k.phi_h;o.phi_R=k.phi_R;o.Mx=k.missing_mass;o.P1=k.P1;o.P2=k.P2;tree.Fill();++candidates;}catch(const std::domain_error&){}}
  if(read%100000==0)std::cout<<read<<" events, "<<candidates<<" candidates\n";
 }
 tree.Write();TTree meta("metadata","processing summary");std::string dataset_id=data.id,selection="rga_baseline_v1";ULong64_t input_events=read,selected_events=selected,dihadron_candidates=candidates;meta.Branch("dataset",&dataset_id);meta.Branch("selection",&selection);meta.Branch("input_events",&input_events);meta.Branch("selected_events",&selected_events);meta.Branch("dihadron_candidates",&dihadron_candidates);meta.Fill();meta.Write();file.Close();if(fs::exists(output))fs::remove(output);fs::rename(temporary,output);std::cout<<"WROTE "<<output<<" ("<<candidates<<" pi+pi- candidates)\n";
}

void fit_tree(const fs::path&input,const fs::path&outdir,std::string dataset_id){
 fs::create_directories(outdir);TFile in(input.c_str(),"READ");auto*tree=dynamic_cast<TTree*>(in.Get("dihadron"));if(!tree)throw std::runtime_error("missing dihadron tree");int hel;double pol,h,r;tree->SetBranchAddress("helicity",&hel);tree->SetBranchAddress("polarization",&pol);tree->SetBranchAddress("phi_h",&h);tree->SetBranchAddress("phi_R",&r);std::vector<fit::FitEvent> events;events.reserve(tree->GetEntries());for(Long64_t i=0;i<tree->GetEntries();++i){tree->GetEntry(i);if(hel==1||hel==-1)events.push_back({hel,pol,h,r,1});}const auto result=fit::fit_beam_spin(events);
 nlohmann::json j={{"schema_version",1},{"dataset",dataset_id},{"channel","piplus_piminus"},{"bin_id","integrated"},{"background_method","none"},{"counts",{{"input_events",events.size()},{"selected_events",events.size()},{"dihadron_candidates",events.size()}}}};const auto&defs=beam_spin_modulations();for(size_t i=0;i<7;++i){j["amplitudes"][std::string(defs[i].id)]={{"value",result.amplitudes[i]},{"uncertainty",result.errors[i]}};j["covariance"].push_back(result.covariance[i]);}j["fit"]={{"converged",result.converged},{"iterations",result.iterations},{"negative_log_likelihood",result.negative_log_likelihood},{"helicity_balanced",true}};std::ofstream(outdir/"piplus_piminus_integrated.json")<<j.dump(2)<<'\n';
 TFile root((outdir/"piplus_piminus_asymmetry.root").c_str(),"RECREATE");TMatrixDSym cov(7);for(int i=0;i<7;++i)for(int k=0;k<7;++k)cov(i,k)=result.covariance[i][k];cov.Write("covariance");TTree estimates("asymmetries","seven simultaneous beam-spin amplitudes");std::string id,formula;double value,error;estimates.Branch("id",&id);estimates.Branch("formula",&formula);estimates.Branch("value",&value);estimates.Branch("error",&error);for(size_t i=0;i<7;++i){id=defs[i].id;formula=defs[i].angular_expression;value=result.amplitudes[i];error=result.errors[i];estimates.Fill();}estimates.Write();
 gStyle->SetOptStat(0);TCanvas canvas("c_asymmetry","pi+pi- beam-spin asymmetries",1100,700);TGraphErrors graph(7);for(int i=0;i<7;++i){graph.SetPoint(i,i+1,result.amplitudes[i]);graph.SetPointError(i,0,result.errors[i]);}graph.SetTitle("#pi^{+}#pi^{-} beam-spin asymmetries;modulation;amplitude");graph.SetMarkerStyle(20);graph.SetMarkerSize(1.2);graph.Draw("AP");for(int i=0;i<7;++i)graph.GetXaxis()->SetBinLabel(graph.GetXaxis()->FindBin(i+1),defs[i].legacy_label.data());canvas.SetGridy();canvas.Write();canvas.SaveAs((outdir/"piplus_piminus_asymmetry.pdf").c_str());canvas.SaveAs((outdir/"piplus_piminus_asymmetry.png").c_str());root.Close();if(!result.converged)throw std::runtime_error("BSA fit did not converge");std::cout<<"WROTE asymmetry ROOT, JSON, PDF, and PNG in "<<outdir<<'\n';
}
void usage(){std::cerr<<"Usage:\n  clas12-rga-pippim build CONFIG DATASET OUTPUT.root [MAX_FILES] [MAX_EVENTS]\n  clas12-rga-pippim fit INPUT.root OUTPUT_DIR DATASET\n  clas12-rga-pippim run CONFIG DATASET OUTPUT_DIR [MAX_FILES] [MAX_EVENTS]\n";}
}
int main(int argc,char**argv){try{if(argc<2){usage();return 2;}const std::string cmd=argv[1];if(cmd=="fit"){if(argc!=5){usage();return 2;}fit_tree(argv[2],argv[3],argv[4]);return 0;}if(cmd!="build"&&cmd!="run"){usage();return 2;}if(argc<5||argc>7){usage();return 2;}const auto cfg=config::load(argv[2]);const auto&d=dataset(cfg,argv[3]);const size_t mf=argc>5?std::stoull(argv[5]):cfg.limits.max_files,me=argc>6?std::stoull(argv[6]):cfg.limits.max_events_per_file;if(cmd=="build")make_tree(cfg,d,argv[4],mf,me);else{const fs::path out=argv[4],root=out/(d.id+"_piplus_piminus.root");make_tree(cfg,d,root,mf,me);fit_tree(root,out,d.id);}return 0;}catch(const std::exception&e){std::cerr<<"clas12-rga-pippim: "<<e.what()<<'\n';return 1;}}

