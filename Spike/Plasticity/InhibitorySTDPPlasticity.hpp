// Inhibitory STDPPlasticity Class Header
// InhibitorySTDPPlasticity.h
//
// This STDPPlasticity learning rule is extracted from the following paper:

//  Vogels, T. P., H. Sprekeler, F. Zenke, C. Clopath, and W. Gerstner. 2011. “Inhibitory Plasticity Balances Excitation and Inhibition in Sensory Pathways and Memory Networks.” Science 334 (6062): 1569–73.

// This implementation is based upon the inhibitory STDP rule described for detailed balance


#ifndef INHIBIT_STDP_H
#define INHIBIT_STDP_H

// Get Synapses & Neurons Class
#include "../Synapses/SpikingSynapses.hpp"
#include "../Neurons/SpikingNeurons.hpp"
#include "../Plasticity/STDPPlasticity.hpp"

// stdlib allows random numbers
#include <stdlib.h>
// Input Output
#include <stdio.h>
// allows maths
#include <math.h>

class InhibitorySTDPPlasticity; // forward definition

namespace Backend {
  class InhibitorySTDPPlasticity : public virtual STDPPlasticity {
  public:
    SPIKE_ADD_BACKEND_FACTORY(InhibitorySTDPPlasticity);

    virtual void apply_stdp_to_synapse_weights(unsigned int current_time_in_timesteps, float timestep) = 0;
  };
}

// STDP Parameters
struct inhibitory_stdp_plasticity_parameters_struct : stdp_plasticity_parameters_struct {
  inhibitory_stdp_plasticity_parameters_struct() : tau_istdp(0.02f), learningrate(0.0004f), targetrate(10.0f), momentumrate{0.0f}, w_max{1000.0f} { } // default Constructor
  // STDP Parameters
  float tau_istdp;
  float learningrate;
  float targetrate;
  float momentumrate;
  float w_max;
  // Alpha must be calculated as 2 * targetrate * tau_istdp
};


class InhibitorySTDPPlasticity : public STDPPlasticity {
public:
  InhibitorySTDPPlasticity(SpikingSynapses* synapses,
                           SpikingNeurons* neurons,
                           SpikingNeurons* input_neurons,
                           stdp_plasticity_parameters_struct* stdp_parameters);
  ~InhibitorySTDPPlasticity() override;
  SPIKE_ADD_BACKEND_GETSET(InhibitorySTDPPlasticity, STDPPlasticity);

  struct inhibitory_stdp_plasticity_parameters_struct* stdp_params;

  void init_backend(Context* ctx = _global_ctx) override;
  void prepare_backend_late() override;

  void state_update(unsigned int current_time_in_timesteps, float timestep) override;

private:
  std::shared_ptr<::Backend::InhibitorySTDPPlasticity> _backend;
};

#endif
