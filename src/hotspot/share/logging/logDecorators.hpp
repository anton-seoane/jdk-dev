/*
 * Copyright (c) 2015, 2023, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */
#ifndef SHARE_LOGGING_LOGDECORATORS_HPP
#define SHARE_LOGGING_LOGDECORATORS_HPP

#include "utilities/globalDefinitions.hpp"
#include "logging/logSelection.hpp"

class outputStream;

// The list of available decorators:
// time         - Current time and date in ISO-8601 format
// uptime       - Time since the start of the JVM in seconds and milliseconds (e.g., 6.567s)
// timemillis   - The same value as generated by System.currentTimeMillis()
// uptimemillis - Milliseconds since the JVM started
// timenanos    - The same value as generated by System.nanoTime()
// uptimenanos  - Nanoseconds since the JVM started
// hostname     - The hostname
// pid          - The process identifier
// tid          - The thread identifier
// level        - The level associated with the log message
// tags         - The tag-set associated with the log message
#define DECORATOR_LIST          \
  DECORATOR(time,         t)    \
  DECORATOR(utctime,      utc)  \
  DECORATOR(uptime,       u)    \
  DECORATOR(timemillis,   tm)   \
  DECORATOR(uptimemillis, um)   \
  DECORATOR(timenanos,    tn)   \
  DECORATOR(uptimenanos,  un)   \
  DECORATOR(hostname,     hn)   \
  DECORATOR(pid,          p)    \
  DECORATOR(tid,          ti)   \
  DECORATOR(level,        l)    \
  DECORATOR(tags,         tg)

// LogDecorators represents a selection of decorators that should be prepended to
// each log message for a given output. Decorators are always prepended in the order
// declared above. For example, logging with 'uptime, level, tags' decorators results in:
// [0,943s][info   ][logging] message.
class LogDecorators {
  friend class TestLogDecorators;
 public:
  enum Decorator {
#define DECORATOR(name, abbr) name##_decorator,
    DECORATOR_LIST
#undef DECORATOR
    Count,
    Invalid,
    NoDecorators
  };

  class DefaultDecorator {
    friend class TestLogDecorators;
    LogSelection _selection;
    uint         _mask;

    DefaultDecorator() : _selection(LogSelection::Invalid), _mask(0) {}

  public:
    template<typename... Tags>
    DefaultDecorator(LogLevelType level, uint mask, LogTagType first, Tags... rest) : _selection(LogSelection::Invalid), _mask(mask) {
      static_assert(1 + sizeof...(rest) <= LogTag::MaxTags + 1,
                    "Too many tags specified!");

      LogTagType tag_arr[LogTag::MaxTags + 1] = { first, rest... };

      if (sizeof...(rest) == LogTag::MaxTags) {
        assert(tag_arr[sizeof...(rest)] == LogTag::__NO_TAG,
               "Too many tags specified! Can only have up to " SIZE_FORMAT " tags in a tag set.", LogTag::MaxTags);
      }

      _selection = LogSelection(tag_arr, false, level);
    }

    const LogSelection& selection() const { return _selection; }
    uint mask()                     const { return _mask; }
  };

 private:
  uint _decorators;
  static const char* _name[][2];
  static const uint defaultsMask = (1 << uptime_decorator) | (1 << level_decorator) | (1 << tags_decorator);
  static const LogDecorators::DefaultDecorator default_decorators[];
  static const size_t number_of_default_decorators;

  static uint mask(LogDecorators::Decorator decorator) {
    return 1 << decorator;
  }


 public:
  static const LogDecorators None;
  static const LogDecorators All;

  constexpr LogDecorators(uint mask) : _decorators(mask) {}

  LogDecorators() : _decorators(defaultsMask) {}

  void clear() {
    _decorators = 0;
  }

  static const char* name(LogDecorators::Decorator decorator) {
    return _name[decorator][0];
  }

  static const char* abbreviation(LogDecorators::Decorator decorator) {
    return _name[decorator][1];
  }

  template<typename... Decorators>
  static uint mask_from_decorators(LogDecorators::Decorator first, Decorators... rest) {
    uint bitmask = 0;
    LogDecorators::Decorator decorators[1 + sizeof...(rest)] = { first, rest... };
    for (const LogDecorators::Decorator decorator : decorators) {
      if (decorator == NoDecorators) return 0;
      bitmask |= mask(decorator);
    }
    return bitmask;
  }

  // Check if we have some default decorators for a given LogSelection. If that is the case,
  // the output parameter mask will contain the defaults-specified decorators mask
  static bool has_default_decorator(const LogSelection& selection, uint* mask, const DefaultDecorator* defaults = default_decorators, size_t defaults_count = number_of_default_decorators);

  static LogDecorators::Decorator from_string(const char* str);

  void combine_with(const LogDecorators &source) {
    _decorators |= source._decorators;
  }

  bool is_empty() const {
    return _decorators == 0;
  }

  bool is_decorator(LogDecorators::Decorator decorator) const {
    return (_decorators & mask(decorator)) != 0;
  }

  bool parse(const char* decorator_args, outputStream* errstream = nullptr);
};

#endif // SHARE_LOGGING_LOGDECORATORS_HPP
