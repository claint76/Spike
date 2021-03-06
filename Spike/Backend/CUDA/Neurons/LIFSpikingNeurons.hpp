#pragma once

#include "Spike/Neurons/LIFSpikingNeurons.hpp"
#include "Spike/Neurons/SpikingNeurons.hpp"
#include "Spike/Backend/CUDA/CUDABackend.hpp"
#include "Spike/Backend/CUDA/Neurons/SpikingNeurons.hpp"
#include "Spike/Backend/CUDA/Synapses/ConductanceSpikingSynapses.hpp"
#include "Spike/Backend/CUDA/Synapses/CurrentSpikingSynapses.hpp"
#include "Spike/Backend/CUDA/Synapses/VoltageSpikingSynapses.hpp"

#include <cuda.h>
#include <vector_types.h>

namespace Backend {
  namespace CUDA {
    struct lif_spiking_neurons_data_struct: spiking_neurons_data_struct {
      float* membrane_potentials_v; 
      float* membrane_time_constants_tau_m;
      float* membrane_decay_constants;
      float* membrane_resistances_R;
      float* thresholds_for_action_potential_spikes;
      float* resting_potentials_v0;
      float* after_spike_reset_potentials_vreset;
      float* background_currents;
      int* refractory_timesteps;
      int* refraction_counter;
      int* neuron_labels;
    };

    class LIFSpikingNeurons : public virtual ::Backend::CUDA::SpikingNeurons,
                              public virtual ::Backend::LIFSpikingNeurons {
    public:
      float* membrane_potentials_v;
      float * membrane_time_constants_tau_m = nullptr;
      float * membrane_decay_constants = nullptr;
      float * membrane_resistances_R = nullptr;
      float* thresholds_for_action_potential_spikes = nullptr;
      float* resting_potentials_v0 = nullptr;
      float* after_spike_reset_potentials_vreset = nullptr;
      float* background_currents = nullptr;
      int* refractory_timesteps = nullptr;

      int* refraction_counter = nullptr;
      int* neuron_labels = nullptr;

      ~LIFSpikingNeurons() override;
      SPIKE_MAKE_BACKEND_CONSTRUCTOR(LIFSpikingNeurons);
      using ::Backend::LIFSpikingNeurons::frontend;

      void prepare() override;
      void reset_state() override;

      void copy_constants_to_device(); // Not virtual
      void allocate_device_pointers(); // Not virtual

      void state_update(unsigned int current_time_in_timesteps, float timestep) override;
    };

    __global__ void lif_update_membrane_potentials(
        injection_kernel current_injection_kernel,
        synaptic_activation_kernel syn_activation_kernel,
        spiking_synapses_data_struct* synaptic_data,
        spiking_neurons_data_struct* neuron_data,
        float timestep,
        int timestep_grouping,
        float current_time_in_seconds,
        unsigned int current_time_in_timesteps,
        size_t total_number_of_neurons);
  }
}
