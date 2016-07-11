#include "SampleAnalyzer/User/Analyzer/cms_exo_15_001.h"
using namespace MA5;
using namespace std;

// -----------------------------------------------------------------------------
// Initialize
// function called one time at the beginning of the analysis
// -----------------------------------------------------------------------------
bool cms_exo_15_001::Initialize(const MA5::Configuration& cfg, const std::map<std::string,std::string>& parameters)
{
        // Information on the analysis, authors, ...
        // VERY IMPORTANT FOR DOCUMENTATION, TRACEABILITY, BUG REPORTS
        INFO << "        <><><><><><><><><><><><><><><><><><><><><><><><><>" << endmsg;
        INFO << "        <>  Analysis: CMS-EXO-15-001, arXiv:1512.01224  <>" << endmsg;
        INFO << "        <>            (narrow resonances, dijets)       <>" << endmsg;
        INFO << "        <>  Recasted by: A.MOLTER                       <>" << endmsg;
        INFO << "        <>  Contact: alexis.molter@protonmail.com       <>" << endmsg;
        INFO << "        <>  Based on MadAnalysis 5 v1.3                 <>" << endmsg;
        INFO << "        <>  For more information, see                   <>" << endmsg;
        INFO << "        <>  http://madanalysis.irmp.ucl.ac.be/wiki/PhysicsAnalysisDatabase" << endmsg;
        INFO << "        <><><><><><><><><><><><><><><><><><><><><><><><><>" << endmsg;

        // initialize variables, histos
        Manager()->AddRegionSelection("All");
        Manager()->AddRegionSelection("wide-jet");
        Manager()->AddRegionSelection("mass> 1.2");

        Manager()->AddCut("trigger");

        string SRForWideJetCut[]= {"wide-jet","mass> 1.2"};
        Manager()->AddCut("2+ jets", SRForWideJetCut);
        //Manager()->AddCut("deltaR (leading jet, jet)< 1.1");
        Manager()->AddCut("delta eta (wide jet, wide jet)< 1.3", SRForWideJetCut);

        string SRForMassCut[]= {"mass> 1.2"};
        Manager()->AddCut("mjj> 1.2", SRForMassCut);

        Manager()->AddHisto("invariante mass", 800, 0, 8000, SRForMassCut);
        Manager()->AddHisto("pT leading jet", 350, 0, 3500, SRForMassCut);
        Manager()->AddHisto("eta leading jet", 30, -3, 3, SRForMassCut);

        return true;
}

// -----------------------------------------------------------------------------
// Finalize
// function called one time at the end of the analysis
// -----------------------------------------------------------------------------
void cms_exo_15_001::Finalize(const SampleFormat& summary, const std::vector<SampleFormat>& files)
{

}

// -----------------------------------------------------------------------------
// Execute
// function called each time one event is read
// -----------------------------------------------------------------------------
bool cms_exo_15_001::Execute(SampleFormat& sample, const EventFormat& event)
{

        if(event.rec()!=0){
                double myEventWeight;
                if(Configuration().IsNoEventWeight()) myEventWeight=1.;
                else if(event.mc()->weight()!=0.) myEventWeight=event.mc()->weight();
                else{
                        WARNING << "Found one event with a zero weight. Skipping..." << endmsg;
                        return false;
                }
                Manager()->InitializeForNewEvent(myEventWeight);
                Manager()->SetCurrentEventWeight(myEventWeight);

                //Declare empty container
                vector<const RecJetFormat*> myJets;
                //Declare wide-jet 
                TLorentzVector widejetone;
                TLorentzVector widejettwo;

                //Declare transverse hadronic energy and pass criteria
                int good= 0;
                double HT= 0;

                //Compute transverse hadronic energy and look if the event have a jet with pT> 500
                for(unsigned int j= 0; j<event.rec()->jets().size(); j++){
                        const RecJetFormat *CurrentJet= &(event.rec()->jets()[j]);
                        double pt= CurrentJet->momentum().Pt();
                        double eta= CurrentJet->momentum().Eta();
                        if(pt> 40 && fabs(eta)< 3) HT+= pt;
                        if(pt> 500) good++;
                }

                //Put jet with pT> 30 and eta< 2.5 inside a vector
                if(Manager()->ApplyCut(HT> 800 || good>= 1, "trigger")){
                        for(unsigned int j= 0; j<event.rec()->jets().size(); j++){
                                const RecJetFormat *CurrentJet= &(event.rec()->jets()[j]);
                                double pt= CurrentJet->momentum().Pt();
                                double eta= CurrentJet->momentum().Eta();
                                if(!(pt> 30 && fabs(eta)< 2.5)) continue;
                                myJets.push_back(CurrentJet);
                        }
                }

                //Sort jet's container
                SORTER->sort(myJets);

                if(Manager()->ApplyCut(myJets.size()>1, "2+ jets")){
                        //Leading jet become wide-jet
                        widejetone= myJets[0]->momentum();
                        widejettwo= myJets[1]->momentum();

                        //Nearest jet will addition with wide-jet
                        for(unsigned int j= 2; j<myJets.size(); j++){
                                double deltaR1= myJets[0]->dr(myJets[j]);
                                double deltaR2= myJets[1]->dr(myJets[j]);

                                if(deltaR1< 1.1 && deltaR2>= 1.1) widejetone+= myJets[j]->momentum();

                                if(deltaR1>= 1.1 && deltaR2< 1.1) widejettwo+= myJets[j]->momentum();

                                if(deltaR1< 1.1 && deltaR2< 1.1){
                                        if(deltaR1>deltaR2) widejettwo+= myJets[j]->momentum();
                                        if(deltaR1<deltaR2) widejetone+= myJets[j]->momentum();
                                        if(deltaR1==deltaR2) widejetone+= myJets[j]->momentum();
                                }
                        }
                }

                if(!Manager()->ApplyCut(fabs(widejetone.Eta()-widejettwo.Eta())< 1.3, "delta eta (wide jet, wide jet)< 1.3")){
                        return true;
                }

                TLorentzVector mom= widejetone+widejettwo;
                double mass= mom.M();

                if(!Manager()->ApplyCut(mass> 1200, "mjj> 1.2")){
                        return true;
                }
                
                        Manager()->FillHisto("invariante mass", mass);
                        Manager()->FillHisto("pT leading jet", mom.Pt());
                        Manager()->FillHisto("eta leading jet", mom.Eta());

        }

        return true;
}
