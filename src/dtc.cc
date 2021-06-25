#include <include/FIFO.h>
#include <interface/Circuit.h>
#include <include/Component.h>
#include <include/Ports.h>
#include <ctime>
#include <deque>
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

using namespace std;
using namespace boost::filesystem;

typedef FIFO<uint64_t> FIFO64;
typedef FIFO<uint16_t> FIFO16;

bool DEBUG=false;

int main(int argc, char* argv[]) {

    // Default parameters
    bool RANDOM_L1=true;
    bool TRIGGER_RULE=true;
    enum RATE_TYPE { NODELAY=0, HALF=1, FULL=2 };
    int OUTPUT_RATE=HALF;
    std::string input_dirname("input_dtc11_10kevt");
    int nevents=1000;

    // argument parsing
    std::string help_msg("Usage: ./build/dtc [options]\n --help:   display this message.\n --input/-i INPUT_DIRNAME:   Change the input directory name, by default uses input_10k.\n --random-l1 L1-TYPE:   L1-TYPE is boolean, set whether L1 trigger rate random with average of 750kHZ or just constantly 750kHz.\n --no-trigger-rule:   Only effective for the random L1 trigger mode, disables the trigger rules.\n --output-rate RATE-TYPE:   set the output rate for the event builder. RATE-TYPE values can be in {\"nodelay\", \"half\", \"full\"}.");
    for (int iarg =0; iarg<argc; iarg++) {
        if (iarg==0) continue;
        if (std::string(argv[iarg])=="--help") {std::cerr<<help_msg<<std::endl; return 0;}
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
        if (std::string(argv[iarg])=="--output-rate") {
            if (iarg+1 < argc) {
                std::string output_rate_type(argv[++iarg]);
                if (output_rate_type=="nodelay")   {OUTPUT_RATE=NODELAY; }
                else if (output_rate_type=="half") {OUTPUT_RATE=HALF;    }
                else if (output_rate_type=="full") {OUTPUT_RATE=FULL;    }
                else {std::cerr<<"Unknow argument "<<output_rate_type<<" following option --output-rate."<<std::endl; return 1;}
            }
            else {
                std::cerr<<"--output-rate option requires one argument."<<std::endl;
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
    std::string output_dir("output_");
    std::size_t pos = 0; 
    if (input_dirname.find("input_") != std::string::npos) {
        pos = input_dirname.find("input_")+6; 
    }
    std::string input_tag = input_dirname.substr(pos);
    while(input_tag.back()=='/') input_tag.pop_back();
    output_dir+=input_tag;
    if (RANDOM_L1) output_dir+="_randomL1"; else output_dir+="_constL1";
    if (RANDOM_L1 && !TRIGGER_RULE)  output_dir+="NoTriggerRule";
    std::cout<<"Running Mode: Randome L1="<<RANDOM_L1<<" TRIGGER_RULE="<<TRIGGER_RULE<<" OUTPUT_RATE=";
    if (OUTPUT_RATE==NODELAY) {std::cout<<"nodelay"; output_dir+="_outputNoDelay"; }
    if (OUTPUT_RATE==HALF)    {std::cout<<"half";    output_dir+="_outputHalfRate";}
    if (OUTPUT_RATE==FULL)    {std::cout<<"full";    output_dir+="_outputFullRate";}
    output_dir+="_N";
    output_dir+=to_string(nevents);
    std::cout<<" Output dir="<<output_dir<<std::endl;
    create_directories(output_dir);

    // get a list of input files related to dtc
    std::vector<std::string> dtc_binary_fn_list;
    path input_dir(input_dirname);
    for (auto iter_fn=directory_iterator(input_dir); iter_fn != directory_iterator(); iter_fn++) {
        if ( is_directory(iter_fn->path()) ) continue;
        string filename = iter_fn->path().string();
        if ( filename.find("dtc") == std::string::npos ) continue;
        dtc_binary_fn_list.push_back(filename);
    }
    // write the order of input filenames into a log file, so that we know which ichip number maps to which physical chip
    std::ofstream log_fn_list(output_dir+"/ordered_file_names.txt");
    for (auto filename : dtc_binary_fn_list) log_fn_list<<filename<<std::endl;
    log_fn_list.close();

    // Define the circuit
    int nchips = dtc_binary_fn_list.size();
    std::cout<< "Number of chips mapped to DTC = " << nchips <<endl;
    std::vector<float> elink_chip_ratio(nchips, 3); // All chips connected to DTC has 3 e-links
    auto circuit = std::make_shared<Circuit>();
    auto player  = std::make_shared<ChipDataPlayer>(dtc_binary_fn_list, nchips, elink_chip_ratio, RANDOM_L1, TRIGGER_RULE); // there are 60 modules in dtc
    auto evt_builder  = std::make_shared<DTCEventBuilder>(nchips, OUTPUT_RATE); 
    circuit->add_component(player);
    circuit->add_component(evt_builder);

    std::vector<std::shared_ptr<FIFO64>>              fifos_input;
    std::vector<std::shared_ptr<FIFO64>>              fifos_output_data;
    std::vector<std::shared_ptr<FIFO16>>               fifos_output_control;
    std::vector<std::shared_ptr<EventBoundaryFinder>>  ebfs;
    for (int ichip=0; ichip<nchips; ichip++){
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
        fifos_output_data[ichip]->out_data.connect( &(evt_builder->in_data[ichip]) );
        fifos_output_data[ichip]->out_data_valid.connect( &(evt_builder->in_data_valid[ichip]) );
        fifos_output_control[ichip]->out_data.connect( &(evt_builder->in_control[ichip]) );
        fifos_output_control[ichip]->out_data_valid.connect( &(evt_builder->in_control_valid[ichip]) );
        evt_builder->out_read_data[ichip].connect( &(fifos_output_data[ichip]->in_pop_enable) );
        evt_builder->out_read_control[ichip].connect( &(fifos_output_control[ichip]->in_pop_enable) );
    }
    int inactive_time = 0;
    unsigned long long i_tick = 0;
    int i_event = 0;
    std::ofstream outputsize_input_fifo(output_dir+"/input_fifo_sizes.txt");
    if (!outputsize_input_fifo) {std::cerr<<"Unable to write to "<<output_dir+"/input_fifo_sizes.txt"<<std::endl; return 4;}
    std::ofstream outputsize_output_fifo_data(output_dir+"/output_fifo_data_sizes.txt");
    if (!outputsize_output_fifo_data) {std::cerr<<"Unable to write to "<<output_dir+"/output_fifo_data_sizes.txt"<<std::endl; return 4;}
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
            if (ichip>0) {outputsize_input_fifo<<","; outputsize_output_fifo_data<<",";}
            outputsize_input_fifo<<to_string(fifos_input[ichip]->d_get_buffer_size());
            outputsize_output_fifo_data<<to_string(fifos_output_data[ichip]->d_get_buffer_size());
        };
        outputsize_input_fifo<<endl;
        outputsize_output_fifo_data<<endl;
        if (evt_builder->out_event_ready.get_value()) {
            i_event++;
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
