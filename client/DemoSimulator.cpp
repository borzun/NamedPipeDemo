#include "DemoSimulator.h"

#include <sstream>
#include <string>
#include <thread>
#include "ClassRepository.h"
#include "CustomClass.h"
#include "DataSerializer.h"
#include "Logger.h"

static constexpr auto kLogTag = "DemoSimulator";
static constexpr auto kTotalDemos = 11;

static constexpr auto kInvalidClassHandle = -1;

DemoSimulator::DemoSimulator(SimulationMode mode, size_t iterations,
                             std::chrono::milliseconds timeout)
    : mode_(mode), iterations_(iterations), wait_timeout_(timeout) {
  PrintHelp();
}

ClientRequest DemoSimulator::ReadRequest() {
  // small sleep between automatic simulations:
  if (curr_iteration_ > 0 && mode_ != SimulationMode::MANUAL) {
    std::this_thread::sleep_for(wait_timeout_);
  }

  Logger::LogDebug("\n");
  std::stringstream ss;
  bool wait_for_response = false;
  ClientRequest::SuccessCallbackType success_callback = nullptr;
  ClientRequest::FailureCallbackType failure_callback = nullptr;

  const int demo_index = GetDemoIndex();
  Logger::LogDebug(Logger::to_string(std::stringstream()
                                     << kLogTag << ": iteration=" << (curr_iteration_ + 1)
                                     << " of " << iterations_ << "; demo=" << demo_index));

  switch (demo_index) {
    case 0: {
      const int val = 42;
      DataSerializer::Serialize<int>(ss, val);
      Logger::LogDebug(Logger::to_string(std::stringstream()
                                         << kLogTag << ": Client wil send int value=" << val));
    } break;
    case 1: {
      const double val = -3658.15706;
      DataSerializer::Serialize<double>(ss, val);
      Logger::LogDebug(Logger::to_string(std::stringstream()
                                         << kLogTag << ": Client wil send double value=" << val));
    } break;
    case 2: {
      const std::string lollipop = "loli PoPq !@??zz z '\\ ~";
      DataSerializer::Serialize<std::string>(ss, lollipop);
      Logger::LogDebug(Logger::to_string(std::stringstream()
                                         << kLogTag << ": Client wil send std::string value=`"
                                         << lollipop << "`"));
    } break;
    case 3: {
      CreateCustomClassRequest(ss);
      ss << "#c";  // #c - create class
      wait_for_response = true;
      Logger::LogDebug(
          Logger::to_string(std::stringstream()
                            << kLogTag
                            << ": Client wil send create CustomClass request with default ctor"));
      success_callback = GetSuccessCallbackOnCustomClassCreation();
    } break;
    case 4: {
      CreateCustomClassRequest(ss);
      ss << "#c";
      const int val = 52;
      // pass argument to ctor:
      DataSerializer::Serialize<int>(ss, val);
      Logger::LogDebug(Logger::to_string(std::stringstream()
                                         << kLogTag
                                         << ": Client wil send create CustomClass request "
                                            "with ctor which accepts single int:"
                                         << val));
      success_callback = GetSuccessCallbackOnCustomClassCreation();
      wait_for_response = true;
    } break;
    case 5: {
      CreateCustomClassRequest(ss);
      ss << "#c";

      const int ival = 52;
      const std::string str = "Hello World!";
      DataSerializer::Serialize<int>(ss, 52);

      DataSerializer::Serialize<std::string>(ss, "Hello World");
      Logger::LogDebug(Logger::to_string(
          std::stringstream()
          << kLogTag << ": Client wil send create CustomClass request with 2 arguments: int="
          << ival << ", str=`" << str << "`"));
      success_callback = GetSuccessCallbackOnCustomClassCreation();
      wait_for_response = true;
    } break;
    case 6: {
      auto handle = GetClassHandleAtRandom();
      if (handle == kInvalidClassHandle) {
        Logger::LogDebug(Logger::to_string(
            std::stringstream() << kLogTag << ": ERROR - invalid class handle, please retry..."));
        return ClientRequest{};
      }
      CreateCallMethodRequest(ss, handle, "PrintToCout");
      Logger::LogDebug(Logger::to_string(std::stringstream()
                                         << kLogTag
                                         << ": Client wil call `PrintToCout` method on "
                                            "CustomClass instance with handle id="
                                         << handle));
    } break;
    case 7: {
      auto handle = GetClassHandleAtRandom();
      if (handle == kInvalidClassHandle) {
        Logger::LogDebug(Logger::to_string(
            std::stringstream() << kLogTag << ": ERROR - invalid class handle, please retry..."));
        return ClientRequest{};
      }
      CreateCallMethodRequest(ss, handle, "PrintToString");
      Logger::LogDebug(Logger::to_string(std::stringstream()
                                         << kLogTag
                                         << ": Client wil call `PrintToString` method on "
                                            "CustomClass instance with handle id="
                                         << handle));
      success_callback = [handle](std::any any) {
        try {
          const std::string str = std::any_cast<std::string>(any);

          Logger::LogDebug(Logger::to_string(
              std::stringstream() << kLogTag << ": Server called a `PrintToString` method=" << str
                                  << "; handle=" << handle));
        } catch (const std::bad_any_cast& exc) {
          Logger::LogError(Logger::to_string(std::stringstream()
                                             << kLogTag
                                             << ": parse error - can't cast to std::string "
                                                "of `PrintToString` return, error="
                                             << exc.what()));
        }
      };
      wait_for_response = true;
    } break;
    case 8: {
      auto handle = GetClassHandleAtRandom();
      if (handle == kInvalidClassHandle) {
        Logger::LogDebug(Logger::to_string(
            std::stringstream() << kLogTag << ": ERROR - invalid class handle, please retry..."));
        return ClientRequest{};
      }
      const int val = 750;
      CreateCallMethodRequest(ss, handle, "SetIntegerValue", val);
      Logger::LogDebug(Logger::to_string(
          std::stringstream() << kLogTag
                              << ": Client wil call `SetIntegerValue` method with argument=" << val
                              << " on CustomClass instance with handle id=" << handle));
      success_callback = [handle, val](std::any any) {
        try {
          const bool is_successfull = std::any_cast<bool>(any);

          Logger::LogDebug(Logger::to_string(
              std::stringstream() << kLogTag
                                  << ": Server called a `SetIntegerValue` method with val=" << val
                                  << "; result=" << is_successfull << "; handle=" << handle));
        } catch (const std::bad_any_cast& exc) {
          Logger::LogError(Logger::to_string(std::stringstream()
                                             << kLogTag
                                             << ": parse error - can't cast to bool of "
                                                "`SetIntegerValue` call response, error="
                                             << exc.what() << "!"));
        }
      };
      wait_for_response = true;
    } break;
    case 9: {
      auto handle = GetClassHandleAtRandom();
      if (handle == kInvalidClassHandle) {
        Logger::LogDebug(Logger::to_string(
            std::stringstream() << kLogTag << ": ERROR - invalid class handle, please retry..."));
        return ClientRequest{};
      }
      std::string str = "Hello World From Client";
      CreateCallMethodRequest(ss, handle, "SetStringValue", str);
      Logger::LogDebug(Logger::to_string(
          std::stringstream() << kLogTag
                              << ": Client wil call `SetStringValue` method with argument=" << str
                              << " on CustomClass instance with handle id=" << handle));
      success_callback = [handle, str](std::any any) {
        try {
          const bool is_successfull = std::any_cast<bool>(any);

          Logger::LogDebug(Logger::to_string(
              std::stringstream() << kLogTag
                                  << ": Server called a `SetStringValue` method with str=" << str
                                  << "; result=" << is_successfull << "; handle=" << handle));
        } catch (const std::bad_any_cast& exc) {
          Logger::LogError(Logger::to_string(std::stringstream()
                                             << kLogTag
                                             << ": parse error - can't cast to bool of "
                                                "`SetStringValue` call response, err="
                                             << exc.what() << "!"));
        }
      };
      wait_for_response = true;
    } break;
    case 10: {
      auto handle = GetClassHandleAtRandom();
      if (handle == kInvalidClassHandle) {
        Logger::LogDebug(Logger::to_string(
            std::stringstream() << kLogTag << ": ERROR - invalid class handle, please retry..."));
        return ClientRequest{};
      }
      CreateRetrieveInstanceRequest(ss, handle);
      wait_for_response = true;
      Logger::LogDebug(
          Logger::to_string(std::stringstream()
                            << kLogTag
                            << ": Client wil retrieve the instance of CustomClass with handle id="
                            << handle));
      success_callback = [handle](std::any any) {
        try {
          const std::string data = std::any_cast<std::string>(any);
          CustomClass obj = CustomClass::Deserialize(data);

          Logger::LogDebug(Logger::to_string(
              std::stringstream() << kLogTag
                                  << ": Server returned a CustomClass instance with handle="
                                  << handle));
          obj.PrintToCout();
        } catch (const std::bad_any_cast& exc) {
          Logger::LogError(Logger::to_string(
              std::stringstream() << kLogTag << ": parse error - can't cast to std::string, err="
                                  << exc.what() << "!"));
        }
      };
    }
  }

  std::string str = ss.str();
  using IterType = decltype(str.begin());
  auto data = RawDataType{std::move_iterator<IterType>(str.begin()),
                          std::move_iterator<IterType>(str.end())};

  ++curr_iteration_;

  return ClientRequest{data, wait_for_response, success_callback, failure_callback};
}

