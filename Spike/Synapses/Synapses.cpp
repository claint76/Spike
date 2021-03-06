//  Synapse Class C++
//  Synapse.cpp
//
//  Author: Nasir Ahmad
//  Date: 7/12/2015

#include "Synapses.hpp"
#include "../Helpers/TerminalHelpers.hpp"

#include <algorithm> // for random shuffle
#include <vector> // for random shuffle
#include <random>

// Synapses Constructor
Synapses::Synapses() : Synapses(42) {
}

// Synapses Constructor
Synapses::Synapses(int seedval) {
  srand(seedval); // Seeding the random numbers
  random_state_manager = new RandomStateManager();
}

void Synapses::prepare_backend_early() {
  random_state_manager->init_backend(backend()->context);
}

void Synapses::sort_synapses(Neurons* input_neurons, Neurons* neurons){
  if (!synapses_sorted){
    std::vector<std::vector<int>> per_neuron_synapses;
    int total_possible_pre_neurons = input_neurons->total_number_of_neurons + neurons->total_number_of_neurons;
    for (int n=0; n < total_possible_pre_neurons; n++){
      std::vector<int> empty;
      per_neuron_synapses.push_back(empty);
    }

    for (int s=0; s < total_number_of_synapses; s++){
      per_neuron_synapses[input_neurons->total_number_of_neurons + presynaptic_neuron_indices[s]].push_back(s);
    }

    // Sorting
    int count = 0;
    for (int n=0; n < total_possible_pre_neurons; n++){
      for (int s=0; s < per_neuron_synapses[n].size(); s++){
        synapse_sort_indices[count] = per_neuron_synapses[n][s];
        count++;
      }
    }
    
    int* temp_presyn_array = (int*)malloc(total_number_of_synapses * sizeof(int));
    int* temp_postsyn_array = (int*)malloc(total_number_of_synapses * sizeof(int));
    float* temp_weight_array = (float*)malloc(total_number_of_synapses * sizeof(float));
    float* temp_scaling_array = (float*)malloc(total_number_of_synapses * sizeof(float));
    // Re-ordering arrays
    for (int s=0; s < total_number_of_synapses; s++){
      temp_presyn_array[s] = presynaptic_neuron_indices[synapse_sort_indices[s]];
      temp_postsyn_array[s] = postsynaptic_neuron_indices[synapse_sort_indices[s]];
      temp_weight_array[s] = synaptic_efficacies_or_weights[synapse_sort_indices[s]];
      temp_scaling_array[s] = weight_scaling_constants[synapse_sort_indices[s]];

      synapse_reversesort_indices[synapse_sort_indices[s]] = s;
    }

    free(presynaptic_neuron_indices);
    free(postsynaptic_neuron_indices);
    free(synaptic_efficacies_or_weights);
    free(weight_scaling_constants);

    presynaptic_neuron_indices = temp_presyn_array;
    postsynaptic_neuron_indices = temp_postsyn_array;
    synaptic_efficacies_or_weights = temp_weight_array;
    weight_scaling_constants = temp_scaling_array;

    synapses_sorted = true;
  }
}

// Synapses Destructor
Synapses::~Synapses() {
  free(presynaptic_neuron_indices);
  free(postsynaptic_neuron_indices);
  free(synaptic_efficacies_or_weights);
  free(synapse_postsynaptic_neuron_count_index);
  free(synapse_sort_indices);
  free(synapse_reversesort_indices);
  free(weight_scaling_constants);

  delete random_state_manager;
}


void Synapses::reset_state() {
  backend()->reset_state();
}


