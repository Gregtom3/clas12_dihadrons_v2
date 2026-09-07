#include "clas12dihadrons/config/AnalysisConfig.hpp"

#include <TApplication.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TGButton.h>
#include <TGClient.h>
#include <TGComboBox.h>
#include <TGFrame.h>
#include <TGLabel.h>
#include <TGNumberEntry.h>
#include <TGTab.h>
#include <TGTextEntry.h>
#include <TGTextView.h>
#include <TROOT.h>
#include <TRootEmbeddedCanvas.h>
#include <TSystem.h>
#include <TTimer.h>

#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace fs=std::filesystem;
namespace {
std::string shell_quote(const std::string& value){std::string out="'";for(char c:value){if(c=='\'')out+="'\\''";else out+=c;}return out+="'";}
std::string tail(const fs::path& path,std::size_t bytes=24000){std::ifstream in(path,std::ios::binary);if(!in)return "No log output yet.\n";in.seekg(0,std::ios::end);const auto size=in.tellg();if(size>static_cast<std::streamoff>(bytes))in.seekg(size-static_cast<std::streamoff>(bytes));else in.seekg(0);std::ostringstream out;out<<in.rdbuf();return out.str();}
std::string capture(const char* command){std::string out;char buffer[512];if(FILE* pipe=popen(command,"r")){while(fgets(buffer,sizeof(buffer),pipe))out+=buffer;pclose(pipe);}return out;}
fs::path existing_parent(fs::path p){while(!p.empty()&&!fs::exists(p))p=p.parent_path();return p.empty()?fs::current_path():p;}

class AnalysisGui final:public TGMainFrame {
 RQ_OBJECT("AnalysisGui")
 public:
 AnalysisGui(const TGWindow* parent):TGMainFrame(parent,1180,780){
  SetWindowName("CLAS12 Dihadron Analysis");SetCleanup(kDeepCleanup);
  auto* tabs=new TGTab(this,1180,760);AddFrame(tabs,new TGLayoutHints(kLHintsExpandX|kLHintsExpandY));
  build_pipeline(tabs->AddTab("Pipeline"));build_monitor(tabs->AddTab("Jobs & Logs"));build_plots(tabs->AddTab("ROOT Results"));
  MapSubwindows();Resize(GetDefaultSize());MapWindow();reload_config();timer_=new TTimer(2000);timer_->Connect("Timeout()","AnalysisGui",this,"Refresh()");timer_->TurnOn();
 }
 ~AnalysisGui() override{if(timer_)timer_->TurnOff();Cleanup();}
 void CloseWindow() override{if(local_pid_>0){status("A local analysis is still running; stop it before closing.");return;}DeleteWindow();gApplication->Terminate(0);}

