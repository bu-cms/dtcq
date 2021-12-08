#include <iostream>
#include <fstream>
#include <assert.h>
#include <vector>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <string>
#include <map>
#include <utility>
#include <tuple>
#include <random>

using namespace std;

class ChipConfigReader
{
    public:
        ChipConfigReader(string config_file_name);
        float GetNELink(string chip_basename);
        vector<float> GetNELinkVector(vector<string> chip_basename_vector);
        float GetAvgSize(string chip_basename);
        vector<float> GetAvgSizeVector(vector<string> chip_basename_vector);
        vector<int> assign_chips_to_event_builders(vector<string> chip_basename_list, int n_event_builders, string mode);
        vector<int> assign_chips_as_original(vector<string> chip_basename_list, int n_event_builders);
        vector<int> assign_chips_as_random(vector<string> chip_basename_list, int n_event_builders);
        vector<int> assign_chips_as_sorted(vector<string> chip_basename_list, int n_event_builders);
        static string filename_to_basename(string chip_filename);
        map<string, tuple<float,int,float>> basename_to_params; //nelink, nevent, avgsize
        vector<string> ordered_basenames; //preserve the ordering of chip basenames in the config file because map doesn't preserve the order
    private:
};
