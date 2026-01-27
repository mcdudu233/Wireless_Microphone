#pragma once

#include "ArduinoLog.h"

namespace logger
{
  void setup();

  template <class T, typename... Args>
  void debug(T msg, Args... args)
  {
    Log.verbose(msg, args...);
  };
  template <class T, typename... Args>
  void info(T msg, Args... args)
  {
    Log.info(msg, args...);
  };
  template <class T, typename... Args>
  void warn(T msg, Args... args)
  {
    Log.warning(msg, args...);
  };
  template <class T, typename... Args>
  void error(T msg, Args... args)
  {
    Log.error(msg, args...);
  };
  template <class T, typename... Args>
  void debugln(T msg, Args... args)
  {
    Log.verboseln(msg, args...);
  };
  template <class T, typename... Args>
  void infoln(T msg, Args... args)
  {
    Log.infoln(msg, args...);
  };
  template <class T, typename... Args>
  void warnln(T msg, Args... args)
  {
    Log.warningln(msg, args...);
  };
  template <class T, typename... Args>
  void errorln(T msg, Args... args)
  {
    Log.errorln(msg, args...);
  };
}
