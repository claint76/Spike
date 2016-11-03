#include "../Simulator/Simulator.h"
#include "../Models/FourLayerVisionSpikingModel.h"

#include "../SpikeAnalyser/SpikeAnalyser.h"
#include "../Helpers/TimerWithMessages.h"
#include "../Helpers/TerminalHelpers.h"


// Use the following line to compile the binary
// make FILE='JITest' EXPERIMENT_DIRECTORY='Experiments'  model -j8


// The function which will autorun when the executable is created
int main (int argc, char *argv[]){

	TimerWithMessages * experiment_timer = new TimerWithMessages();

	
	// Simulator Parameters
	// float timestep = 0.00002;
	float timestep = 0.00002;
	bool high_fidelity_spike_storage = true;

	// bool simulate_network_to_test_untrained = true;
	// bool simulate_network_to_train_network = true;
	// bool simulate_network_to_test_trained = true;
	
	// // Parameters for OPTIMISATION + Information Analysis
	// const float optimal_max_firing_rate = 100.0f;//set if optimizing based on maxfr //Maximum rate (spikes/sec) 87 +- 46  (*1)
	// //*1 Bair, W., & Movshon, J. A. (2004).  Adaptive Temporal Integration of Motion in Direction-Selective Neurons in Macaque Visual Cortex. The Journal of Neuroscience, 24(33), 7305遯ｶ�ｿｽ7323.
	// int number_of_bins = 5;
	// bool useThresholdForMaxFR = true;
	// const float presentation_time_per_stimulus_per_epoch_test = 0.5f;
	// float max_firing_rate = optimal_max_firing_rate*presentation_time_per_stimulus_per_epoch_test;


	float presentation_time_per_stimulus_per_epoch = 0.2;


	// SIMULATOR OPTIONS
	Simulator_Options * simulator_options = new Simulator_Options();

	simulator_options->run_simulation_general_options->presentation_time_per_stimulus_per_epoch = presentation_time_per_stimulus_per_epoch;
	simulator_options->run_simulation_general_options->number_of_epochs = 1;
	simulator_options->run_simulation_general_options->apply_stdp_to_relevant_synapses = false;
	simulator_options->run_simulation_general_options->stimulus_presentation_order_seed = 1;

	simulator_options->recording_electrodes_options->count_neuron_spikes_recording_electrodes_bool = true;

	simulator_options->stimuli_presentation_options->presentation_format = PRESENTATION_FORMAT_OBJECT_BY_OBJECT_RESET_BETWEEN_OBJECTS;
	simulator_options->stimuli_presentation_options->object_order = OBJECT_ORDER_ORIGINAL;
	simulator_options->stimuli_presentation_options->transform_order = TRANSFORM_ORDER_ORIGINAL;

	int index_of_neuron_group_index_of_interest = 0;
	int number_of_optimisation_stages = 1;
	float initial_optimisation_parameter_min = 1.0f*pow(10, -12);;
	float initial_optimisation_parameter_max = 1.0*pow(10, 1);
	float optimisation_parameter_min = initial_optimisation_parameter_min;
	float optimisation_parameter_max = initial_optimisation_parameter_max;

	float optimisation_ideal_output_score = 100.0;
	float optimisation_threshold = 5.0;
	
	for (int optimisation_stage = 0; optimisation_stage < number_of_optimisation_stages; optimisation_stage++) {

		float final_optimal_parameter = 0.0; // Eventually have an array of these to use on subsequent optimisation_stage iterations :)
		int number_of_iterations_for_optimisation_stage = 0;

		float previous_optimisation_output_score = -1.0;

		while (true) {

			number_of_iterations_for_optimisation_stage++;
			

			// MODEL
			FourLayerVisionSpikingModel * four_layer_vision_spiking_model = new FourLayerVisionSpikingModel();
			four_layer_vision_spiking_model->SetTimestep(timestep);

			four_layer_vision_spiking_model->number_of_non_input_layers = 1;
			four_layer_vision_spiking_model->INHIBITORY_NEURONS_ON = false;


			float test_optimisation_parameter_value = (optimisation_parameter_max + optimisation_parameter_min) / 2.0;
			

			four_layer_vision_spiking_model->LBL_biological_conductance_scaling_constant_lambda_E2E_FF[0] = test_optimisation_parameter_value;

			four_layer_vision_spiking_model->finalise_model();
			four_layer_vision_spiking_model->copy_model_to_device(high_fidelity_spike_storage);

			// CREATE SIMULATOR
			Simulator * simulator = new Simulator(four_layer_vision_spiking_model, simulator_options);

			// RUN SIMULATION
			SpikeAnalyser * spike_analyser = new SpikeAnalyser(four_layer_vision_spiking_model->spiking_neurons, four_layer_vision_spiking_model->input_spiking_neurons);
			simulator->RunSimulation(spike_analyser);

			spike_analyser->calculate_various_neuron_spike_totals_and_averages(presentation_time_per_stimulus_per_epoch);

			// float optimisation_output_score = spike_analyser->average_number_of_spikes_per_neuron_group_per_second[index_of_neuron_group_index_of_interest];
			float optimisation_output_score = spike_analyser->max_number_of_spikes_per_neuron_group_per_second[index_of_neuron_group_index_of_interest];

			printf("previous_optimisation_output_score: %f\n", previous_optimisation_output_score);
			printf("number_of_iterations_for_optimisation_stage: %d\n", number_of_iterations_for_optimisation_stage);
			printf("optimisation_parameter_max: %.12f\n", optimisation_parameter_max);
			printf("optimisation_parameter_min: %.12f\n", optimisation_parameter_min);
			printf("test_optimisation_parameter_value: %.12f\n", test_optimisation_parameter_value);
			printf("optimisation_output_score: %f\n", optimisation_output_score);

			

			if (optimisation_output_score <= optimisation_ideal_output_score) {

				if (optimisation_ideal_output_score - optimisation_output_score < optimisation_threshold) {
					final_optimal_parameter = optimisation_output_score;
					break;
				} else {
					optimisation_parameter_min = test_optimisation_parameter_value;
				}

			} else if (optimisation_output_score >= optimisation_ideal_output_score) {

				if (optimisation_output_score - optimisation_ideal_output_score < optimisation_threshold) {
					final_optimal_parameter = optimisation_output_score;
					break;
				} else {
					optimisation_parameter_max = test_optimisation_parameter_value;
				}


			}

			printf("NEW optimisation_parameter_max: %.12f\n", optimisation_parameter_max);
			printf("NEW optimisation_parameter_min: %.12f\n", optimisation_parameter_min);


			print_line_of_dashes_with_blank_lines_either_side();

			free(four_layer_vision_spiking_model);
			free(simulator);

			previous_optimisation_output_score = optimisation_output_score;
			
		}	

		printf("final_optimal_parameter: %f\n", final_optimal_parameter);
		printf("number_of_iterations_for_optimisation_stage: %d\n", number_of_iterations_for_optimisation_stage);

	}


	/////////// END OF EXPERIMENT ///////////
	experiment_timer->stop_timer_and_log_time_and_message("Experiment Completed.", true);

	return 0;
}