int Synapses::AddGroup(int presynaptic_group_id, 
                        int postsynaptic_group_id, 
                        Neurons * neurons,
                        Neurons * input_neurons,
                        float timestep,
                        synapse_parameters_struct * synapse_params) {

  if (print_synapse_group_details == true) {
          printf("Adding synapse group...\n");
          printf("presynaptic_group_id: %d\n", presynaptic_group_id);
          printf("postsynaptic_group_id: %d\n", postsynaptic_group_id);
  }

  int* start_neuron_indices_for_neuron_groups = neurons->start_neuron_indices_for_each_group;
  int* last_neuron_indices_for_neuron_groups = neurons->last_neuron_indices_for_each_group;

  int * presynaptic_group_shape;
  int * postsynaptic_group_shape;

  int prestart = 0;
  int preend = 0;
  int poststart = 0;

  // Calculate presynaptic group start and end indices
  // Also assign presynaptic group shape
  bool presynaptic_group_is_input = PRESYNAPTIC_IS_INPUT(presynaptic_group_id);

  if (presynaptic_group_is_input) {
    int* start_neuron_indices_for_input_neuron_groups = input_neurons->start_neuron_indices_for_each_group;
    int* last_neuron_indices_for_input_neuron_groups = input_neurons->last_neuron_indices_for_each_group;

    int corrected_presynaptic_group_id = CORRECTED_PRESYNAPTIC_ID(presynaptic_group_id, presynaptic_group_is_input);

    presynaptic_group_shape = input_neurons->group_shapes[corrected_presynaptic_group_id];

    if (presynaptic_group_id < -1){
      prestart = start_neuron_indices_for_input_neuron_groups[corrected_presynaptic_group_id];
    }
    preend = last_neuron_indices_for_input_neuron_groups[corrected_presynaptic_group_id] + 1;
  } else {
    presynaptic_group_shape = neurons->group_shapes[presynaptic_group_id];

    if (presynaptic_group_id > 0){
      prestart = start_neuron_indices_for_neuron_groups[presynaptic_group_id];
    }
    preend = last_neuron_indices_for_neuron_groups[presynaptic_group_id] + 1;

  }

  // Calculate postsynaptic group start and end indices
  // Also assign postsynaptic group shape
  if (postsynaptic_group_id < 0) { // If presynaptic group is Input group EXIT
          print_message_and_exit("Input groups cannot be a postsynaptic neuron group.");
  } else if (postsynaptic_group_id >= 0){
          postsynaptic_group_shape = neurons->group_shapes[postsynaptic_group_id];
          poststart = start_neuron_indices_for_neuron_groups[postsynaptic_group_id];
  }
  
  int postend = last_neuron_indices_for_neuron_groups[postsynaptic_group_id] + 1;

  if (print_synapse_group_details == true) {
          const char * presynaptic_group_type_string = (presynaptic_group_id < 0) ? "input_neurons" : "neurons";
          printf("Presynaptic neurons start index: %d (%s)\n", prestart, presynaptic_group_type_string);
          printf("Presynaptic neurons end index: %d (%s)\n", preend, presynaptic_group_type_string);
          printf("Postsynaptic neurons start index: %d (neurons)\n", poststart);
          printf("Postsynaptic neurons end index: %d (neurons)\n", postend);
  }


  int original_number_of_synapses = total_number_of_synapses;

  // Carry out the creation of the connectivity matrix
  switch (synapse_params->connectivity_type){
            
        case CONNECTIVITY_TYPE_ALL_TO_ALL:
          {
            
            int increment = (preend-prestart)*(postend-poststart);
            Synapses::increment_number_of_synapses(increment);

            // If the connectivity is all_to_all
            for (int i = prestart; i < preend; i++){
              for (int j = poststart; j < postend; j++){
                // Index
                int idx = original_number_of_synapses + (i-prestart)*(postend-poststart) + (j-poststart);
                // Setup Synapses
                presynaptic_neuron_indices[idx] = CORRECTED_PRESYNAPTIC_ID(i, presynaptic_group_is_input);
                postsynaptic_neuron_indices[idx] = j;
              }
            }
            break;
          }
        case CONNECTIVITY_TYPE_ONE_TO_ONE:
          {
            int increment = (preend-prestart);
            Synapses::increment_number_of_synapses(increment);
            
            // If the connectivity is one_to_one
            if ((preend-prestart) != (postend-poststart)) print_message_and_exit("Unequal populations for one_to_one.");
            // Create the connectivity
            for (int i = 0; i < (preend-prestart); i++){
              presynaptic_neuron_indices[original_number_of_synapses + i] = CORRECTED_PRESYNAPTIC_ID(prestart + i, presynaptic_group_is_input);
              postsynaptic_neuron_indices[original_number_of_synapses + i] = poststart + i;
            }

            break;
          }
        case CONNECTIVITY_TYPE_RANDOM:
          {
            // If the connectivity is random
            // Begin a count
            for (int i = prestart; i < preend; i++){
              for (int j = poststart; j < postend; j++){
                // Probability of connection
                float prob = ((float)rand() / (RAND_MAX));
                // If it is within the probability range, connect!
                if (prob < synapse_params->random_connectivity_probability){
            
                  Synapses::increment_number_of_synapses(1);

                  // Setup Synapses
                  presynaptic_neuron_indices[total_number_of_synapses - 1] = CORRECTED_PRESYNAPTIC_ID(i, presynaptic_group_is_input);
                  postsynaptic_neuron_indices[total_number_of_synapses - 1] = j;
                }
              }
            }
            break;
          }
    
        case CONNECTIVITY_TYPE_GAUSSIAN_SAMPLE:
          {

            float standard_deviation_sigma = synapse_params->gaussian_synapses_standard_deviation;
            int number_of_new_synapses_per_postsynaptic_neuron = synapse_params->gaussian_synapses_per_postsynaptic_neuron;
            int number_of_presynaptic_neurons = presynaptic_group_shape[0] * presynaptic_group_shape[1];
            if (number_of_new_synapses_per_postsynaptic_neuron > number_of_presynaptic_neurons){
              print_message_and_exit("Synapse creation error. Pre-synaptic population smaller than requested synapses per post-synaptic neuron (Gaussian Sampling).");
            }
      
            int number_of_postsynaptic_neurons_in_group = postend - poststart;
            int total_number_of_new_synapses = number_of_new_synapses_per_postsynaptic_neuron * number_of_postsynaptic_neurons_in_group;
            Synapses::increment_number_of_synapses(total_number_of_new_synapses);

            for (int postid = 0; postid < number_of_postsynaptic_neurons_in_group; postid++){
              float post_fractional_centre_x = ((float)(postid % postsynaptic_group_shape[0]) / (float)postsynaptic_group_shape[0]);
              float post_fractional_centre_y = ((float)((float)postid / (float)postsynaptic_group_shape[1]) / (float)postsynaptic_group_shape[1]);
              int pre_centre_x = presynaptic_group_shape[0] * post_fractional_centre_x;
              int pre_centre_y = presynaptic_group_shape[1] * post_fractional_centre_y;

              // Constructing the probability with which we should connect to each pre-synaptic neuron
              std::vector<float> pre_neuron_probabilities;
              std::vector<float> pre_neuron_indices;
              float total_probability = 0.0;
              for (int preid = 0; preid < number_of_presynaptic_neurons; preid++){
                int pre_xcoord = preid % presynaptic_group_shape[0]; 
                int pre_ycoord = preid / presynaptic_group_shape[0];
                float probability_of_connection = expf(- (powf((pre_xcoord - pre_centre_x), 2.0)) / (2.0 * powf(standard_deviation_sigma, 2)));
                probability_of_connection *= expf(- (powf((pre_ycoord - pre_centre_y), 2.0)) / (2.0 * powf(standard_deviation_sigma, 2)));
                pre_neuron_probabilities.push_back(probability_of_connection);
                pre_neuron_indices.push_back(preid);
                total_probability += probability_of_connection;
              }
              
              for (int i=0; i < number_of_new_synapses_per_postsynaptic_neuron; i++){
                postsynaptic_neuron_indices[original_number_of_synapses + postid*number_of_new_synapses_per_postsynaptic_neuron + i] = poststart + postid;
                float randval = total_probability*((float)rand() / (RAND_MAX));
                float probability_trace = 0.0;
                for (int preloc = 0; preloc < pre_neuron_probabilities.size(); preloc++){
                  if ((randval > probability_trace) && (randval < (probability_trace + pre_neuron_probabilities[preloc]))){
                    int chosenpreid = CORRECTED_PRESYNAPTIC_ID(pre_neuron_indices[preloc] + prestart, presynaptic_group_is_input);
                    presynaptic_neuron_indices[original_number_of_synapses + postid*number_of_new_synapses_per_postsynaptic_neuron + i] = chosenpreid;
                    total_probability -= pre_neuron_probabilities[preloc];
                    pre_neuron_indices.erase(pre_neuron_indices.begin() + preloc);
                    pre_neuron_probabilities.erase(pre_neuron_probabilities.begin() + preloc);
                    break;
                  } else {
                    probability_trace += pre_neuron_probabilities[preloc];
                  }
                }
              }
            }

            if (total_number_of_new_synapses > largest_synapse_group_size) {
              largest_synapse_group_size = total_number_of_new_synapses;
            }

            break;
          }
        case CONNECTIVITY_TYPE_PAIRWISE:
          {
      // Check that the number of pre and post syns are equivalent
      if (synapse_params->pairwise_connect_presynaptic.size() != synapse_params->pairwise_connect_postsynaptic.size()){
        std::cerr << "Synapse pre and post vectors are not the same length!" << std::endl;
        exit(1);
      }
      // If we desire a single connection
      Synapses::increment_number_of_synapses(synapse_params->pairwise_connect_presynaptic.size());

      // Setup Synapses
      int numpostneurons = postsynaptic_group_shape[0]*postsynaptic_group_shape[1];
      int numpreneurons = presynaptic_group_shape[0]*presynaptic_group_shape[1];
      for (int i=0; i < synapse_params->pairwise_connect_presynaptic.size(); i++){
        if ((synapse_params->pairwise_connect_presynaptic[i] < 0) || (synapse_params->pairwise_connect_postsynaptic[i] < 0)){
          print_message_and_exit("PAIRWISE CONNECTION ERROR: Negative pre/post indices encountered. All indices should be positive (relative to the number of neurons in the pre/post synaptic neuron groups).");
        }
        if ((synapse_params->pairwise_connect_presynaptic[i] >= numpreneurons) || (synapse_params->pairwise_connect_postsynaptic[i] >= numpostneurons)){
          print_message_and_exit("PAIRWISE CONNECTION ERROR: Pre/post indices encountered too large. All indices should be up to the size of the neuron group. Indexing is from zero.");
        }
        presynaptic_neuron_indices[original_number_of_synapses + i] = CORRECTED_PRESYNAPTIC_ID(prestart + int(synapse_params->pairwise_connect_presynaptic[i]), presynaptic_group_is_input);
        postsynaptic_neuron_indices[original_number_of_synapses + i] = poststart + int(synapse_params->pairwise_connect_postsynaptic[i]);
      }

      break;
    }
    default:
    {
      print_message_and_exit("Unknown Connection Type.");
      break;
    }
  }

  temp_number_of_synapses_in_last_group = total_number_of_synapses - original_number_of_synapses;

  if (print_synapse_group_details == true) printf("%d new synapses added.\n\n", temp_number_of_synapses_in_last_group);

  for (int i = original_number_of_synapses; i < total_number_of_synapses; i++){
          
          weight_scaling_constants[i] = synapse_params->weight_scaling_constant;
    
          float weight_range_bottom = synapse_params->weight_range[0];
          float weight_range_top = synapse_params->weight_range[1];

          float weight = weight_range_bottom;
          if (weight_range_top != weight_range_bottom)
      weight = weight_range_bottom + (weight_range_top - weight_range_bottom)*((float)rand() / (RAND_MAX));
          synaptic_efficacies_or_weights[i] = weight;

    if (synapse_params->connectivity_type == CONNECTIVITY_TYPE_PAIRWISE){
      if (synapse_params->pairwise_connect_weight.size() == temp_number_of_synapses_in_last_group){
        synaptic_efficacies_or_weights[i] = synapse_params->pairwise_connect_weight[i - original_number_of_synapses];
      } else if (synapse_params->pairwise_connect_weight.size() != 0) {
        print_message_and_exit("PAIRWISE CONNECTION ISSUE: Weight vector length not as expected. Should be the same length as pre/post vecs.");
      }
    }

          // Used for event count
          // printf("postsynaptic_neuron_indices[i]: %d\n", postsynaptic_neuron_indices[i]);
          synapse_postsynaptic_neuron_count_index[postsynaptic_neuron_indices[i]] = neurons->per_neuron_afferent_synapse_count[postsynaptic_neuron_indices[i]];
          neurons->per_neuron_afferent_synapse_count[postsynaptic_neuron_indices[i]] ++;

    if (neurons->per_neuron_afferent_synapse_count[postsynaptic_neuron_indices[i]] > maximum_number_of_afferent_synapses)
      maximum_number_of_afferent_synapses = neurons->per_neuron_afferent_synapse_count[postsynaptic_neuron_indices[i]];

    int presynaptic_id = CORRECTED_PRESYNAPTIC_ID(presynaptic_neuron_indices[i], presynaptic_group_is_input);
    if (presynaptic_group_is_input)
    input_neurons->AddEfferentSynapse(presynaptic_id, i);
    else
    neurons->AddEfferentSynapse(presynaptic_id, i);

    synapse_sort_indices[i] = i;
    synapse_reversesort_indices[i] = i;
  }

  // SETTING UP PLASTICITY
  int plasticity_id = -1;
  int original_num_plasticity_indices = 0;
  if (synapse_params->plasticity_vec.size() > 0){
    for (int vecid = 0; vecid < synapse_params->plasticity_vec.size(); vecid++){
      Plasticity* plasticity_ptr = synapse_params->plasticity_vec[vecid];
      //Check first for nullptr
      if (plasticity_ptr == nullptr)
        continue;
      plasticity_id = plasticity_ptr->plasticity_rule_id;
      //Store or recall STDP Pointer
      // Check if this pointer has already been stored
      if (plasticity_id < 0){
        plasticity_id = plasticity_rule_vec.size();
        plasticity_rule_vec.push_back(plasticity_ptr);
        // Apply ID to STDP class
        plasticity_ptr->plasticity_rule_id = plasticity_id;
      }

      plasticity_ptr->AddSynapseIndices((total_number_of_synapses - temp_number_of_synapses_in_last_group), temp_number_of_synapses_in_last_group);
        //for (int i = (total_number_of_synapses - temp_number_of_synapses_in_last_group); i < total_number_of_synapses; i++){
          //Set STDP on or off for synapse (now using stdp id)
          //plasticity_ptr->AddSynapse(presynaptic_neuron_indices[i], postsynaptic_neuron_indices[i], i);
        //}
    }
  }



  postpop_start_per_group.push_back(poststart);
  prepop_is_input.push_back(presynaptic_group_is_input);
  prepop_start_per_group.push_back(prestart);
  last_index_of_synapse_per_group.push_back(total_number_of_synapses);

  return(last_index_of_synapse_per_group.size() - 1);

}