 void ReloadConfig(){reload_config();}
 void Validate(){try{const auto c=clas12::dihadron::config::load(config_->GetText());status("Configuration is valid: "+std::to_string(c.datasets.size())+" datasets.");}catch(const std::exception&e){status(std::string("Configuration error: ")+e.what());}}
 void RunAll(){launch("run");}
 void BuildTree(){launch("build");}
 void FitTree(){launch("fit");}
 void StopLocal(){if(local_pid_<=0){status("No local process is running.");return;}if(kill(local_pid_,SIGTERM)==0)status("Sent SIGTERM to local process "+std::to_string(local_pid_)+".");else status("Could not stop the local process.");}
 void Refresh(){
  if(local_pid_>0){int code=0;const auto done=waitpid(local_pid_,&code,WNOHANG);if(done==local_pid_){status(WIFEXITED(code)&&WEXITSTATUS(code)==0?"Local pipeline completed successfully.":"Local pipeline stopped or failed; inspect the log.");local_pid_=-1;}}
  const auto log=tail(log_path());log_->LoadBuffer(log.c_str());
  std::string jobs="Local process: "+std::string(local_pid_>0?std::to_string(local_pid_):"idle")+"\n\n";if(mode_->GetSelected()==2){jobs+="SLURM QUEUE\n";const auto q=capture("squeue --me --noheader --format='%.18i %.10T %.32j %.12M %.20R' 2>&1");jobs+=q.empty()?"No queued or running jobs.\n":q;}jobs_->LoadBuffer(jobs.c_str());
  try{const auto s=fs::space(existing_parent(output_->GetText()));std::ostringstream text;text<<std::fixed<<std::setprecision(1)<<"Storage: "<<s.available/1073741824.0<<" GiB available / "<<s.capacity/1073741824.0<<" GiB total";storage_->SetText(text.str().c_str());}catch(...){storage_->SetText("Storage: unavailable");}
  timer_->Start(2000,kTRUE);
 }
 void LoadPlot(){
  const fs::path path=plot_file_->GetText();TFile file(path.c_str(),"READ");if(file.IsZombie()){status("Cannot open ROOT result: "+path.string());return;}auto* source=dynamic_cast<TCanvas*>(file.Get("c_asymmetry"));if(!source){status("ROOT file does not contain c_asymmetry.");return;}auto* canvas=canvas_->GetCanvas();canvas->cd();canvas->Clear();source->DrawClonePad();canvas->Modified();canvas->Update();status("Loaded "+path.string());
 }
 private:
 TGTextEntry *config_{},*executable_{},*output_{},*input_tree_{},*account_{},*partition_{},*plot_file_{};
 TGComboBox *dataset_{},*mode_{};TGNumberEntry *max_files_{},*max_events_{};TGTextView *log_{},*jobs_{};TGLabel *state_{},*storage_{};TRootEmbeddedCanvas*canvas_{};TTimer*timer_{};pid_t local_pid_{-1};std::vector<std::string> dataset_ids_;
 TGTextEntry* entry(TGCompositeFrame* row,const char* label,const char* value,int width=720){row->AddFrame(new TGLabel(row,label),new TGLayoutHints(kLHintsLeft|kLHintsCenterY,4,10,4,4));auto* e=new TGTextEntry(row,value);e->Resize(width,24);row->AddFrame(e,new TGLayoutHints(kLHintsExpandX,2,4,4,4));return e;}
 TGHorizontalFrame* row(TGCompositeFrame* parent){auto*r=new TGHorizontalFrame(parent);parent->AddFrame(r,new TGLayoutHints(kLHintsExpandX,6,6,2,2));return r;}
 TGTextButton* button(TGCompositeFrame* parent,const char* text,const char* slot){auto*b=new TGTextButton(parent,text);b->Connect("Clicked()","AnalysisGui",this,slot);parent->AddFrame(b,new TGLayoutHints(kLHintsLeft,4,4,5,5));return b;}
 void build_pipeline(TGCompositeFrame* page){
  config_=entry(row(page),"Analysis YAML","config/rga.yaml");executable_=entry(row(page),"Pipeline executable","./build/clas12-rga-pippim");output_=entry(row(page),"Output directory","results/pippim");input_tree_=entry(row(page),"Existing ROOT tree","results/pippim/Fall2018_RGA_inbending_piplus_piminus.root");
  auto* choices=row(page);choices->AddFrame(new TGLabel(choices,"Dataset"),new TGLayoutHints(kLHintsCenterY,4,10,4,4));dataset_=new TGComboBox(choices);dataset_->Resize(310,24);choices->AddFrame(dataset_,new TGLayoutHints(kLHintsLeft,2,20,4,4));choices->AddFrame(new TGLabel(choices,"Launch"),new TGLayoutHints(kLHintsCenterY,4,10,4,4));mode_=new TGComboBox(choices);mode_->AddEntry("Local",1);mode_->AddEntry("Slurm",2);mode_->Select(1);mode_->Resize(150,24);choices->AddFrame(mode_,new TGLayoutHints(kLHintsLeft,2,20,4,4));
  auto* limits=row(page);limits->AddFrame(new TGLabel(limits,"Max files (0 = all)"),new TGLayoutHints(kLHintsCenterY,4,8,4,4));max_files_=new TGNumberEntry(limits,2,8,-1,TGNumberFormat::kNESInteger,TGNumberFormat::kNEANonNegative);limits->AddFrame(max_files_,new TGLayoutHints(kLHintsLeft,2,24,4,4));limits->AddFrame(new TGLabel(limits,"Max events (0 = all)"),new TGLayoutHints(kLHintsCenterY,4,8,4,4));max_events_=new TGNumberEntry(limits,10000,12,-1,TGNumberFormat::kNESInteger,TGNumberFormat::kNEANonNegative);limits->AddFrame(max_events_,new TGLayoutHints(kLHintsLeft,2,24,4,4));
  account_=entry(row(page),"Slurm account","clas12",240);partition_=entry(row(page),"Slurm partition","production",240);
  auto* actions=row(page);button(actions,"Reload datasets","ReloadConfig()");button(actions,"Validate","Validate()");button(actions,"Run HIPO → asymmetry","RunAll()");button(actions,"Build ROOT tree","BuildTree()");button(actions,"Fit existing tree","FitTree()");button(actions,"Stop local","StopLocal()");
  state_=new TGLabel(page,"Ready.");page->AddFrame(state_,new TGLayoutHints(kLHintsExpandX,10,10,16,4));storage_=new TGLabel(page,"Storage: checking...");page->AddFrame(storage_,new TGLayoutHints(kLHintsExpandX,10,10,4,4));
 }
 void build_monitor(TGCompositeFrame* page){auto*split=new TGHorizontalFrame(page);page->AddFrame(split,new TGLayoutHints(kLHintsExpandX|kLHintsExpandY,4,4,4,4));jobs_=new TGTextView(split,410,650);log_=new TGTextView(split,740,650);split->AddFrame(jobs_,new TGLayoutHints(kLHintsExpandY,2,4,2,2));split->AddFrame(log_,new TGLayoutHints(kLHintsExpandX|kLHintsExpandY,4,2,2,2));}
 void build_plots(TGCompositeFrame* page){plot_file_=entry(row(page),"Asymmetry ROOT file","results/pippim/piplus_piminus_asymmetry.root");button(row(page),"Load ROOT canvas","LoadPlot()");canvas_=new TRootEmbeddedCanvas("results",page,1120,650);page->AddFrame(canvas_,new TGLayoutHints(kLHintsExpandX|kLHintsExpandY,6,6,6,6));}
 void status(const std::string& text){state_->SetText(text.c_str());state_->Layout();}
 fs::path log_path()const{return fs::path(output_->GetText())/"gui-pipeline.log";}
 std::string selected_dataset()const{const int id=dataset_->GetSelected();if(id<1||static_cast<size_t>(id)>dataset_ids_.size())throw std::runtime_error("select a dataset");return dataset_ids_[id-1];}
 void reload_config(){try{const auto c=clas12::dihadron::config::load(config_?config_->GetText():"config/rga.yaml");dataset_->RemoveEntries(0,9999);dataset_ids_.clear();int i=1;for(const auto&d:c.datasets){if(d.kind==clas12::dihadron::config::DatasetKind::data){dataset_ids_.push_back(d.id);dataset_->AddEntry(d.id.c_str(),i++);}}if(!dataset_ids_.empty())dataset_->Select(1);status("Loaded datasets from configuration.");}catch(const std::exception&e){status(std::string("Could not load configuration: ")+e.what());}}
 std::vector<std::string> command(const std::string& action){const std::string exe=executable_->GetText(),data=selected_dataset(),out=output_->GetText(),files=std::to_string(static_cast<unsigned long long>(max_files_->GetNumber())),events=std::to_string(static_cast<unsigned long long>(max_events_->GetNumber()));if(action=="fit")return{exe,"fit",input_tree_->GetText(),out,data};if(action=="build")return{exe,"build",config_->GetText(),data,(fs::path(out)/(data+"_piplus_piminus.root")).string(),files,events};return{exe,"run",config_->GetText(),data,out,files,events};}
 void launch(const std::string& action){try{if(local_pid_>0)throw std::runtime_error("a local pipeline is already running");const auto args=command(action);fs::create_directories(output_->GetText());if(mode_->GetSelected()==2)submit_slurm(args,action);else launch_local(args);}catch(const std::exception&e){status(std::string("Launch failed: ")+e.what());}}
 void launch_local(const std::vector<std::string>& args){const auto log=log_path();local_pid_=fork();if(local_pid_<0)throw std::runtime_error("fork failed");if(local_pid_==0){const int fd=open(log.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0644);if(fd>=0){dup2(fd,STDOUT_FILENO);dup2(fd,STDERR_FILENO);close(fd);}std::vector<char*> av;for(const auto&a:args)av.push_back(const_cast<char*>(a.c_str()));av.push_back(nullptr);execv(av[0],av.data());_exit(127);}status("Started local pipeline PID "+std::to_string(local_pid_)+".");}
 void submit_slurm(const std::vector<std::string>& args,const std::string& action){const auto script=fs::path(output_->GetText())/"gui-submit.sh";std::ofstream out(script);out<<"#!/usr/bin/env bash\nset -euo pipefail\nexec";for(const auto&a:args)out<<' '<<shell_quote(a);out<<'\n';out.close();chmod(script.c_str(),0755);const auto pid=fork();if(pid<0)throw std::runtime_error("fork failed");if(pid==0){const auto log=log_path().string(),name="pippim-"+action,account=std::string(account_->GetText()),partition=std::string(partition_->GetText()),script_s=script.string();execlp("sbatch","sbatch","--job-name",name.c_str(),"--output",log.c_str(),"--account",account.c_str(),"--partition",partition.c_str(),script_s.c_str(),static_cast<char*>(nullptr));_exit(127);}int code=0;waitpid(pid,&code,0);if(!WIFEXITED(code)||WEXITSTATUS(code)!=0)throw std::runtime_error("sbatch failed");status("Submitted Slurm job; refresh Jobs & Logs for status.");}
};
}

int main(int argc,char**argv){TApplication application("clas12-gui",&argc,argv);AnalysisGui gui(gClient->GetRoot());application.Run();return 0;}

