/*
 * ViSP, open source Visual Servoing Platform software.
 * Copyright (C) 2005 - 2025 by Inria. All rights reserved.
 *
 * This software is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file LICENSE.txt at the root directory of this source
 * distribution for additional information about the GNU GPL.
 *
 * For using ViSP with software that can not be combined with the GNU
 * GPL, please contact Inria about acquiring a ViSP Professional
 * Edition License.
 *
 * See https://visp.inria.fr for more information.
 *
 * This software was developed at:
 * Inria Rennes - Bretagne Atlantique
 * Campus Universitaire de Beaulieu
 * 35042 Rennes Cedex
 * France
 *
 * If you have questions regarding the use of this file, please contact
 * Inria at visp@inria.fr
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Description:
 * UDP Client
 */

#include <cstring>
#include <sstream>

#include <visp3/core/vpConfig.h>

// inet_ntop() not supported on win XP
#ifdef VISP_HAVE_FUNC_INET_NTOP

#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))) // UNIX
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <unistd.h>
#define DWORD int
#else
#if defined(__MINGW32__)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_VISTA // 0x0600
#endif
#endif

#if defined(__clang__)
// Mute warning : non-portable path to file '<WS2tcpip.h>'; specified path differs in case from file name on disk [-Wnonportable-system-include-path]
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wnonportable-system-include-path"
#endif

#include <Ws2tcpip.h>

#if defined(__clang__)
#  pragma clang diagnostic pop
#endif

#endif

#include <visp3/core/vpUDPClient.h>

BEGIN_VISP_NAMESPACE
/*!
  Default constructor.

  Use connect() to establish the connexion with the server.
*/
vpUDPClient::vpUDPClient()
  : m_is_init(false), m_serverAddress(), m_serverLength(0), m_socketFileDescriptor()
#if defined(_WIN32)
  ,
  m_wsa()
#endif
{ }

vpUDPClient::vpUDPClient(const vpUDPClient &client)
{
  *this = client;
}

vpUDPClient &vpUDPClient::operator=(const vpUDPClient &client)
{
  m_is_init = client.m_is_init;
  m_serverAddress = client.m_serverAddress;
  m_serverLength = client.m_serverLength;
  m_socketFileDescriptor = client.m_socketFileDescriptor;
#if defined(_WIN32)
  m_wsa = client.m_wsa;
#endif
  return *this;
}

/*!
  Create a (IPv4) UDP client.

  \param hostname : Server hostname or IP address.
  \param port : Server port number.
*/
vpUDPClient::vpUDPClient(const std::string &hostname, int port)
  : m_is_init(false), m_serverAddress(), m_serverLength(0), m_socketFileDescriptor()
#if defined(_WIN32)
  ,
  m_wsa()
#endif
{
  init(hostname, port);
}

/*!
   Destructor.
 */
vpUDPClient::~vpUDPClient() { close(); }

/*!
   Close UDP connexion.
 */
void vpUDPClient::close()
{
  if (m_is_init) {
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))) // UNIX
    ::close(m_socketFileDescriptor);
#else
    closesocket(m_socketFileDescriptor);
    WSACleanup();
#endif
    m_is_init = false;
  }
}

/*!
  Initialize a (IPv4) UDP client.

  \param hostname : Server hostname or IP address.
  \param port : Server port number.
*/
void vpUDPClient::init(const std::string &hostname, int port)
{
  // Close connexion if already initialized
  close();

#if defined(_WIN32)
  if (WSAStartup(MAKEWORD(2, 2), &m_wsa) != 0) {
    std::stringstream ss;
    ss << "Failed WSAStartup for the server, error code: " << WSAGetLastError();
    throw vpException(vpException::fatalError, ss.str());
  }
#endif

  /* socket: create the socket */
  m_socketFileDescriptor = socket(AF_INET, SOCK_DGRAM, 0);
#if defined(_WIN32)
  if (m_socketFileDescriptor == INVALID_SOCKET)
#else
  if (m_socketFileDescriptor < 0)
#endif
    throw vpException(vpException::fatalError, "Error opening UDP socket for the client!");

  /* build the server's Internet address */
  memset(&m_serverAddress, 0, sizeof(m_serverAddress));
  std::stringstream ss;
  ss << port;
  struct addrinfo hints;
  struct addrinfo *result = nullptr;
  struct addrinfo *ptr = nullptr;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_protocol = IPPROTO_UDP;

  DWORD dwRetval = getaddrinfo(hostname.c_str(), ss.str().c_str(), &hints, &result);
  if (dwRetval != 0) {
    ss.str("");
    ss << "getaddrinfo failed with error: " << dwRetval;
    throw vpException(vpException::fatalError, ss.str());
  }

  for (ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
    if (ptr->ai_family == AF_INET && ptr->ai_socktype == SOCK_DGRAM) {
      m_serverAddress = *(struct sockaddr_in *)ptr->ai_addr;
      break;
    }
  }

  freeaddrinfo(result);

  m_serverLength = sizeof(m_serverAddress);
  m_is_init = true;
}