void Synapses::increment_number_of_synapses(int increment) {

  total_number_of_synapses += increment;

  if (total_number_of_synapses - increment == 0) {
          presynaptic_neuron_indices = (int*)malloc(total_number_of_synapses * sizeof(int));
          postsynaptic_neuron_indices = (int*)malloc(total_number_of_synapses * sizeof(int));
          synaptic_efficacies_or_weights = (float*)malloc(total_number_of_synapses * sizeof(float));
          weight_scaling_constants = (float*)malloc(total_number_of_synapses * sizeof(float));
          synapse_postsynaptic_neuron_count_index = (int*)malloc(total_number_of_synapses * sizeof(int));
          synapse_sort_indices = (int*)malloc(total_number_of_synapses * sizeof(int));
          synapse_reversesort_indices = (int*)malloc(total_number_of_synapses * sizeof(int));
  } else {
    int* temp_presynaptic_neuron_indices = (int*)realloc(presynaptic_neuron_indices, total_number_of_synapses * sizeof(int));
    int* temp_postsynaptic_neuron_indices = (int*)realloc(postsynaptic_neuron_indices, total_number_of_synapses * sizeof(int));
    float* temp_synaptic_efficacies_or_weights = (float*)realloc(synaptic_efficacies_or_weights, total_number_of_synapses * sizeof(float));
    int* temp_synapse_postsynaptic_neuron_count_index = (int*)realloc(synapse_postsynaptic_neuron_count_index, total_number_of_synapses * sizeof(int));
    float* temp_weight_scaling_constants = (float*)realloc(weight_scaling_constants, total_number_of_synapses * sizeof(float));
    int* temp_sort_indices = (int*)realloc(synapse_sort_indices, total_number_of_synapses * sizeof(int));
    int* temp_revsort_indices = (int*)realloc(synapse_reversesort_indices, total_number_of_synapses * sizeof(int));

    if (temp_presynaptic_neuron_indices != nullptr) presynaptic_neuron_indices = temp_presynaptic_neuron_indices;
    if (temp_postsynaptic_neuron_indices != nullptr) postsynaptic_neuron_indices = temp_postsynaptic_neuron_indices;
    if (temp_synaptic_efficacies_or_weights != nullptr) synaptic_efficacies_or_weights = temp_synaptic_efficacies_or_weights;
    if (temp_synapse_postsynaptic_neuron_count_index != nullptr) synapse_postsynaptic_neuron_count_index = temp_synapse_postsynaptic_neuron_count_index;
    if (temp_weight_scaling_constants != nullptr) weight_scaling_constants = temp_weight_scaling_constants;
    if (temp_sort_indices != nullptr) synapse_sort_indices = temp_sort_indices;
    if (temp_revsort_indices != nullptr) synapse_reversesort_indices = temp_revsort_indices;
  }

}

