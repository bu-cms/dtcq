#include <interface/ChipConfigReader.h>

using namespace std;

ChipConfigReader::ChipConfigReader(string config_file_name){
    ifstream config(config_file_name);
    string basename;
    float nelink;
    int nevent;
    float avg_size;
    if (!config) {
        cerr<<"Cannot read config file: "<<config_file_name<<endl;
    }
    while (config>>basename>>nelink>>nevent>>avg_size) {
        basename_to_params[basename] = tuple<float,int,float>(nelink,nevent,avg_size);
        ordered_basenames.push_back(basename);
    }
    config.close();
}

string ChipConfigReader::filename_to_basename(string chip_filename) {
    size_t pos_beg = chip_filename.find_last_of("/");
    if (pos_beg == string::npos) pos_beg = 0;
    else pos_beg++;
    size_t pos_end = chip_filename.find_last_of(".");
    if (pos_end == string::npos) pos_end = chip_filename.size();
    string chip_basename = chip_filename.substr(pos_beg, pos_end - pos_beg);
    return chip_basename;
}

float ChipConfigReader::GetNELink(string chip_filename) {
    string chip_basename = filename_to_basename(chip_filename);
    auto found = basename_to_params.find(chip_basename);
    if (found == basename_to_params.end()) return 3.0; //highest possible bandwidth by default
    float ret = get<0>(found->second);
    return ret;
}

vector<float> ChipConfigReader::GetNELinkVector(vector<string> chip_filename_vector) {
    vector<float> ret(chip_filename_vector.size());
    transform(chip_filename_vector.begin(), chip_filename_vector.end(), ret.begin(), [this](string x){return this->GetNELink(x);});
    return ret;
}

float ChipConfigReader::GetAvgSize(string chip_filename) {
    string chip_basename = filename_to_basename(chip_filename);
    auto found = basename_to_params.find(chip_basename);
    if (found == basename_to_params.end()) return 30; //highest possible bandwidth by default
    float ret = get<2>(found->second);
    return ret;
}

vector<float> ChipConfigReader::GetAvgSizeVector(vector<string> chip_filename_vector) {
    vector<float> ret(chip_filename_vector.size());
    transform(chip_filename_vector.begin(), chip_filename_vector.end(), ret.begin(), [this](string x){return this->GetAvgSize(x);});
    return ret;
}

vector<int> ChipConfigReader::assign_chips_as_original(vector<string> chip_basename_list, int n_event_builders) {
    assert(n_event_builders>0);
    vector<float> chip_avg_size = this->GetAvgSizeVector(chip_basename_list);
    vector<int> assignment(chip_basename_list.size(),-1);
    float sum_of_size = accumulate(chip_avg_size.begin(), chip_avg_size.end(), float(0));
    float size_threshold_per_eb = sum_of_size / n_event_builders;
    // assign chips in the order of basename mapping
    int eb_iter = 0;
    float current_size_allocated = 0;
    std::cout<<"Assigning chips according to config file ordering... Sum of event size="<<sum_of_size<<" threshold="<<size_threshold_per_eb<<std::endl;
    std::cout<<"iEB\t|\tchip size\t|\tcumulated size\t|\tchip name"<<std::endl;
    for (string basename : ordered_basenames) {
        for (int ichip=0; ichip<chip_basename_list.size(); ichip++) {
            if (assignment[ichip]>=0) continue;
            if (chip_basename_list[ichip] == basename) {
                assert(eb_iter<n_event_builders);
                assignment[ichip] = eb_iter;
                current_size_allocated += chip_avg_size[ichip];
                std::cout<<eb_iter<<"\t|\t"<<chip_avg_size[ichip]<<"\t\t|\t"<<current_size_allocated<<"\t\t|\t"<<basename<<std::endl;
                if (current_size_allocated > (eb_iter+1) * size_threshold_per_eb) {
                    eb_iter++;
                }
                break;
            }
        }
    }
    // Check for remaining chips not assigned
    for (int ichip=0; ichip<chip_basename_list.size(); ichip++) {
        if (assignment[ichip] < 0) {
            cerr<<"Warning: Chip "<<chip_basename_list[ichip]<<" not in config file, assignning to the last event builder."<<endl;
            assignment[ichip] = n_event_builders - 1;
        }
    }
    return assignment;
}

