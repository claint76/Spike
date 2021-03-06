#pragma once

#include "Spike/Synapses/ConductanceSpikingSynapses.hpp"
#include "SpikingSynapses.hpp"
#include "Spike/Backend/CUDA/CUDABackend.hpp"
#include <cuda.h>
#include <vector_types.h>
#include <curand.h>
#include <curand_kernel.h>

namespace Backend {
  namespace CUDA {
    struct conductance_spiking_synapses_data_struct: spiking_synapses_data_struct {
      float* decay_factors_g;
      float* reversal_potentials_Vhat;
      float* neuron_wise_conductance_trace;
    };
    class ConductanceSpikingSynapses : public virtual ::Backend::CUDA::SpikingSynapses,
                                       public virtual ::Backend::ConductanceSpikingSynapses {
    public:

      ~ConductanceSpikingSynapses() override;
      SPIKE_MAKE_BACKEND_CONSTRUCTOR(ConductanceSpikingSynapses);
      using ::Backend::ConductanceSpikingSynapses::frontend;

      // Variables used for memory-trace based synaptic input
      int conductance_trace_length = 0;
      float* neuron_wise_conductance_trace = nullptr;
      float* h_neuron_wise_conductance_trace = nullptr;
      float* d_decay_factors_g = nullptr;
      float* d_reversal_potentials_Vhat = nullptr;

      void prepare() override;
      void reset_state() override;

      void allocate_device_pointers(); // Not virtual
      void copy_constants_and_initial_efficacies_to_device(); // Not virtual

      void state_update(unsigned int current_time_in_timesteps, float timestep) final;

    };
    __device__ float conductance_spiking_current_injection_kernel(
        spiking_synapses_data_struct* in_synaptic_data,
        spiking_neurons_data_struct* neuron_data,
        float multiplication_to_volts,
        float current_membrane_voltage,
        unsigned int current_time_in_timesteps,
        float timestep,
        int idx,
        int g);
  }
}