void Synapses::save_connectivity_as_txt(std::string path, std::string prefix, int synapsegroupid){
  int startid = 0;
  int endid = total_number_of_synapses;
  if (synapsegroupid >= 0)
    endid = last_index_of_synapse_per_group[synapsegroupid];
  if ((synapsegroupid > 0) && (synapsegroupid < last_index_of_synapse_per_group.size())){
    startid = last_index_of_synapse_per_group[synapsegroupid - 1];
  }
  int precorrection = 0;
  int postcorrection = 0;
  bool presynaptic_group_is_input = false;
  if (synapsegroupid >= 0){
    postcorrection = postpop_start_per_group[synapsegroupid];
    precorrection = prepop_start_per_group[synapsegroupid];
    presynaptic_group_is_input = prepop_is_input[synapsegroupid];
  }
  std::ofstream preidfile, postidfile, weightfile;

  // Open output files
  preidfile.open((path + "/" + prefix + "PresynapticIDs.txt"), std::ios::out | std::ios::binary);
  postidfile.open((path + "/" + prefix + "PostsynapticIDs.txt"), std::ios::out | std::ios::binary);
  weightfile.open((path + "/" + prefix + "SynapticWeights.txt"), std::ios::out | std::ios::binary);

  // Ensure weight data has been copied to frontend
  if (_backend)
    backend()->copy_to_frontend();

  // Send data to file
  for (int i = startid; i < endid; i++){
    if (synapsegroupid >= 0)
      preidfile << CORRECTED_PRESYNAPTIC_ID(presynaptic_neuron_indices[synapse_reversesort_indices[i]], presynaptic_group_is_input) - precorrection << std::endl;
    else 
      preidfile << presynaptic_neuron_indices[synapse_reversesort_indices[i]] << std::endl;
    postidfile << postsynaptic_neuron_indices[synapse_reversesort_indices[i]] - postcorrection << std::endl;
    weightfile << synaptic_efficacies_or_weights[synapse_reversesort_indices[i]] << std::endl;
  }

  // Close files
  preidfile.close();
  postidfile.close();
  weightfile.close();

};
// Ensure copied from device, then send
void Synapses::save_connectivity_as_binary(std::string path, std::string prefix, int synapsegroupid){
  int startid = 0;
  int endid = total_number_of_synapses;
  if (synapsegroupid >= 0)
    endid = last_index_of_synapse_per_group[synapsegroupid];
  if ((synapsegroupid > 0) && (synapsegroupid < last_index_of_synapse_per_group.size())){
    startid = last_index_of_synapse_per_group[synapsegroupid - 1];
  }
  int precorrection = 0;
  int postcorrection = 0;
  bool presynaptic_group_is_input = false;
  if (synapsegroupid >= 0){
    postcorrection = postpop_start_per_group[synapsegroupid];
    precorrection = prepop_start_per_group[synapsegroupid];
    presynaptic_group_is_input = prepop_is_input[synapsegroupid];
  }
  std::ofstream preidfile, postidfile, weightfile;

  // Open output files
  preidfile.open((path + "/" + prefix + "PresynapticIDs.bin"), std::ios::out | std::ios::binary);
  postidfile.open((path + "/" + prefix + "PostsynapticIDs.bin"), std::ios::out | std::ios::binary);
  weightfile.open((path + "/" + prefix + "SynapticWeights.bin"), std::ios::out | std::ios::binary);

  // Ensure weight data has been copied to frontend
  if (_backend)
    backend()->copy_to_frontend();

  // Send data to file
  int preid, postid;
  float weight;
  for (int i = startid; i < endid; i++){
    if (synapsegroupid >= 0)
      preid = CORRECTED_PRESYNAPTIC_ID(presynaptic_neuron_indices[synapse_reversesort_indices[i]], presynaptic_group_is_input) - precorrection;
    else
      preid = presynaptic_neuron_indices[synapse_reversesort_indices[i]];
    postid = postsynaptic_neuron_indices[synapse_reversesort_indices[i]] - postcorrection;
    weight = synaptic_efficacies_or_weights[synapse_reversesort_indices[i]];
    preidfile.write((char *)&preid, sizeof(int));
    postidfile.write((char *)&postid, sizeof(int));
    weightfile.write((char *)&weight, sizeof(float));
  }
  /*
  // Send data to file
  preidfile.write((char *)&presynaptic_neuron_indices[startid], (endid - startid)*sizeof(int));
  postidfile.write((char *)&postsynaptic_neuron_indices[startid], (endid - startid)*sizeof(int));
  */
  //weightfile.write((char *)&synaptic_efficacies_or_weights[startid], (endid - startid)*sizeof(float));

  // Close files
  preidfile.close();
  postidfile.close();
  weightfile.close();
}

