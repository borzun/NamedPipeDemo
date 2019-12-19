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

## Deviations from requirements
As stated in the [#Overview] section, I might be interpreted original requirements of `StreamBase` in a different way:
* REQ-5:
    > The server should be able to receive both sync/async connection requests from client
    >
    In my implementation I didn't use `overlapped I/O` on a **server** side, so, the server blocks and wait until the client will connect to a pipe. After that, it starts a new thread to process requests on that client's instance.
* REQ-7:
    > The server should be able to register a custom class (w/ related functions, attributes) which can be used by the client (see req-4)
    >
    I used the already defined class, which is accessible by both `server` and `client` - see [$CustomClass]. I thought about passing a custom class as a JSON for example, or just simple string, which contains a list of methods signatures and attributes, but in that case it is needed to implement some kind of a reflection, which is a bit hard for C++. Also, I couldn't find a way to simulate RPC, i.e. pass executable code via named pipe.

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

![dialog #1](https://github.com/borzun/NamedPipeDemo/blob/master/resources/dialog1.png)

After that, the program prompts you to select the simulation mode - i.e. which way to generate the data for the server. Here are the possible modes:
* *step by step* (0) - the simulator will automatically choose the demo data sequentially; there is a small timeout between each request;
* *randomly* (1) - the simulator will choose the demo randomly; there is a small timeout between each request;
* *manually* (2) - user selects demo which will be run next.

![dialog #2](https://github.com/borzun/NamedPipeDemo/blob/master/resources/dialog2.png)

# Components
Here are a detailed description of each component and their main classes.

## NamedPipeCommon

The `NamedPipeCommon` library contains auxiliary methods and classes for serializing/deserializing data and some other utilities. The corresponding classes are defined in [common folder](https://github.com/borzun/NamedPipeDemo/tree/master/common).

Next, let's discuss the main classes of this component.

### DataSerializer
The [DataSerializer](https://github.com/borzun/NamedPipeDemo/blob/master/common/DataSerializer.h) class responsible for serializing the corresponding strong type into raw data (`vector<char>`). Basically, it serializes the data into `std::ostream` and didn't perform any check on endianness.

The serializer is really simple and didn't use any data compression, like [Google protobuf](https://developers.google.com/protocol-buffers) or [Thrift](https://thrift.apache.org/). So, in order to properly identify which object is serialized, we need to pass a type designator (like `i` for integers, `d` for doubles, `s` for strings, etc.).

### DataDeserializer.
The [DataDeserializer](https://github.com/borzun/NamedPipeDemo/blob/master/common/DataDeserializer.h) class performs an inverse operation to the `DataSerializer` - by deserializing the data from raw format into a corresponding strong type. Users should specify to which type it should be deserialized by specifying the template parameter.

Note, that this class's methods signature differs from the serializer. The main difference is that the deserializer uses raw data with index instead `std::istream`. This breaks the [principle of least astonishment (POLA)](https://en.wikipedia.org/wiki/Principle_of_least_astonishment) and consistency, but it helps to quickly checks which parser should deserializer the data without modifying actual stream (`std::istream::unget`).

Also, as with `DataSerializer`, this class doesn't perform any checks on endianness. So, serializing/deserializing should be used only on the same machine. This issue should be addressed in the future.

### CustomClass
The [CustomClass](https://github.com/borzun/NamedPipeDemo/blob/master/common/CustomClass.h) is the class to meet the REQ-7 from `StreamBase` app.

### Logger
The [Logger](https://github.com/borzun/NamedPipeDemo/blob/master/common/Logger.h) class to protect from torn writes to the `std::cout` and `std::cin`. It uses the simple synchronization like `std::mutex` when writing to the output streams.

## NamedPipeClient

The client component sends data to the server (in both sync or async ways). The core of this component is [Client class](https://github.com/borzun/NamedPipeDemo/blob/master/client/Client.h), which accepts the [input data source](https://github.com/borzun/NamedPipeDemo/blob/master/client/IDataSource.h) from which it will read data, [parser class object](https://github.com/borzun/NamedPipeDemo/blob/master/client/ResponseParser.h), which will parse a responses from server on client's request and the [execution policy](https://github.com/borzun/NamedPipeDemo/blob/master/common/Types.h#L7). The `Client` uses internally the [Pipe](https://github.com/borzun/NamedPipeDemo/blob/master/client/Pipe.h) class to communicate with a server via NamedPipe API.

For simulation purposes, I created a helper class [DemoSimulator](https://github.com/borzun/NamedPipeDemo/blob/master/client/DemoSimulator.h), which is passed to `Client` class on the start and basically responsible for creating a corresponding [ClientRequest](https://github.com/borzun/NamedPipeDemo/blob/master/client/ClientRequest.h) objects, which then will be sent to a server. If `ClientRequest` needs to wait for a response from the server, it will be stored in the `ResponseParser` until a corresponding response with the same id will be sent back.

Note, that for responses on creating instances of `CustomClass` objects, I used a [ClassRepository](https://github.com/borzun/NamedPipeDemo/blob/master/client/ClassRepository.h) to store all the handles available at server.

Let's dive into implementations of main classes of this component:

## Client
A client reads input from the data source until it has some data to read and connects to a `Pipe`. If there is no instance of pipe, client will wait for 500 seconds or so until the pipe is ready. After that it sends data and (if needed) waits for a response. If the server closed the pipe, the client will then again try to reconnect for 500 seconds.

### Pipe
The `Pipe` class is based on the corresponding MSDN example: https://docs.microsoft.com/en-us/windows/win32/ipc/named-pipe-client. For asynchronous operations on a pipe, it uses the [Overlapped I/O](https://docs.microsoft.com/en-us/windows/win32/ipc/synchronous-and-overlapped-input-and-output). In order to properly wait for a responses and do not clutter the pipe (997 - **ERROR_IO_PENDING**), I used [RegisterWaitForSingleObject](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-registerwaitforsingleobject) on a completion event and after that retrieved results via [GetOverlappedResult](https://docs.microsoft.com/en-us/windows/win32/api/ioapiset/nf-ioapiset-getoverlappedresult) method. Maybe I should have used the [IOCP](https://docs.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports) - will try them in the future. However, it looks like the `RegisterWaitForSingleObject` gets a job done via callback - ofc, I allocate memory on a heap to handle the response in a callback, so, maybe there are some memory leaks, need to double check.

When sending data to the pipe, client decides whether it wants to wait for a response (via `ReadFile`). Note that it might hang when an error occurred on the server side processing your request.

### DemoSimulator
As said previously, the `DemoSimulator` creates a data request and the corresponding handlers of responses. Those handlers are basically callbacks, which are encapsulated in a class `ClientRequest`. Note, that when sending requests to a server to create instances of `CustomClass`, client uses the next format of requests:
* To create an instance of a class (server will return a handle - `int`):
```
#<typeid of class>#c<optional arguments to ctor>
```
* To call a method on an instance of a class (server might return a result (return type) of method's call):
```
#<typeid of class><handle>#m<MethodName><optional list of arguments>
```
* To get serialized instance of a class (server will return an instance of the class as `std::string`):
```
#<typeid of class><handle>#g
```
## NamedPipeServer
The server component, which is defined in a [server folder](https://github.com/borzun/NamedPipeDemo/tree/master/server) based on the next example - https://docs.microsoft.com/en-us/windows/win32/ipc/multithreaded-pipe-server . I.e. it will create an instance of a pipe when a new client connects to it and process requests to this pipe instance in a separate `std::thread`.

The main class in this component is [Server](https://github.com/borzun/NamedPipeDemo/blob/master/server/Server.h), which maintains a list of pipe instances and corresponding threads. It uses the [RequestParser](https://github.com/borzun/NamedPipeDemo/blob/master/server/RequestParser.h) to parse requests from client, more specifically, for requests on `CustomClass`, it uses [CustomClassParser](https://github.com/borzun/NamedPipeDemo/blob/master/server/CustomClassParser.h) class.

All instances of `CustomClass` are stored in the [ClassRegistry](https://github.com/borzun/NamedPipeDemo/blob/master/server/ClassRegistry.h)

# Issues
This implementation is far from perfect and, would say, still in WIP stage, so, it might contain some problems, like:
1. Endianness handling;
2. Problems when the server is destroyed - in that case, we need to properly synchronize on the pipe data. I achieved this via simple mutexes, but I didn't test this properly.
3. Getting public attributes of class - this is not implemented, but I think it would be easy to add this feature.
4. Sending errors back to the client - also, relatively easy to implement.
5. Overlapped I/O on the server.
6. Reflection when registering custom classes - see REQ-

Author - Bohdan Kurylovych (borzun)