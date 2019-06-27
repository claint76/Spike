#pragma once

__device__ float my_conductance_spiking_injection_kernel(
    spiking_synapses_data_struct* in_synaptic_data,
    spiking_neurons_data_struct* neuron_data,
    float multiplication_to_volts,
    float current_membrane_voltage,
    unsigned int current_time_in_timesteps,
    float timestep,
    int idx,
    int g){
  
  conductance_spiking_synapses_data_struct* synaptic_data = (conductance_spiking_synapses_data_struct*) in_synaptic_data;
    
  int bufferloc = ((current_time_in_timesteps + g) % synaptic_data->neuron_inputs.temporal_buffersize)*synaptic_data->neuron_inputs.input_buffersize;
    
  float total_current = 0.0f;
  for (int syn_label = 0; syn_label < synaptic_data->num_syn_labels; syn_label++){
    float decay_factor = synaptic_data->decay_factors_g[syn_label];
    float reversal_value = synaptic_data->reversal_potentials_Vhat[syn_label];
    float synaptic_conductance_g = synaptic_data->neuron_wise_conductance_trace[syn_label + idx*synaptic_data->num_syn_labels];
    // Update the synaptic conductance
    synaptic_conductance_g *= decay_factor;
    float conductance_increment = synaptic_data->neuron_inputs.circular_input_buffer[bufferloc + syn_label + idx*synaptic_data->num_syn_labels];
    if (conductance_increment != 0.0f){
      synaptic_conductance_g += conductance_increment*synaptic_data->weight_scaling_constants[syn_label];
      // Reset the conductance update
      synaptic_data->neuron_inputs.circular_input_buffer[bufferloc + syn_label + idx*synaptic_data->num_syn_labels] = 0.0f;
    }
    total_current += synaptic_conductance_g*(reversal_value - current_membrane_voltage);

    synaptic_data->neuron_wise_conductance_trace[syn_label + idx*synaptic_data->num_syn_labels] = synaptic_conductance_g;

  }
  return total_current*multiplication_to_volts;
};

__device__ float my_current_spiking_injection_kernel(
    spiking_synapses_data_struct* in_synaptic_data,
    spiking_neurons_data_struct* neuron_data,
    float multiplication_to_volts,
    float current_membrane_voltage,
    unsigned int current_time_in_timesteps,
    float timestep,
    int idx,
    int g){
  
  current_spiking_synapses_data_struct* synaptic_data = (current_spiking_synapses_data_struct*) in_synaptic_data;
    
  int bufferloc = ((current_time_in_timesteps + g) % synaptic_data->neuron_inputs.temporal_buffersize)*synaptic_data->neuron_inputs.input_buffersize;
  float total_current = 0.0f;

  for (int syn_label = 0; syn_label < synaptic_data->num_syn_labels; syn_label++){
    float decay_term_value = synaptic_data->decay_terms_tau[syn_label];
    float decay_factor = expf(- timestep / decay_term_value);
    float synaptic_current = synaptic_data->neuron_wise_current_trace[syn_label + idx*synaptic_data->num_syn_labels];
    // Update the synaptic current
    synaptic_current *= decay_factor;
    float current_increment = synaptic_data->neuron_inputs.circular_input_buffer[bufferloc + syn_label + idx*synaptic_data->num_syn_labels];
    if (current_increment != 0.0){
      synaptic_current += current_increment*synaptic_data->weight_scaling_constants[syn_label];
      // Reset the current update
      synaptic_data->neuron_inputs.circular_input_buffer[bufferloc + syn_label + idx*synaptic_data->num_syn_labels] = 0.0f;
    }
    total_current += synaptic_current;
    synaptic_data->neuron_wise_current_trace[syn_label + idx*synaptic_data->num_syn_labels] = synaptic_current;

  }

  return total_current*multiplication_to_volts;
};


__device__ float my_voltage_spiking_injection_kernel(
    spiking_synapses_data_struct* in_synaptic_data,
    spiking_neurons_data_struct* neuron_data,
    float multiplication_to_volts,
    float current_membrane_voltage,
    unsigned int current_time_in_timesteps,
    float timestep,
    int idx,
    int g){
  
  spiking_synapses_data_struct* synaptic_data = (spiking_synapses_data_struct*) in_synaptic_data;
    
  int bufferloc = ((current_time_in_timesteps + g) % synaptic_data->neuron_inputs.temporal_buffersize)*synaptic_data->neuron_inputs.input_buffersize;

  float total_current = 0.0f;
  for (int syn_label = 0; syn_label < synaptic_data->num_syn_labels; syn_label++){
    float input_current = synaptic_data->neuron_inputs.circular_input_buffer[bufferloc + syn_label + idx*synaptic_data->num_syn_labels];
    if (input_current != 0.0f){
      total_current += input_current;
      synaptic_data->neuron_inputs.circular_input_buffer[bufferloc + syn_label + idx*synaptic_data->num_syn_labels] = 0.0f;
    }
  }


  // This is already in volts, no conversion necessary
  return total_current;
}
