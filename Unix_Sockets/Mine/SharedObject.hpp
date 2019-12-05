#pragma once

#include <functional>
#include <memory>
#include <string>

#include "UnixDomainSocket.h"

// clang-format off
/** Class for a shared object in shared memory
 *
 * \tparam T the object we share. will be created in shared memory
 *
 *	example of use:
 * \code
	 #include "SharedObject.hpp"

	 // A regular class
	 struct MachineInfo {
	  enum class CPU { x86, ARM };

	  CPU cpu;
	  char ip[4] = {0};
	  char hostName[100] = {0}; // since object is in shared memory we can't use std::string
	  // etc ...
	};

	// Its shared contrapart, a class which is created in shared memory, and identified by a unique name
	using SharedMachineInfo = SharedObject<MachineInfo>;

	// Code of the process which 'create' the SO 
		SharedMachineInfo so("MachineInfo", MySharedObject::Profile::creator);

		// ...
		// some time later
		// in this block the use of the object is exclusive (r\w)
		std::string hostName;
		{
			// read\write from\to the SO
			auto info = so.Aquire();
			info.cpu = MachineInfo::CPU::ARM; 
			info.ip = {197, 12, 13, 78};
			hostName = info.hostName; // read the value from the SO
		}
		// Here the lock is released so the other process can read\write it now

	// Code in a process which 'use' the SO
		SharedMachineInfo so("MachineInfo", MySharedObject::Profile::user);

		// ...
		// some time later
		// in this block the use of the object is exclusive (r\w)
		char ip[4] = {0};
		{
		  auto info = so.Aquire();
		  ip = info.ip; // read from the SO
		  info.hostName = "Victoria" // you can also write to the SO, its 'shared' :-)
		}
		// Here the lock is released so the other process can read\write it now
  \endcode
*/
// clang-format on

template <class T> class SharedObject {
public:
  using Payload = T;
  using FnReleaseObject = std::function<void(T *)>;
  using Handle = std::unique_ptr<T, FnReleaseObject>;

  /** Are you the SO 'creator' or the SO already exits, and you are just a
   * 'user' of it?
   */
  enum class Profile {
    creator //! The process which create the SO
    ,
    user //! A process which use the SO
  };

public:
  /**
   *
   * \param objectName the name of the SO. you need it to be the same in all
   * processes which uses it \param profile see #Profile
   */
  SharedObject(Profile profile);

  /** dtor
   * if you are the 'creator' of the SO, it will be destroyed here
   */
  ~SharedObject();

  /** Acquire a synchronized handle to the SO
   *
   * \return a Handle to the SO. the handle is used as pointer to the SO.
   *    The handle is scoped, so the lock is released
   *    at the end of the scope.
   */
  Handle Acquire();

  void CopyObject(const T& value) { m_object = value; }

private:
  const Profile m_profile;
  Payload m_object;
  IPC m_ipc;

  static std::string_view MyObject(SharedObject *pObject)
  {
    return {reinterpret_cast<char*>(&pObject->m_object), sizeof(Payload)};
  }
};

//! template implementation
#include "SharedObject.inl"
