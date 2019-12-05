#pragma once

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>

template <class T>
SharedObject<T>::SharedObject(Profile profile)
        : m_profile(profile), m_ipc(profile == Profile::creator ? IPC::Role::server : IPC::Role::client, [this](std::string_view message)
{
  //this->CopyObject(*reinterpret_cast<const Payload*>(message.data()));
  return SharedObject::MyObject(this);
}) {

}

template <class T> SharedObject<T>::~SharedObject() {
}

template <class T> typename SharedObject<T>::Handle SharedObject<T>::Acquire() {
  if(m_profile == Profile::user)
  {
    m_ipc.Send(0, [this](std::string_view serializedReturnValue) {
      memcpy(&m_object, serializedReturnValue.data(), serializedReturnValue.size());
    });
  }
  return Handle(&m_object, [](T*){});
}
