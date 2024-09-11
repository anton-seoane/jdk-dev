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
#include "precompiled.hpp"
#include "logging/logDecorators.hpp"
#include "runtime/os.hpp"
#include "logDecorators.hpp"

const LogLevelType AnyLevel = LogLevelType::NotMentioned;
#define DEFAULT_DECORATORS \
  DEFAULT_VALUE(mask_from_decorators(NoDecorators), Trace, LOG_TAGS(jit))

template <LogDecorators::Decorator d>
struct AllBitmask {
  // Use recursive template deduction to calculate the bitmask of all decorations.
  static const uint _value = (1 << d) | AllBitmask<static_cast<LogDecorators::Decorator>(d + 1)>::_value;
};

template<>
struct AllBitmask<LogDecorators::Count> {
  static const uint _value = 0;
};

const LogDecorators LogDecorators::None = {0};
const LogDecorators LogDecorators::All = {AllBitmask<time_decorator>::_value};

const char* LogDecorators::_name[][2] = {
#define DECORATOR(n, a) {#n, #a},
  DECORATOR_LIST
#undef DECORATOR
};

const LogDecorators::DefaultDecorator LogDecorators::default_decorators[] = {
#define DEFAULT_VALUE(mask, level, ...) LogDecorators::DefaultDecorator(LogLevel::level, mask, __VA_ARGS__),
  DEFAULT_DECORATORS
#undef DEFAULT_VALUE
};
const size_t LogDecorators::number_of_default_decorators = sizeof(default_decorators) / sizeof(LogDecorators::DefaultDecorator);

LogDecorators::Decorator LogDecorators::from_string(const char* str) {
  for (size_t i = 0; i < Count; i++) {
    Decorator d = static_cast<Decorator>(i);
    if (strcasecmp(str, name(d)) == 0 || strcasecmp(str, abbreviation(d)) == 0) {
      return d;
    }
  }
  return Invalid;
}

bool LogDecorators::parse(const char* decorator_args, outputStream* errstream) {
  if (decorator_args == nullptr || strlen(decorator_args) == 0) {
    // No decorators supplied, keep default decorators
    return true;
  }

  if (strcasecmp(decorator_args, "none") == 0 ) {
    _decorators = 0;
    return true;
  }

  bool result = true;
  uint tmp_decorators = 0;
  char* args_copy = os::strdup_check_oom(decorator_args, mtLogging);
  char* token = args_copy;
  char* comma_pos;
  do {
    comma_pos = strchr(token, ',');
    if (comma_pos != nullptr) {
      *comma_pos = '\0';
    }
    Decorator d = from_string(token);
    if (d == Invalid) {
      if (errstream != nullptr) {
        errstream->print_cr("Invalid decorator '%s'.", token);
      }
      result = false;
      break;
    }
    tmp_decorators |= mask(d);
    token = comma_pos + 1;
  } while (comma_pos != nullptr);
  os::free(args_copy);
  if (result) {
    _decorators = tmp_decorators;
  }
  return result;
}

bool LogDecorators::has_default_decorator(const LogSelection& selection, uint* mask, const DefaultDecorator* defaults) {
  size_t max_specificity = 0;
  for (size_t i = 0; i < number_of_default_decorators; ++i) {
    auto current_default = defaults[i];
    const bool ignore_level = current_default.selection().level() == AnyLevel;
    const bool level_matches = ignore_level || selection.level() == current_default.selection().level();
    if (!level_matches) continue;
    if (!selection.superset_of(current_default.selection())) {
      continue;
    }
    size_t specificity = current_default.selection().ntags();
    if (specificity > max_specificity) {
      *mask = current_default.mask();
      max_specificity = specificity;
    } else if (specificity == max_specificity) {
      *mask |= current_default.mask();
    }
  }
  return max_specificity > 0;
}
