#pragma once

#include <chrono>
#include <iostream>
#include <string>
#include "ClientRequest.h"
#include "IDataSource.h"

enum class SimulationMode {
  // The simulation will be performed one by one with some sleep between steps;
  STEP_BY_STEP = 0,
  // The simulation will be performed randomly with some sleep between steps;
  RANDOM,
  // User selects the actual step
  MANUAL
};

// This class performs simulation of Demo.
// Basically, it can automatically select which data to send or you can manually choose which data to send to server.
class DemoSimulator : public IDataSource {
 public:
	 // mode - simulation mode
	 // iterations - number of demos to run.
	 // timeout - timeout between auto demo 
  explicit DemoSimulator(SimulationMode mode, size_t iterations, std::chrono::milliseconds timeout = std::chrono::milliseconds(2000));

  ClientRequest ReadRequest() override;

  bool IsGood() const override;

 private:
  int GetDemoIndex() const;

  void PrintHelp() const;

 private:
  std::ostream& CreateCustomClassRequest(std::ostream& stream) const;

  template <typename... Args>
  std::ostream& CreateCallMethodRequest(std::ostream& stream, ClassHandle instance,
                                        const std::string& method_name, Args... args) const;

  std::ostream& SerializeArguments(std::ostream& stream) const { return stream; }

  template <typename Arg0, typename... Args>
  std::ostream& SerializeArguments(std::ostream& stream, Arg0 arg0, Args... args) const;

  std::ostream& CreateRetrieveInstanceRequest(std::ostream& stream, ClassHandle instance) const;

  ClassHandle GetClassHandleAtRandom() const;

  // Callbacks:
  ClientRequest::SuccessCallbackType GetSuccessCallbackOnCustomClassCreation() const;

 private:
  const SimulationMode mode_;
  const size_t iterations_;
  size_t curr_iteration_ = 0;
  const std::chrono::milliseconds wait_timeout_;
};