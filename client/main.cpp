#include <iostream>
#include "Client.h"
#include "DemoSimulator.h"
#include "ResponseParser.h"

int main(int argc, char** argv) {
  std::cout << "Hello. You are starting a NamedPipeClient!\n"
            << "Which version of client do you want to start - sync (0) or "
               "async (1)? Please enter a number..."
            << std::endl;
  int exec = 0;
  std::cin >> exec;
  ExecutionPolicy exec_policy = exec == 1 ? ExecutionPolicy::Async : ExecutionPolicy::Sync;

  // choose demo simulator mode:
  std::cout << "\n\n";
  std::cout << "Please, choose a simulation mode: \n"
            << "\t - 0 - step by step simulation - each step from "
               "DemoSimulator will be executed one by one;\n"
            << "\t - 1 - random simulation - DemoSimulator will execute steps "
               "randomly;\n"
            << "\t - 2 - manual simulation selection - You will chosee which "
               "step to execute\n"
            << " Please, choose a number 0, 1 or 2..." << std::endl;

  int execution_mode = 0;
  std::cin >> execution_mode;

  auto simulation_mode = SimulationMode::STEP_BY_STEP;
  switch (execution_mode) {
    case 0:
      simulation_mode = SimulationMode::STEP_BY_STEP;
      break;
    case 1:
      simulation_mode = SimulationMode::RANDOM;
      break;
    case 2:
      simulation_mode = SimulationMode::MANUAL;
      break;
  }

  auto parser = std::make_shared<ResponseParser>();
  const int kStepsCount = 10000;  // Number of steps to execute
  auto data_source = std::make_shared<DemoSimulator>(simulation_mode, kStepsCount);

  const std::string pipe_name = "\\\\.\\pipe\\demo_pipe";
  Client client(pipe_name, data_source, parser, exec_policy);
  if (!client.Start()) {
    std::cerr << "ERROR - exiting application with error - see logs!" << std::endl;
	int stop = 0;
	std::cin >> stop;
    return -1;
  }

  std::cout << "Closing application..." << std::endl;
  

  return 0;
}
