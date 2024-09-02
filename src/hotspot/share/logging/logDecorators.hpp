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

#define DEFAULT_DECORATORS \
  DEFAULT_VALUE((1 << pid_decorator) | (1 << tags_decorator), Trace, LOG_TAGS(ref, gc)) \
  DEFAULT_VALUE(0, Trace, LOG_TAGS(jit))

// LogDecorators represents a selection of decorators that should be prepended to
// each log message for a given output. Decorators are always prepended in the order
// declared above. For example, logging with 'uptime, level, tags' decorators results in:
// [0,943s][info   ][logging] message.
class LogDecorators {
 public:
  enum Decorator {
#define DECORATOR(name, abbr) name##_decorator,
    DECORATOR_LIST
#undef DECORATOR
    Count,
    Invalid
  };

  class DefaultDecorator {
  private:

  public:
    DefaultDecorator() {}
    static const DefaultDecorator Invalid;

    DefaultDecorator(LogLevelType level, uint mask, ...) : _mask(mask) {
      size_t i;
      va_list ap;
      LogTagType tags[LogTag::MaxTags];
      va_start(ap, mask);
      for (i = 0; i < LogTag::MaxTags; i++) {
        LogTagType tag = static_cast<LogTagType>(va_arg(ap, int));
        tags[i] = tag;
        if (tag == LogTag::__NO_TAG) {
          assert(i > 0, "Must specify at least one tag!");
          break;
        }
      }
      assert(i < LogTag::MaxTags || static_cast<LogTagType>(va_arg(ap, int)) == LogTag::__NO_TAG,
            "Too many tags specified! Can only have up to " SIZE_FORMAT " tags in a tag set.", LogTag::MaxTags);
      va_end(ap);

      _selection = LogSelection(tags, false, level);
    }

    LogSelection selection() const { return _selection; }
    uint mask()              const { return _mask; }

    bool operator==(const DefaultDecorator& ref) const;
    bool operator!=(const DefaultDecorator& ref) const;
  
  private:
    LogSelection _selection = LogSelection::Invalid;
    uint         _mask = 0;
  };

 private:
  uint _decorators;
  static const char* _name[][2];
  static const uint DefaultDecoratorsMask = (1 << uptime_decorator) | (1 << level_decorator) | (1 << tags_decorator);
  static LogDecorators::DefaultDecorator DefaultDecorators[];

  static uint mask(LogDecorators::Decorator decorator) {
    return 1 << decorator;
  }

  // constexpr LogDecorators(uint mask) : _decorators(mask) {
  // }

 public:
  static const LogDecorators None;
  static const LogDecorators All;

  LogDecorators(uint mask) : _decorators(mask) {
  }

  LogDecorators() : _decorators(DefaultDecoratorsMask) {
  }

  void clear() {
    _decorators = 0;
  }

  static const char* name(LogDecorators::Decorator decorator) {
    return _name[decorator][0];
  }

  static const char* abbreviation(LogDecorators::Decorator decorator) {
    return _name[decorator][1];
  }

  static bool has_default_decorator(LogSelection selection, uint* mask) {
    bool match_level;
    int specificity, max_specificity = 0;
    for (size_t i = 0; DefaultDecorators[i] != DefaultDecorator::Invalid; ++i) {
      match_level = DefaultDecorators[i].selection().level() != LogLevelType::NotMentioned;
      specificity = selection.contains(DefaultDecorators[i].selection(), match_level);
      if (specificity > max_specificity) {
        *mask = DefaultDecorators[i].mask();
        max_specificity = specificity;
      } else if (specificity == max_specificity) {
        *mask |= DefaultDecorators[i].mask();
      }
    }
    return max_specificity > 0;
  }

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
