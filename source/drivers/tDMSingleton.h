/*
 *  tDMSingleton.h
 *  Avida
 *
 *  Created by David on 10/31/08.
 *  Copyright 2008-2010 Michigan State University. All rights reserved.
 *
 *
 *  This file is part of Avida.
 *
 *  Avida is free software; you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 *  Avida is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License along with Avida.
 *  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef tDMSingleton_h
#define tDMSingleton_h

#include "cDMObject.h"
#include "cDriverManager.h"
#include "cMutex.h"


template<typename T> class tDMSingleton : public cDMObject
{
private:
  static tDMSingleton* s_dms;
  
  T* m_object;
  
  
  tDMSingleton(T* object) : m_object(object) { ; }
  
  tDMSingleton(); // @not_implemented
  tDMSingleton(const tDMSingleton&); // @not_implemented
  tDMSingleton& operator=(const tDMSingleton&); // @not_implemented
  
  
public:
  ~tDMSingleton() { delete m_object; }

  static void Initialize(T* (construct)())
  {
    if (!s_dms) {
      s_dms = new tDMSingleton((*construct)());
      cDriverManager::Register(s_dms);
    }
  }

  static T& GetInstance() { return *s_dms->m_object; }
};

template<typename T> tDMSingleton<T>* tDMSingleton<T>::s_dms = NULL;


template<typename T> class tLazyDMSingleton : public cDMObject
{
private:
  static tLazyDMSingleton* s_dms;
  
  cMutex m_mutex;
  T* (*m_construct)();
  T* m_object;
  
  
  tLazyDMSingleton(T* (*construct)()) : m_object(NULL), m_construct(construct) { ; }
  
  tLazyDMSingleton(); // @not_implemented
  tLazyDMSingleton(const tLazyDMSingleton&); // @not_implemented
  tLazyDMSingleton& operator=(const tLazyDMSingleton&); // @not_implemented
  
  
public:
  ~tLazyDMSingleton() { delete m_object; }
  
  static void Initialize(T* (*construct)())
  {
    if (!s_dms) {
      s_dms = new tLazyDMSingleton(construct);
      cDriverManager::Register(s_dms);
    }
  }

  static T& GetInstance()
  {
    cMutexAutoLock lock(s_dms->m_mutex);
    
    if (!s_dms->m_object) {
      s_dms->m_object = (*s_dms->m_construct)();
    }
    
    return *s_dms->m_object;
  }
};

template<typename T> tLazyDMSingleton<T>* tLazyDMSingleton<T>::s_dms = NULL;

#endif