/*!
  Receive data sent by the server.

  \param msg : ASCII message or byte data.
  \param timeoutMs : Timeout in millisecond (if zero, the call is blocking).

  \return The message length / size of the byte array sent received, or -1 if
  there is an error, or 0 if there is a timeout.

  \note
  To transform the ASCII representation of an integer:
  \code
  int val = atoi(msg.c_str());
  //or
  std::istringstream ss(msg);
  ss >> val;
  \endcode
  To convert from a byte array to an integer:
  \code
  int val = *reinterpret_cast<const int *>(msg.c_str());
  \endcode
*/
int vpUDPClient::receive(std::string &msg, int timeoutMs)
{
  if (!m_is_init) {
    throw(vpException(vpException::notInitialized, "UDP client is not initialized"));
  }
  fd_set s;
  FD_ZERO(&s);
  FD_SET(m_socketFileDescriptor, &s);
  struct timeval timeout;
  if (timeoutMs > 0) {
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
  }
  int retval = select(static_cast<int>(m_socketFileDescriptor) + 1, &s, nullptr, nullptr, timeoutMs > 0 ? &timeout : nullptr);

  if (retval == -1) {
    std::cerr << "Error select!" << std::endl;
    return -1;
  }

  if (retval > 0) {
    /* recvfrom: receive a UDP datagram from the server */
    int length = static_cast<int>(recvfrom(m_socketFileDescriptor, m_buf, sizeof(m_buf), 0,
                                           (struct sockaddr *)&m_serverAddress, (socklen_t *)&m_serverLength));
    if (length <= 0) {
      return length < 0 ? -1 : 0;
    }

    msg = std::string(m_buf, length);
    return length;
  }

  // Timeout
  return 0;
}

/*!
  Receive data sent by the server.

  \param msg : A message to send over the network.
  \param len : Message length.
  \param timeoutMs : Timeout in millisecond (if zero, the call is blocking).

  \return The message length / size of the byte array sent received, or -1 if
  there is an error, or 0 if there is a timeout.
*/
int vpUDPClient::receive(void *msg, size_t len, int timeoutMs)
{
  if (!m_is_init) {
    throw(vpException(vpException::notInitialized, "UDP client is not initialized"));
  }
  fd_set s;
  FD_ZERO(&s);
  FD_SET(m_socketFileDescriptor, &s);
  struct timeval timeout;
  if (timeoutMs > 0) {
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;
  }
  int retval = select(static_cast<int>(m_socketFileDescriptor) + 1, &s, nullptr, nullptr, timeoutMs > 0 ? &timeout : nullptr);

  if (retval == -1) {
    std::cerr << "Error select!" << std::endl;
    return -1;
  }

  if (retval > 0) {
    /* recvfrom: receive a UDP datagram from the server */
    int length = static_cast<int>(recvfrom(m_socketFileDescriptor, (char *)msg, static_cast<int>(len), 0,
                                           (struct sockaddr *)&m_serverAddress, (socklen_t *)&m_serverLength));
    if (length <= 0) {
      return length < 0 ? -1 : 0;
    }

    return length;
  }

  // Timeout
  return 0;
}

/*!
  Send data to the server.

  \param msg : ASCII message or byte data.

  \return The message length / size of the byte array sent.

  \note
  To send the ASCII representation of an integer:
  \code
  int val = 1024;
  std::ostringstream os;
  os << val;
  server.send(os.str(), hostname, port);
  \endcode
  To send directly the byte data (assuming the same integer representation on
  the server and the client):
  \code
  int val = 1024;
  char data[sizeof(val)];
  memcpy(data, &val, sizeof(val));
  std::string msg(data, sizeof(val)); //required to avoid the string being splitted with the first \0 character
  server.send(msg, hostname, port);
  \endcode
*/
int vpUDPClient::send(const std::string &msg)
{
  if (!m_is_init) {
    throw(vpException(vpException::notInitialized, "UDP client is not initialized"));
  }
  if (msg.size() > VP_MAX_UDP_PAYLOAD) {
    std::cerr << "Message is too long!" << std::endl;
    return 0;
  }

/* send the message to the server */
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))) // UNIX
  return static_cast<int>(
      sendto(m_socketFileDescriptor, msg.c_str(), msg.size(), 0, (struct sockaddr *)&m_serverAddress, m_serverLength));
#else
  return sendto(m_socketFileDescriptor, msg.c_str(), static_cast<int>(msg.size()), 0, (struct sockaddr *)&m_serverAddress,
                m_serverLength);
#endif
}

/*!
  Send data to the server.

  \param msg : Message to send.
  \param len : Message length.

  \return The message length / size of the byte array sent.

*/
int vpUDPClient::send(const void *msg, size_t len)
{
  if (!m_is_init) {
    throw(vpException(vpException::notInitialized, "UDP client is not initialized"));
  }
  if (len > VP_MAX_UDP_PAYLOAD) {
    std::cerr << "Message is too long!" << std::endl;
    return 0;
  }

/* send the message to the server */
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))) // UNIX
  return static_cast<int>(
      sendto(m_socketFileDescriptor, msg, len, 0, (struct sockaddr *)&m_serverAddress, m_serverLength));
#else
  return sendto(m_socketFileDescriptor, (char *)msg, static_cast<int>(len), 0, (struct sockaddr *)&m_serverAddress, m_serverLength);
#endif
}
END_VISP_NAMESPACE
#elif !defined(VISP_BUILD_SHARED_LIBS)
// Work around to avoid warning: libvisp_core.a(vpUDPClient.cpp.o) has no symbols
void dummy_vpUDPClient() { }
#endif