// Load Network??
//void Synapses::load_connectivity_from_txt(std::string path, std::string prefix);
//void Synapses::load_connectivity_from_binary(std::string path, std::string prefix);

void Synapses::save_weights_as_txt(std::string path, std::string prefix, int synapsegroupid){
  int startid = 0;
  int endid = total_number_of_synapses;
  if (synapsegroupid >= 0)
    endid = last_index_of_synapse_per_group[synapsegroupid];
  if ((synapsegroupid > 0) && (synapsegroupid < last_index_of_synapse_per_group.size())){
    startid = last_index_of_synapse_per_group[synapsegroupid - 1];
  }

  std::ofstream weightfile;
  if (_backend)
    backend()->copy_to_frontend();
  weightfile.open((path + "/" + prefix + "SynapticWeights.txt"), std::ios::out | std::ios::binary);
  for (int i = startid; i < endid; i++){
    weightfile << synaptic_efficacies_or_weights[synapse_reversesort_indices[i]] << std::endl;
  }
  weightfile.close();
}

void Synapses::save_weights_as_binary(std::string path, std::string prefix, int synapsegroupid){
  int startid = 0;
  int endid = total_number_of_synapses;
  if (synapsegroupid >= 0)
    endid = last_index_of_synapse_per_group[synapsegroupid];
  if ((synapsegroupid > 0) && (synapsegroupid < last_index_of_synapse_per_group.size())){
    startid = last_index_of_synapse_per_group[synapsegroupid - 1];
  }

  std::ofstream weightfile;
  if (_backend)
    backend()->copy_to_frontend();
  weightfile.open((path + "/" + prefix + "SynapticWeights.bin"), std::ios::out | std::ios::binary);
  for (int i = startid; i < endid; i++){
    weightfile.write((char *)&synaptic_efficacies_or_weights[synapse_reversesort_indices[i]], sizeof(float));
  }
  
  weightfile.close();
}