bool DemoSimulator::IsGood() const { return curr_iteration_ < iterations_; }

std::ostream& DemoSimulator::CreateCustomClassRequest(std::ostream& stream) const {
  stream << "#";
  DataSerializer::Serialize<std::string>(stream, typeid(CustomClass).name());
  return stream;
}

template <typename... Args>
std::ostream& DemoSimulator::CreateCallMethodRequest(std::ostream& stream, ClassHandle instance,
                                                     const std::string& method_name,
                                                     Args... args) const {
  CreateCustomClassRequest(stream);

  // Serialize the instance, which should be callsed
  DataSerializer::Serialize<ClassHandle>(stream, instance);

  // Serialize the method keyword and method name
  stream << "#m";
  DataSerializer::Serialize<std::string>(stream, method_name);

  // Serialize all the arguments:
  SerializeArguments(stream, args...);

  return stream;
}

template <typename Arg0, typename... Args>
std::ostream& DemoSimulator::SerializeArguments(std::ostream& stream, Arg0 arg0,
                                                Args... args) const {
  DataSerializer::Serialize<Arg0>(stream, arg0);

  return SerializeArguments(stream, args...);
}

ClassHandle DemoSimulator::GetClassHandleAtRandom() const {
  ClassRepository& repository = ClassRepository::GetInstance();
  const size_t handles_size = repository.GetAllHandles().size();
  if (handles_size == 0) {
    return -1;
  }

  const auto handle_idx = rand() % handles_size;
  return repository.GetAllHandles()[handle_idx];
}

