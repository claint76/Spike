#ifndef GeneratorInputSpikingNeurons_H
#define GeneratorInputSpikingNeurons_H

#include "InputSpikingNeurons.hpp"

struct generator_input_spiking_neuron_parameters_struct : input_spiking_neuron_parameters_struct {
	generator_input_spiking_neuron_parameters_struct() { input_spiking_neuron_parameters_struct(); }
};

class GeneratorInputSpikingNeurons; // forward definition

namespace Backend {
  class GeneratorInputSpikingNeurons : public virtual InputSpikingNeurons {
  public:
    SPIKE_ADD_BACKEND_FACTORY(GeneratorInputSpikingNeurons);
    virtual void setup_stimulus() = 0;
  };
} 

class GeneratorInputSpikingNeurons : public InputSpikingNeurons {
public:
  ~GeneratorInputSpikingNeurons() override;

  void init_backend(Context* ctx) override;
  SPIKE_ADD_BACKEND_GETSET(GeneratorInputSpikingNeurons, InputSpikingNeurons);
  
  // Variables
  int length_of_longest_stimulus;
  float stimulus_onset_adjustment = 0.0f;

  // Host Pointers
  int* number_of_spikes_in_stimuli = nullptr;
  int** neuron_id_matrix_for_stimuli = nullptr;
  float** spike_times_matrix_for_stimuli = nullptr;
  float* temporal_lengths_of_stimuli = nullptr;

  int add_stimulus(int spikenumber, int* ids, float* spiketimes);
  int add_stimulus(std::vector<int> ids, std::vector<float> spiketimes);

  void select_stimulus(int stimulus_index) override;

private:
  std::shared_ptr<::Backend::GeneratorInputSpikingNeurons> _backend;
};

#endif
