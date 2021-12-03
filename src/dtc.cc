#include <include/FIFO.h>
#include <interface/Circuit.h>
#include <include/Component.h>
#include <include/Ports.h>
#include <ctime>
#include <deque>
#include <regex>
#include <algorithm>
#include <iostream>
#include <boost/filesystem.hpp>
#include <fstream>
#include <stdint.h>
#include <memory>
#include <random>
#include <limits>
#include <assert.h>
#include <bitset>
#include <stdexcept>
#include <interface/EventBoundaryFinder.h>
#include <interface/ChipDataPlayer.h>
#include <interface/DTCEventBuilder.h>
#include <interface/ChipConfigReader.h>
#include "TFile.h"
#include "TDirectory.h"
#include "TList.h"
#include "TKey.h"
#include "TTreeReader.h"
#include "TTreeReaderValue.h"

using namespace std;
//using namespace boost::filesystem;

typedef FIFO<uint64_t> FIFO64;
typedef FIFO<uint16_t> FIFO16;


int main(int argc, char* argv[]) {

    // Default parameters
    bool DEBUG=false;
    bool RANDOM_L1=true;
    bool DRY_RUN=false;
    bool TRIGGER_RULE=true;
    int OUTPUT_LINKS=12;
    std::string input_dirname("input_dtc11_10kevt");
    std::string config_filename("config/default.config");
    std::string dtcname("dtc");
    std::string tag("");
    int nevents=1000;

    // argument parsing
    std::string help_msg("Usage: ./build/dtc [options]\n\
            --help:                         display this message.\n\
            --debug:                        enable some debug output.\n\
            --dry-run:                      print out event builder assignment without actually running the simulation.\n\
            --input/-i INPUT_DIRNAME:       Change the input directory name, by default uses input_10k.\n\
            --config/-c CONFIG_FILENAME:    Config file that include n-elinks and n-events-compression per chip, by default uses config/default.config.\n\
            --dtc/-d DTC:                   DTC number to simulate. \n\
            --nevents/-n N_Events:          Number of events to run before the end of simulation. Default value = 1000.\n\
            --random-l1 L1-TYPE:            L1-TYPE is boolean, set whether L1 trigger rate random with average of 750kHZ or just constantly 750kHz.\n\
            --no-trigger-rule:              Only effective for the random L1 trigger mode, disables the trigger rules.\n\
            --output-links N_OptLinks:      set the number of output optical links, each connects to a event builder. Default value = 12.\n");
    for (int iarg =0; iarg<argc; iarg++) {
        if (iarg==0) continue;
        if (std::string(argv[iarg])=="--help") {std::cerr<<help_msg<<std::endl; return 0;}
        if (std::string(argv[iarg])=="--debug") {DEBUG=true;continue;}
        if (std::string(argv[iarg])=="--input" || std::string(argv[iarg])=="-i") {
            if (iarg+1 < argc) {
                input_dirname = argv[++iarg];
            }
            else {
                std::cerr<<"--input/-i option requires one argument."<<std::endl;
                return 1;
            }
            continue;
        }
        if (std::string(argv[iarg])=="--config" || std::string(argv[iarg])=="-c") {
            if (iarg+1 < argc) {
                config_filename = argv[++iarg];
            }
            else {
                std::cerr<<"--config/-c option requires one argument."<<std::endl;
                return 1;
            }
            continue;
        }
        if (std::string(argv[iarg])=="--dtc" || std::string(argv[iarg])=="-d") {
            if (iarg+1 < argc) {
                dtcname += argv[++iarg]; // result into dtcname="dtc15" for example
            }
            else {
                std::cerr<<"--dtc/-d option requires one argument."<<std::endl;
                return 1;
            }
            continue;
        }
        if (std::string(argv[iarg])=="--tag" || std::string(argv[iarg])=="-t") {
            if (iarg+1 < argc) {
                tag = string("_") + argv[++iarg];
            }
            else {
                std::cerr<<"--tag/-t option requires one argument."<<std::endl;
                return 1;
            }
            continue;
        }
        if (std::string(argv[iarg])=="--random-l1") {
            if (iarg+1 < argc) {
                std::string input_random_l1(argv[++iarg]);
                if (input_random_l1=="0" || input_random_l1=="false" || input_random_l1=="False") RANDOM_L1=false;
            }
            else {
                std::cerr<<"--random-l1 option requires one argument."<<std::endl;
                return 1;
            }
            continue;
        }
        if (std::string(argv[iarg])=="--no-trigger-rule") {
            TRIGGER_RULE = false;
            continue;
        }
        if (std::string(argv[iarg])=="--dry-run") {
            DRY_RUN = true;
            continue;
        }
        if (std::string(argv[iarg])=="--output-links") {
            if (iarg+1 < argc) {
                std::string output_links_str(argv[++iarg]);
                OUTPUT_LINKS = stoi(output_links_str);
            }
            else {
                std::cerr<<"--output-links option requires one argument."<<std::endl;
                return 1;
            }
            continue;
        }
        if (std::string(argv[iarg])=="--nevents" || std::string(argv[iarg])=="-n") {
            if (iarg+1 < argc) {
                std::string input_nevents_str(argv[++iarg]);
                nevents = stoi(input_nevents_str);
            }
            else {
                std::cerr<<"--nevents/-n option requires one argument."<<std::endl;
                return 1;
            }
            continue;
        }
        std::cerr<<"Unknow option/argument: "<<argv[iarg]<<std::endl;
        return 2;
    }

    // print out parameters and setup the output dir
    std::string output_dir("output/");
    std::size_t pos = 0; 
    while(input_dirname.back()=='/') input_dirname.pop_back();
    if (input_dirname.find("/") != std::string::npos) {
        pos = input_dirname.find("/")+1; 
    }
    std::string input_tag = input_dirname.substr(pos);
    output_dir+=input_tag+"_"+dtcname+tag;
    if (RANDOM_L1) output_dir+="_randomL1"; else output_dir+="_constL1";
    if (RANDOM_L1 && !TRIGGER_RULE)  output_dir+="NoTriggerRule";
    std::cout<<"Running Mode: Randome L1="<<RANDOM_L1<<" TRIGGER_RULE="<<TRIGGER_RULE<<" OUTPUT_LINKS="<<OUTPUT_LINKS;
    output_dir+="_olinks";
    output_dir+=to_string(OUTPUT_LINKS);
    output_dir+="_N";
    output_dir+=to_string(nevents);
    std::cout<<" Output dir="<<output_dir<<std::endl;
    boost::filesystem::create_directories("output");
    boost::filesystem::create_directories(output_dir);

    // read the root file, get a list of event sizes for relavent chips
    // save chip the ordered chip information into txt file
    string root_file_name = (input_dirname + "/chiptrees.root");
    TFile* input_root_file = TFile::Open(root_file_name.c_str());
    TDirectory* dtcdir = (TDirectory*) input_root_file->Get(dtcname.c_str());
    assert(dtcdir); //make sure this directory exists in root file
    int nchips = dtcdir->GetNkeys();
    assert(nchips>0);
    std::vector<TTree*> vec_trees(nchips);
    int itree = 0;
    // get list of chip trees
    for (const auto && key : *dtcdir->GetListOfKeys()) {
        vec_trees[itree] = (TTree*) ((TKey*)key)->ReadObj();
        itree++;
    }
    int input_events = vec_trees[0]->GetEntries();
    // initialize the chip sizes as 2d vectors, rows=input_events, cols=nchips
    std::vector<std::vector<unsigned short>> vec_event_chip_sizes(input_events, std::vector<unsigned short>(nchips));
    std::vector<std::vector<unsigned short>> vec_event_chip_parse_time(input_events, std::vector<unsigned short>(nchips));
    std::vector<std::string> chip_basename_list(nchips);
    // be ready to write the order of chips into txt file
    std::ofstream os_chip_order(output_dir+"/ordered_chips.csv");
    std::cout<<"Reading root file for "<<dtcname<<" nchips="<<nchips<<std::endl;
    os_chip_order<<"index  dtc    barrel layer  disk   module chip"<<std::endl;
    for (int ichip=0; ichip<nchips; ichip++) {
        TTreeReader chip_reader(vec_trees[ichip]);
        TTreeReaderValue<int>  branch_dtc          ( chip_reader , "dtc");
        TTreeReaderValue<bool> branch_barrel       ( chip_reader , "barrel");
        TTreeReaderValue<int>  branch_layer        ( chip_reader , "layer");
        TTreeReaderValue<int>  branch_disk         ( chip_reader , "disk");
        TTreeReaderValue<int>  branch_module_id    ( chip_reader , "module_id");
        TTreeReaderValue<int>  branch_module_index ( chip_reader , "module_index");
        TTreeReaderValue<int>  branch_size_pad     ( chip_reader , "stream_size_chip_aurora_pad");
        TTreeReaderValue<int>  branch_parse_time   ( chip_reader , "parsing_time");
        int ievent = 0;
        chip_reader.Restart();
        while (chip_reader.Next()) {
            assert(*branch_size_pad < 65536);
            assert(*branch_parse_time < 65536);
            vec_event_chip_sizes[ievent][ichip] = *branch_size_pad;
            vec_event_chip_parse_time[ievent][ichip] = *branch_parse_time;
            ievent += 1;
        }
        chip_reader.Restart();
        chip_reader.Next();
        assert(ievent==input_events);
        std::regex rgx("module([0-9]+)chip([0-9]+)");
        std::smatch matches;
        std::string treename = vec_trees[ichip]->GetName();
        std::regex_search(treename, matches, rgx);
        assert(matches.size()==3);
        os_chip_order<<std::setw(7)<<std::left<<matches[1].str();
        os_chip_order<<std::setw(7)<<std::left<<*branch_dtc;
        os_chip_order<<std::setw(7)<<std::left<<*branch_barrel;
        os_chip_order<<std::setw(7)<<std::left<<*branch_layer;
        os_chip_order<<std::setw(7)<<std::left<<*branch_disk;
        os_chip_order<<std::setw(7)<<std::left<<*branch_module_id;
        os_chip_order<<std::setw(7)<<std::left<<matches[2].str();
        os_chip_order<<std::endl;
        // construct basename
        std::string chip_basename("dtc");
        chip_basename += std::to_string(*branch_dtc)    + "isBarrel";
        chip_basename += std::to_string(*branch_barrel) + "layer";
        chip_basename += std::to_string(*branch_layer)  + "disk";
        chip_basename += std::to_string(*branch_disk)   + "module";
        chip_basename += std::to_string(*branch_module_id) + "chip";
        chip_basename += matches[2].str();
        chip_basename_list[ichip] = chip_basename;
    }
    os_chip_order.close();

    // read config
    std::cout<< "Number of chips mapped to DTC = " << nchips <<endl;
    ChipConfigReader config(config_filename);

    // assign the chips to the event builders
    std::vector<int> eb_assignment = config.assign_chips_to_event_builders(chip_basename_list, OUTPUT_LINKS);
    std::vector<int> nchips_per_eb(OUTPUT_LINKS, 0);
    std::vector<int> ichip_to_ichip_per_eb(nchips);
    for (int ichip=0; ichip<nchips; ichip++) {
        int ieb = eb_assignment[ichip];
        ichip_to_ichip_per_eb[ichip] = nchips_per_eb[ieb];
        nchips_per_eb[ieb]++;
    }
    // save the eb assignment somewhere
    std::ofstream log_eb_assignment(output_dir+"/eb_assignment.txt");
    std::cout<<"nchips in each eb:"<<std::endl;
    for (auto nchips_in_each_eb : nchips_per_eb) {
        log_eb_assignment<<nchips_in_each_eb<<"\t";
        std::cout        <<nchips_in_each_eb<<"\t";
        assert(nchips_in_each_eb>0);
    }
    log_eb_assignment<<std::endl;
    std::cout        <<std::endl;
    for (int ieb=0; ieb<OUTPUT_LINKS; ieb++) {
        log_eb_assignment<<ieb<<":\t";
        std::cout        <<ieb<<":\t";
        float sum_of_avgsize = 0;
        for (int ichip=0; ichip<nchips; ichip++) {
            if (ieb==eb_assignment[ichip]) {
                log_eb_assignment<<chip_basename_list[ichip]<<"\t";
                std::cout        <<chip_basename_list[ichip]<<"\t";
                sum_of_avgsize+=config.GetAvgSize(chip_basename_list[ichip]);
            }
        }
    log_eb_assignment<<"sum_of_avgsize="<<sum_of_avgsize<<std::endl;
    std::cout        <<"sum_of_avgsize="<<sum_of_avgsize<<std::endl;
    }
    log_eb_assignment.close();

    // setup circuit and components
    auto circuit = std::make_shared<Circuit>();
    // read the elink to chip ratio and configure data player accordingly
    std::vector<float> elink_chip_ratio = config.GetNELinkVector(chip_basename_list); // n-elinks/n-chips for each chip
    if (DEBUG) std::cout<<"Creating player object"<<std::endl;
    auto player  = std::make_shared<ChipDataPlayer>(vec_event_chip_sizes, nchips, elink_chip_ratio, RANDOM_L1, TRIGGER_RULE);
    if (DEBUG) std::cout<<"Created player object"<<std::endl;
    circuit->add_component(player);
    std::vector<std::shared_ptr<DTCEventBuilder>> evt_builders;
    for (int ieb=0; ieb<OUTPUT_LINKS; ieb++) {
        evt_builders.push_back(std::make_shared<DTCEventBuilder>(nchips_per_eb[ieb], 1)); 
        circuit->add_component(evt_builders[ieb]);
    }

    std::vector<std::shared_ptr<FIFO64>>              fifos_input;
    std::vector<std::shared_ptr<FIFO64>>              fifos_output_data;
    std::vector<std::shared_ptr<FIFO16>>              fifos_output_control;
    std::vector<std::shared_ptr<EventBoundaryFinder>> ebfs;
    for (int ichip=0; ichip<nchips; ichip++){
        int ieb = eb_assignment[ichip];
        int ichip_per_eb = ichip_to_ichip_per_eb[ichip];
        fifos_input.push_back(std::make_shared<FIFO64>());
        fifos_output_data.push_back(std::make_shared<FIFO64>());
        fifos_output_control.push_back(std::make_shared<FIFO16>());
        ebfs.push_back(std::make_shared<EventBoundaryFinder>());
        circuit->add_component(fifos_input[ichip]);
        circuit->add_component(fifos_output_data[ichip]);
        circuit->add_component(fifos_output_control[ichip]);
        circuit->add_component(ebfs[ichip] );
        player->out_data[ichip].connect( &(fifos_input[ichip]->in_data) );
        player->out_read[ichip].connect( &(fifos_input[ichip]->in_push_enable) );
        //Input FIFO <-> Boundary finder
        fifos_input[ichip]->out_data.connect( &(ebfs[ichip]->in_fifo_i1_data) );
        fifos_input[ichip]->out_data_valid.connect( &(ebfs[ichip]->in_fifo_i1_data_valid) );
        ebfs[ichip]->out_fifo_i1_pop.connect( &(fifos_input[ichip]->in_pop_enable) );
        //Boundary finder <-> output FIFO
        ebfs[ichip]->out_fifo_o1_data.connect( &(fifos_output_data[ichip]->in_data) );
        ebfs[ichip]->out_fifo_o1_read.connect( &(fifos_output_data[ichip]->in_push_enable) );
        ebfs[ichip]->out_fifo_o2_data.connect( &(fifos_output_control[ichip]->in_data) );
        ebfs[ichip]->out_fifo_o2_read.connect( &(fifos_output_control[ichip]->in_push_enable) );
        // Output FIFO <-> Event Builder
        fifos_output_data[ichip]->out_data.connect( &(evt_builders[ieb]->in_data[ichip_per_eb]) );
        fifos_output_data[ichip]->out_data_valid.connect( &(evt_builders[ieb]->in_data_valid[ichip_per_eb]) );
        fifos_output_control[ichip]->out_data.connect( &(evt_builders[ieb]->in_control[ichip_per_eb]) );
        fifos_output_control[ichip]->out_data_valid.connect( &(evt_builders[ieb]->in_control_valid[ichip_per_eb]) );
        evt_builders[ieb]->out_read_data[ichip_per_eb].connect( &(fifos_output_data[ichip]->in_pop_enable) );
        evt_builders[ieb]->out_read_control[ichip_per_eb].connect( &(fifos_output_control[ichip]->in_pop_enable) );
    }
    
    if (DRY_RUN) {
        return 0;
    }

    int inactive_time = 0;
    unsigned long long i_tick = 0;
    std::vector<int> i_event_per_eb(evt_builders.size(),0);
    int i_event = 0; //technically going to be the min value in i_event_per_eb
    std::vector<std::ofstream> ofstreamvector_output_fifo_data;
    for (int ichip=0; ichip<nchips; ichip++) {
        string chip_basename = chip_basename_list[ichip];
        string ichip_output_fname = output_dir+"/output_fifo_data_"+chip_basename+".bin";
        ofstreamvector_output_fifo_data.emplace_back(std::ofstream{ichip_output_fname, std::ios::binary});
        if (!ofstreamvector_output_fifo_data[ichip]) {std::cerr<<"Unable to write to "<<ichip_output_fname<<std::endl; return 4;}
    }
    std::cout<<"auto-ticking..."<<std::endl;
    while (true)
    {
        //////////////////// DEBUG BLOCK ///////////////////////////////
        if (DEBUG) {                                                ////
            std::cout<<"itick="<<i_tick<<std::endl;
            // availability of Data player?
            std::cout<<"player->out_read:"<<std::endl;
            for (int ichip=0; ichip<nchips; ichip++) {
                if (ichip>0) std::cout<<",";
                std::cout<<to_string(player->out_read[ichip].get_value());
            }
            std::cout<<std::endl;
            // availability of Input FIFOs
            std::cout<<"fifos_input->out_data_valid:"<<std::endl;
            for (int ichip=0; ichip<nchips; ichip++) {
                if (ichip>0) std::cout<<",";
                std::cout<<to_string(fifos_input[ichip]->out_data_valid.get_value());
            }
            std::cout<<std::endl;
            // availability of Event Boudary Finder
            std::cout<<"fifos_input->out_data_valid:"<<std::endl;
            for (int ichip=0; ichip<nchips; ichip++) {
                if (ichip>0) std::cout<<",";
                std::cout<<to_string(ebfs[ichip]->out_fifo_o1_read.get_value());
            }
            std::cout<<std::endl;
            std::cout<<"fifos_input->out_control_valid:"<<std::endl;
            for (int ichip=0; ichip<nchips; ichip++) {
                if (ichip>0) std::cout<<",";
                std::cout<<to_string(ebfs[ichip]->out_fifo_o2_read.get_value());
            }
            std::cout<<std::endl;
            // availability of Output Control FIFO
            std::cout<<"fifos_output_control->out_data_valid:"<<std::endl;
            for (int ichip=0; ichip<nchips; ichip++) {
                if (ichip>0) std::cout<<",";
                std::cout<<to_string(fifos_output_control[ichip]->out_data_valid.get_value());
            }
            std::cout<<std::endl;
            // availability of Output Data FIFO
            std::cout<<"fifos_output_data->out_data_valid:"<<std::endl;
            for (int ichip=0; ichip<nchips; ichip++) {
                if (ichip>0) std::cout<<",";
                std::cout<<to_string(fifos_output_data[ichip]->out_data_valid.get_value());
            }
            std::cout<<std::endl;
            // occupancy of Output Data FIFO
            std::cout<<"fifos_output_data->get_buffer_size:"<<std::endl;
            for (int ichip=0; ichip<nchips; ichip++) {
                if (ichip>0) std::cout<<",";
                std::cout<<to_string(fifos_output_data[ichip]->d_get_buffer_size());
            }
            std::cout<<std::endl;
            char key='x';
            while ((key!='n') && (key!='q')) {
                std::cout<<"press \"n\" to continue next tick, \"q\" to skip debugging..."<<std::endl;
                std::cin>>key;
            }
            if (key == 'q') DEBUG=false;
        }                                                           ////
        //////////////////// DEBUG BLOCK ///////////////////////////////


        i_tick++;
        //std::cout<<"tick="<<i_tick<<std::endl;
        circuit->tick();
        for (int ichip=0; ichip<nchips; ichip++) {
            int value = fifos_output_data[ichip]->d_get_buffer_size();
            ofstreamvector_output_fifo_data[ichip].write(reinterpret_cast<const char*>(&value), sizeof(value) );
        };
        for (int ieb=0; ieb<evt_builders.size(); ieb++) if (evt_builders[ieb]->out_event_ready.get_value()) {
            i_event_per_eb[ieb]++;
        }
        int min_i_event_among_eb = *std::min_element(i_event_per_eb.begin(), i_event_per_eb.end());
        //if (i_tick%5000==0){
        //    cerr<<min_i_event_among_eb<<endl;
        //    for (auto ievt : i_event_per_eb) cerr<<ievt;
        //    cerr<<endl;
        //}
        if (min_i_event_among_eb > i_event) {
            i_event = min_i_event_among_eb;
            // progress bar
            int barWidth = 70;
            std::cout << "[";
            int pos = barWidth * i_event/nevents;
            for (int i = 0; i < barWidth; ++i) {
                if (i < pos) std::cout << "=";
                else if (i == pos) std::cout << ">";
                else std::cout << " ";
            }
            std::cout << "] " << i_event <<"/"<< nevents << " %\r";
            std::cout.flush();
            if (i_event>=nevents) break;
        }
    }
    cout<<std::endl<<"total ticks="<<i_tick<<endl;
}