std::ostream& DemoSimulator::CreateRetrieveInstanceRequest(std::ostream& stream,
                                                           ClassHandle instance) const {
  CreateCustomClassRequest(stream);

  // Serialize the instance, which should be callsed
  DataSerializer::Serialize<ClassHandle>(stream, instance);

  stream << "#g";
  return stream;
}

ClientRequest::SuccessCallbackType DemoSimulator::GetSuccessCallbackOnCustomClassCreation() const {
  return [](std::any any) {
    try {
      const ClassHandle handle = std::any_cast<ClassHandle>(any);
      ClassRepository::GetInstance().RegisterClassHandle(handle);
      Logger::LogDebug(Logger::to_string(
          std::stringstream() << kLogTag << ": Server created a CustomClass instance with handle: "
                              << handle));
    } catch (const std::bad_any_cast& exc) {
      Logger::LogError(
          Logger::to_string(std::stringstream()
                            << kLogTag << ": parse error - can't cast to ClassHandle type, error="
                            << exc.what() << "!"));
    }
  };
}

int DemoSimulator::GetDemoIndex() const {
  switch (mode_) {
    case SimulationMode::STEP_BY_STEP:
      return curr_iteration_ % kTotalDemos;
    case SimulationMode::RANDOM:
      return rand() % kTotalDemos;
    case SimulationMode::MANUAL: {
      while (true) {
        if (curr_iteration_ == 0) {
          Logger::LogDebug(Logger::to_string(
              std::stringstream() << "Please, choose the demo index from 0 to 10. Type help in "
                                     "order to show the help again:"));
        }
        std::string input;
        std::getline(std::cin, input);

        if (input == "help") {
          PrintHelp();
          continue;
        } else {
          try {
            const int val = std::stoi(input);
            if (val < 0 || val >= kTotalDemos) {
              Logger::LogError(
                  Logger::to_string(std::stringstream() << "There is no demo with such index..."));
            } else {
              return val;
            }
          } catch (std::invalid_argument& exc) {
            Logger::LogError(Logger::to_string(
                std::stringstream()
                << "Invalid number! Please, type correct value, err=" << exc.what()));
          }
        }
        return -1;
      }
    }
  }

  return -1;
}

