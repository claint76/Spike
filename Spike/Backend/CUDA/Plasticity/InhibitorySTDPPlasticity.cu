// -*- mode: c++ -*-
#include "Spike/Backend/CUDA/Plasticity/InhibitorySTDPPlasticity.hpp"

SPIKE_EXPORT_BACKEND_TYPE(CUDA, InhibitorySTDPPlasticity);

namespace Backend {
  namespace CUDA {
    InhibitorySTDPPlasticity::~InhibitorySTDPPlasticity() {
      CudaSafeCall(cudaFree(vogels_pre_memory_trace));
      CudaSafeCall(cudaFree(vogels_post_memory_trace));
    }

    void InhibitorySTDPPlasticity::reset_state() {
      STDPPlasticity::reset_state();

      CudaSafeCall(cudaMemcpy((void*)vogels_pre_memory_trace,
                              (void*)vogels_memory_trace_reset,
                              sizeof(float)*total_number_of_plastic_synapses,
                              cudaMemcpyHostToDevice));
      CudaSafeCall(cudaMemcpy((void*)vogels_post_memory_trace,
                              (void*)vogels_memory_trace_reset,
                              sizeof(float)*total_number_of_plastic_synapses,
                              cudaMemcpyHostToDevice));
      CudaSafeCall(cudaMemcpy((void*)vogels_prevupdate,
                              (void*)vogels_memory_trace_reset,
                              sizeof(float)*total_number_of_plastic_synapses,
                              cudaMemcpyHostToDevice));
    }

    void InhibitorySTDPPlasticity::prepare() {
      STDPPlasticity::prepare();

      vogels_memory_trace_reset = (float*)malloc(sizeof(float)*total_number_of_plastic_synapses);
      for (int i=0; i < total_number_of_plastic_synapses; i++){
  vogels_memory_trace_reset[i] = 0.0f;
      }

      allocate_device_pointers();
    }

    void InhibitorySTDPPlasticity::allocate_device_pointers() {
      // The following doesn't do anything in original code...
      // ::Backend::CUDA::STDPPlasticity::allocate_device_pointers();

      CudaSafeCall(cudaMalloc((void **)&vogels_pre_memory_trace, sizeof(int)*total_number_of_plastic_synapses));
      CudaSafeCall(cudaMalloc((void **)&vogels_post_memory_trace, sizeof(int)*total_number_of_plastic_synapses));
      CudaSafeCall(cudaMalloc((void **)&vogels_prevupdate, sizeof(int)*total_number_of_plastic_synapses));
    }

    void InhibitorySTDPPlasticity::apply_stdp_to_synapse_weights(int current_time_in_timesteps, float timestep) {

    // Vogels update rule requires a neuron wise memory trace. This must be updated upon neuron firing.
    vogels_apply_stdp_to_synapse_weights_kernel<<<neurons_backend->number_of_neuron_blocks_per_grid, neurons_backend->threads_per_block>>>
       (synapses_backend->postsynaptic_neuron_indices,
       synapses_backend->presynaptic_neuron_indices,
       synapses_backend->delays,
       neurons_backend->d_neuron_data,
       input_neurons_backend->d_neuron_data,
       synapses_backend->synaptic_efficacies_or_weights,
       vogels_pre_memory_trace,
       vogels_post_memory_trace,
       expf(- timestep / frontend()->stdp_params->tau_istdp),
       *(frontend()->stdp_params),
       timestep,
       frontend()->model->timestep_grouping,
       current_time_in_timesteps,
       plastic_synapse_indices,
       total_number_of_plastic_synapses);
    CudaCheckError();
    }

    __global__ void vogels_apply_stdp_to_synapse_weights_kernel
          (int* d_postsyns,
           int* d_presyns,
           int* d_syndelays,
           spiking_neurons_data_struct* neuron_data,
           spiking_neurons_data_struct* input_neuron_data,
           float* d_synaptic_efficacies_or_weights,
           float* vogels_pre_memory_trace,
           float* vogels_post_memory_trace,
           float trace_decay,
           struct inhibitory_stdp_plasticity_parameters_struct stdp_vars,
           float timestep,
           int timestep_grouping,
           int current_time_in_timesteps,
           int* d_plastic_synapse_indices,
           size_t total_number_of_plastic_synapses){
      // Global Index
      int indx = threadIdx.x + blockIdx.x * blockDim.x;

      // Running though all neurons
      while (indx < total_number_of_plastic_synapses) {
        int idx = d_plastic_synapse_indices[indx];

        // Getting synapse details
        float vogels_pre_memory_trace_val = vogels_pre_memory_trace[indx];
        float vogels_post_memory_trace_val = vogels_post_memory_trace[indx];
        int postid = d_postsyns[idx];
        int preid = d_presyns[idx];
        int bufsize = input_neuron_data->neuron_spike_time_bitbuffer_bytesize[0];
        float new_synaptic_weight = d_synaptic_efficacies_or_weights[idx];

        // Correcting for input vs output neuron types
        bool is_input = PRESYNAPTIC_IS_INPUT(preid);
        int corr_preid = CORRECTED_PRESYNAPTIC_ID(preid, is_input);
        uint8_t* pre_bitbuffer = is_input ? input_neuron_data->neuron_spike_time_bitbuffer : neuron_data->neuron_spike_time_bitbuffer;

        // Looping over timesteps
        for (int g=0; g < timestep_grouping; g++){	
          // Decaying STDP traces
          vogels_pre_memory_trace_val *= trace_decay;
          vogels_post_memory_trace_val *= trace_decay;

          // Bit Indexing to detect spikes
          int postbitloc = (current_time_in_timesteps + g) % (bufsize*8);
          int prebitloc = postbitloc - d_syndelays[idx];
          prebitloc = (prebitloc < 0) ? (bufsize*8 + prebitloc) : prebitloc;

          // OnPre Trace Update
          if (pre_bitbuffer[corr_preid*bufsize + (prebitloc / 8)] & (1 << (prebitloc % 8))){
            vogels_pre_memory_trace_val += 1.0f;
          }
          // OnPost Trace Update
          if (neuron_data->neuron_spike_time_bitbuffer[postid*bufsize + (postbitloc / 8)] & (1 << (postbitloc % 8))){
            vogels_post_memory_trace_val += 1.0f;
          }
          
          float syn_update_val = 0.0f; 
          // OnPre Weight Update
          if (pre_bitbuffer[corr_preid*bufsize + (prebitloc / 8)] & (1 << (prebitloc % 8))){
            syn_update_val += stdp_vars.learningrate*(vogels_post_memory_trace_val);
            syn_update_val += - stdp_vars.learningrate*(2.0*stdp_vars.targetrate*stdp_vars.tau_istdp);
          }
          // OnPost Weight Update
          if (neuron_data->neuron_spike_time_bitbuffer[postid*bufsize + (postbitloc / 8)] & (1 << (postbitloc % 8))){
            syn_update_val += stdp_vars.learningrate*(vogels_pre_memory_trace_val);
          }

          new_synaptic_weight += syn_update_val;
          // Weight Update
          if (new_synaptic_weight < 0.0f){
            new_synaptic_weight = 0.0f;
          }
        }

        // Update Weight
        d_synaptic_efficacies_or_weights[idx] = new_synaptic_weight;
        // Correctly set the trace values
        vogels_pre_memory_trace[indx] = vogels_pre_memory_trace_val;
        vogels_post_memory_trace[indx] = vogels_post_memory_trace_val;

        indx += blockDim.x * gridDim.x;
      }

    }
  }
}
