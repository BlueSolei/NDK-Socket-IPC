#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#include <functional>
#include <string_view>
#include <string>
#include <thread>

#include "AndroidLog.h"

// Can be anything if using abstract namespace
#define SOCKET_NAME "serverSocket"
#define BUFFER_SIZE 16

using object_t = int;

class IPC {
public:

  struct Server {
  };
  struct Client {
  };

  using OnReturn = std::function<void(std::string_view data)>;
  using OnRecieve = std::function<std::string_view(std::string_view data)>;
  using BoolPtr = std::shared_ptr<std::atomic<bool>>;

  IPC(Server, OnRecieve onRecieve) {
    m_threadServer = std::thread(SetupServer, onRecieve, m_isQuit);
  }

  IPC(Client) {
    SetupClient();
  }

  ~IPC() {
    StopServer();
    StopClient();
  }

  template<class T>
  void Send(const T &value, const OnReturn &onReturn) {
    SendData({reinterpret_cast<const char *>(&value), sizeof(T)}, onReturn);
  }

private:

  void StopServer() {
    *m_isQuit = true;
    if (m_threadServer.joinable()) m_threadServer.join();
  }

  void StopClient()
  {
    close(m_data_socket);
  }

  static void SetupServer(OnRecieve onRecieve, BoolPtr isQuit) {
    int ret;
    struct sockaddr_un server_addr;
    int socket_fd;
    int data_socket;
    std::string buffer;
    char socket_name[108]; // 108 sun_path length max

    LOGI("** Start server setup");

    // AF_UNIX for domain unix IPC and SOCK_STREAM since it works for the example
    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) {
      LOGE("** socket: %s", strerror(errno));
      return;
    }
    LOGI("** Socket made");

    // NDK needs abstract namespace by leading with '\0'
    // Ya I was like WTF! too... http://www.toptip.ca/2013/01/unix-domain-socket-with-abstract-socket.html?m=1
    // Note you don't need to unlink() the socket then
    memcpy(&socket_name[0], "\0", 1);
    strcpy(&socket_name[1], SOCKET_NAME);

    // clear for safty
    memset(&server_addr, 0, sizeof(struct sockaddr_un));
    server_addr.sun_family = AF_UNIX; // Unix Domain instead of AF_INET IP domain
    strncpy(server_addr.sun_path, socket_name, sizeof(server_addr.sun_path) - 1); // 108 char max

    ret = bind(socket_fd, (const struct sockaddr *) &server_addr, sizeof(struct sockaddr_un));
    if (ret < 0) {
      LOGE("** bind: %s", strerror(errno));
      return;
    }
    LOGI("** Bind made");

    // Open 8 back buffers for this demo
    ret = listen(socket_fd, 8);
    if (ret < 0) {
      LOGE("** listen: %s", strerror(errno));
      return;
    }
    LOGI("** Socket listening for packages");

    // Wait for incoming connection.
    data_socket = accept(socket_fd, NULL, NULL);
    if (data_socket < 0) {
      LOGE("** accept: %s", strerror(errno));
      return;
    }
    LOGI("** Accepted data");
    // This is the main loop for handling connections
    // Assuming in example connection is established only once
    // Would be better to refactor this for robustness
    while (!*isQuit) {

      LOGI("** Server read ...");
      buffer = ReadMessage(data_socket, buffer);
      if(buffer.size() == 0) continue;

      auto answer = onRecieve(buffer);

      // Send back result
      WriteMessage(data_socket, answer);

      // Close socket between accepts
    }
    close(data_socket);
    close(socket_fd);
  }

  static std::string &ReadMessage(int data_socket, std::string &buffer) {
    buffer.resize(0);
    size_t messageSize = 0;
    LOGI("** Read a message header: size");
    auto ret = read(data_socket, &messageSize, sizeof(messageSize));
    if (ret < 0) {
      LOGE("** 'read msg size' failed: %s", strerror(errno));
      return buffer;
    }
    LOGI("** Will read a message of %d bytes", messageSize);

    buffer.resize(messageSize);
    auto bufferPtr = &buffer[0];
    for(size_t howMuchRead = 0, reminder = messageSize; reminder > 0; bufferPtr += howMuchRead, reminder -= howMuchRead)
    {
      // Wait for next data packet
      howMuchRead = read(data_socket, bufferPtr, reminder);
      if (howMuchRead < 0) {
        LOGE("** read: %s", strerror(errno));
        break;
      } else if(0 == howMuchRead) {
        buffer.resize(messageSize - reminder);
        LOGI("** Read() received no data! message truncated fro %d to %d", messageSize, buffer.size());
        break;
      }

      LOGI("** Read a partial message of %d bytes", howMuchRead);
      LOGI("** Buffer read: %d", buffer[0]);
    }
    return buffer;
  }

  void SetupClient() {
    LOGI("** Start client setup");
    char socket_name[108]; // 108 sun_path length max

    m_data_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_data_socket < 0) {
      LOGE("** socket: %s", strerror(errno));
      return;
    }

    // NDK needs abstract namespace by leading with '\0'
    // Ya I was like WTF! too... http://www.toptip.ca/2013/01/unix-domain-socket-with-abstract-socket.html?m=1
    // Note you don't need to unlink() the socket then
    memcpy(&socket_name[0], "\0", 1);
    strcpy(&socket_name[1], SOCKET_NAME);

    // clear for safty
    memset(&m_server_addr, 0, sizeof(struct sockaddr_un));
    m_server_addr.sun_family = AF_UNIX; // Unix Domain instead of AF_INET IP domain
    strncpy(m_server_addr.sun_path, socket_name, sizeof(m_server_addr.sun_path) - 1); // 108 char max

    // Assuming only one init connection for demo
    int ret = connect(m_data_socket, (const struct sockaddr *) &m_server_addr, sizeof(struct sockaddr_un));
    if (ret < 0) {
      LOGE("** connect: %s", strerror(errno));
      return;
    }

    LOGI("** Client Setup Complete");
  }

  void SendData(std::string_view data, const OnReturn& onReturn) {
    auto err = WriteMessage(m_data_socket, data);
    if(err == -1) return;

    LOGI("** Client receive ...");
    auto answer = ReadMessage(m_data_socket, m_buffer);
    if(answer.size() == 0) return;

    onReturn(answer);
    LOGI("** Return: %s", (char*)answer.c_str());
  }

  static int WriteMessage(int data_socket, std::string_view data)
  {
    LOGI("** Sending data: %d, size: %d", data[0], data.size());
    std::uint32_t messageSize = data.size();
    auto ret = write(data_socket, &messageSize, sizeof(messageSize));
    if (ret < 0) {
      LOGE("** write: %s", strerror(errno));
      return -1;
    }

    ret = write(data_socket, data.data(), data.size());
    if (ret < 0) {
      LOGE("** write: %s", strerror(errno));
      return -1;
    }
    return 0;
  }

private:

  int m_data_socket = -1;
  struct sockaddr_un m_server_addr;

  std::string m_buffer;

  BoolPtr m_isQuit = std::make_shared<std::atomic<bool>>(false);
  std::thread m_threadServer;
};
using IPCUPtr = std::unique_ptr<IPC>;