void DemoSimulator::PrintHelp() const {
  Logger::LogDebug("This is a demo simulator, which will send command to the server.\n");
  Logger::LogDebug(
      Logger::to_string(std::stringstream()
                        << "Currently client can send next commands (which are depicted by "
                           "corresponding index):\n"
                        << " 0 - Send integer number to the server. No response.\n"
                        << " 1 - Send double number to the server. No response.\n"
                        << " 2 - Send string to the server. No response.\n"
                        << " 3 - Create a CustomClass object on a server via default ctor. "
                           "Server will send handle(int) as a response.\n"
                        << " 4 - Create a CustomClass object on a server via ctor with 1 "
                           "argument: int. Server will send handle(int) as a response.\n"
                        << " 5 - Create a CustomClass object on a server via ctor with 2 "
                           "arguments: std::string and int. Server will send handle(int) as a "
                           "response.\n"
                        << " 6 - Print to std::cout random CustomClass object on server. No "
                           "response from server.\n"
                        << " 7 - Server prints a random CustomClass to std::string and send that "
                           "string back to client. Server will send a std::string as a "
                           "response.\n"
                        << " 8 - Server changes integer attribute (SetIntegerValue method) of "
                           "random CustomClass object. Server will send a bool indicating "
                           "success of this operation.\n"
                        << " 9 - Server changes std::string attribute (SetStringValue method) of "
                           "random CustomClass object. Server will send a bool indicating "
                           "success of this operation.\n"
                        << " 10 - Request a CustomClass object with specific handle from server. "
                           "Server will send serialized version of the CustomClass object.\n"));

  switch (mode_) {
    case SimulationMode::STEP_BY_STEP:
      Logger::LogDebug(
          "This demo runs in step-by-step mode, means it will execute all "
          "demos from 0 to 10 in sequantual mode with some small sleep between "
          "demos.");
      break;
    case SimulationMode::RANDOM:
      Logger::LogDebug(
          "This demo runs in random mode. This means it will pick a demo index "
          "at random from 0 till 10 and will execute that demo.");
      break;
    case SimulationMode::MANUAL:
      Logger::LogDebug(
          "This demo runs in manual mode. You need manually run a demo by "
          "entering the demo index from 0 to 10.");
      break;
  }
}