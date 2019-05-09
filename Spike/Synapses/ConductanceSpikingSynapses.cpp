#include "ConductanceSpikingSynapses.hpp"
#include "../Helpers/TerminalHelpers.hpp"

// ConductanceSpikingSynapses Destructor
ConductanceSpikingSynapses::~ConductanceSpikingSynapses() {
}


int ConductanceSpikingSynapses::AddGroup(int presynaptic_group_id, 
                                          int postsynaptic_group_id, 
                                          Neurons * neurons,
                                          Neurons * input_neurons,
                                          float timestep,
                                          synapse_parameters_struct * synapse_params) {
  
  
  int groupID = SpikingSynapses::AddGroup(presynaptic_group_id, 
                            postsynaptic_group_id, 
                            neurons,
                            input_neurons,
                            timestep,
                            synapse_params);

  conductance_spiking_synapse_parameters_struct * conductance_spiking_synapse_group_params = (conductance_spiking_synapse_parameters_struct*)synapse_params;
  
  // Incrementing number of synapses
  ConductanceSpikingSynapses::increment_number_of_synapses(temp_number_of_synapses_in_last_group);

  // Set up unique parameter labels
  bool found = false;
  int param_label = 0;
  for (int p = 0; p < reversal_potentials_Vhat.size(); p++){
    if ((conductance_spiking_synapse_group_params->reversal_potential_Vhat == reversal_potentials_Vhat[p]) && (conductance_spiking_synapse_group_params->decay_term_tau_g == decay_terms_tau_g[p])){
      found = true;
      param_label = p;
    }
  }
  if (!found){
    param_label = reversal_potentials_Vhat.size();
    // Set constants
    reversal_potentials_Vhat.push_back(conductance_spiking_synapse_group_params->reversal_potential_Vhat);
    decay_terms_tau_g.push_back(conductance_spiking_synapse_group_params->decay_term_tau_g);
    
    // Keep number of parameter labels up to date
    number_of_parameter_labels = reversal_potentials_Vhat.size();
  }



  for (int i = (total_number_of_synapses - temp_number_of_synapses_in_last_group); i < total_number_of_synapses; i++) {
    parameter_labels[i] = param_label;
  }
  
  return(groupID);
}

void ConductanceSpikingSynapses::increment_number_of_synapses(int increment) {
}


void ConductanceSpikingSynapses::state_update(unsigned int current_time_in_timesteps, float timestep) {
  backend()->state_update(current_time_in_timesteps, timestep);
}

SPIKE_MAKE_INIT_BACKEND(ConductanceSpikingSynapses);