void Synapses::load_weights(std::vector<float> weights, int synapsegroupid){
  int startid = 0;
  int endid = total_number_of_synapses;
  if (synapsegroupid >= 0)
    endid = last_index_of_synapse_per_group[synapsegroupid];
  if ((synapsegroupid > 0) && (synapsegroupid < last_index_of_synapse_per_group.size())){
    startid = last_index_of_synapse_per_group[synapsegroupid - 1];
  }

  if (weights.size() == (endid - startid)){
    for (int i = startid; i < endid; i++){
      synaptic_efficacies_or_weights[synapse_reversesort_indices[i]] = weights[i - startid];
    }
  } else {
    print_message_and_exit("Number of weights loading not equal to number of synapses!!");
  }
  
  if (_backend)
    backend()->copy_to_backend();
}

void Synapses::load_weights_from_txt(std::string filepath, int synapsegroupid){
  std::ifstream weightfile;
  weightfile.open(filepath, std::ios::in | std::ios::binary);
  
  // Getting values into a vector
  std::vector<float> loadingweights;
  float weightval = 0.0f;
  while (weightfile >> weightval){
    loadingweights.push_back(weightval);
  }
  weightfile.close();

  load_weights(loadingweights, synapsegroupid);

}
void Synapses::load_weights_from_binary(std::string filepath, int synapsegroupid){
  std::ifstream weightfile;
  weightfile.open(filepath, std::ios::in | std::ios::binary);

  // Getting values into a vector
  std::vector<float> loadingweights;
  float weightval = 0.0f;
  while (weightfile.read(reinterpret_cast<char*>(&weightval), sizeof(float))){
    loadingweights.push_back(weightval);
  }
  weightfile.close();
  
  load_weights(loadingweights, synapsegroupid);
}


SPIKE_MAKE_STUB_INIT_BACKEND(Synapses);