vector<int> ChipConfigReader::assign_chips_as_random(vector<string> chip_basename_list, int n_event_builders) {
    assert(n_event_builders>0);
    vector<float> chip_avg_size = this->GetAvgSizeVector(chip_basename_list);
    float sum_of_size = accumulate(chip_avg_size.begin(), chip_avg_size.end(), float(0));
    float size_threshold_per_eb = sum_of_size / n_event_builders;
    vector<int> shuffled_chips(chip_basename_list.size(), 0);
    for (int i=0; i<shuffled_chips.size(); i++) shuffled_chips[i]=i;
    std::shuffle(shuffled_chips.begin(), shuffled_chips.end(), std::default_random_engine(233));
    vector<int> assignment(chip_basename_list.size(),-1);
    vector<float> allocated_sizes(n_event_builders, 0);
    std::cout<<"Assigning chips randomly... Sum of event size="<<sum_of_size<<" threshold="<<size_threshold_per_eb<<std::endl;
    std::cout<<"iEB\t|\tcumulated size"<<std::endl;
    for (int ichip : shuffled_chips) {
        int ieb_minsize = min_element(allocated_sizes.begin(), allocated_sizes.end()) - allocated_sizes.begin();
        assignment[ichip] = ieb_minsize;
        allocated_sizes[ieb_minsize] += chip_avg_size[ichip];
    }
    for (int ieb=0; ieb<n_event_builders; ieb++) {
        std::cout<<ieb<<"\t|\t"<<allocated_sizes[ieb]<<std::endl;
    }
    return assignment;
}

vector<int> ChipConfigReader::assign_chips_as_sorted(vector<string> chip_basename_list, int n_event_builders) {
    assert(n_event_builders>0);
    vector<float> chip_avg_size = this->GetAvgSizeVector(chip_basename_list);
    float sum_of_size = accumulate(chip_avg_size.begin(), chip_avg_size.end(), float(0));
    float size_threshold_per_eb = sum_of_size / n_event_builders;
    vector<int> sorted_chips(chip_basename_list.size());
    std::iota(sorted_chips.begin(), sorted_chips.end(), 0);
    std::stable_sort(sorted_chips.begin(), sorted_chips.end(), [&chip_avg_size](int i1, int i2) {return chip_avg_size[i1]<chip_avg_size[i2];});
    vector<int> assignment(chip_basename_list.size(),-1);
    std::cout<<"Assigning chips as rate-sorted... Sum of event size="<<sum_of_size<<" threshold="<<size_threshold_per_eb<<std::endl;
    std::cout<<"iEB\t|\tcumulated size"<<std::endl;
    int eb_iter = 0;
    float current_size_allocated = 0;
    for (int ichip : sorted_chips) {
        assignment[ichip] = eb_iter;
        current_size_allocated += chip_avg_size[ichip];
        if (current_size_allocated > (eb_iter+1) * size_threshold_per_eb) {
            std::cout<<eb_iter<<"\t|\t"<<current_size_allocated<<std::endl;
            eb_iter++;
        }
    }
    return assignment;
}

vector<int> ChipConfigReader::assign_chips_to_event_builders(vector<string> chip_basename_list, int n_event_builders, std::string mode) {
    // assign chips according to their original order in the config file
    if (mode=="original"){
        return this->assign_chips_as_original(chip_basename_list, n_event_builders);
    }
    // assign chips randomly but also try to balance load
    else if (mode=="random"){
        return this->assign_chips_as_random(chip_basename_list, n_event_builders);
    }
    else if (mode=="sorted"){
        return this->assign_chips_as_sorted(chip_basename_list, n_event_builders);
    }
    else {
        string msg="assignment mode ";
        msg += mode;
        msg += " unimplemented!";
        throw std::runtime_error(msg);
    }
}
