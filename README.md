NamedPipeDemo

# Overview

This is the demo of the home assignment regarding server-client IPC via NamedPipe. This implementation uses the WinAPI implementation of NamedPipe: https://docs.microsoft.com/en-us/windows/win32/ipc/named-pipes .

**NOTE**, that this project might not fully implement actual requirements, thus, I will make some assumptions and might interpret/implement some requirements a bit differently.

## Tech Stack
This repository uses the next technologies:
* C++17 - I used some features like `std::any` or [structured bindings](https://en.cppreference.com/w/cpp/language/structured_binding);
* CMake 3.8.0 - however I think lower versions are fine too;
* clang-format - this repo uses Google code style with some small modifications;
* WinAPI NamedPipe

## How to build
You should build the repo via `CMake`, for example:
```bash
mkdir build
cd build
cmake ../NamedPipeDemo -G "Visual Studio 15 2017"
```

## Architecture
The source code of this repository is divided into 3 submodules:
1. __NamedPipeClient__ executable (see [client folder](https://github.com/borzun/NamedPipeDemo/tree/master/client)) - the process, which will start a client and connect to the named pipe. You can send requests to the named pipe from the client and receive responses from the server.
2. __NamedPipeServer__ executable (see [server folder](https://github.com/borzun/NamedPipeDemo/tree/master/server)) - the process, which will create a named pipe and wait for connections. Each new connection is processed in a separate thread.
3. __NamedPipeCommon__ library (see [common folder](https://github.com/borzun/NamedPipeDemo/tree/master/common)) - the library, which shares same code between client and server. It contains some serializers/deserializer, common types, utilities.

Client and server are communicated between a pipe with name=`\\\\.\\pipe\\demo_pipe`. You can change it only in code, i.e. it is not configurable in a runtime.

Overall, about architecture - I strived to adhere to SRP, thus, creating separate classes to maintain low coupling in the system. I tried to use the dependency injection, to make sure that code is testable (__but it doesn't have tests__). However, there are some places, like singleton or utility classes with static methods, which can be improved.

Detailed info about the design of each component, can be seen in the following sections:

## How To Use
After you built both `NamedPipeClient` and `NamedPipeServer`, in order to run the program, you need to run both server and client. 

When you started the `NamedPipeClient`, it prompts you to choose either sync or async version of the client. Note that after you have chosen a version, you can't change your decision after that:

After that, the program prompts you to select the simulation mode - i.e. which way to generate the data for the server. Here are the possible modes:
* *step by step* (0) - the simulator will automatically choose the demo data sequentially; there is a small timeout between each request;
* *randomly* (1) - the simulator will choose the demo randomly; there is a small timeout between each request;
* *manually* (2) - user selects demo which will be run next.