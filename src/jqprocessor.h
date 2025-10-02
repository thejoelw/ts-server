#pragma once

#include <string_view>

#include <jq.h>

#include "badrequestexception.h"

class JqProcessor {
public:
  JqProcessor(const std::string_view query) {
    jq = jq_init();
    if (jq == nullptr) {
      throw BadRequestException("JQ init failed");
    }

    if (jq_compile(jq, query.data()) == 0) {
      jq_teardown(&jq);
      jq = nullptr;
      throw BadRequestException("JQ compile failed");
    }
  }

  ~JqProcessor() { jq_teardown(&jq); }

  template <typename CallbackType>
  void process(const std::string_view input, CallbackType callback) {
    jv parsed_input = jv_parse_sized(input.data(), input.size());
    if (!jv_is_valid(parsed_input)) {
      jv_free(parsed_input);
      return;
    }

    jq_start(jq, parsed_input, 0);

    jv result;
    while (jv_is_valid(result = jq_next(jq))) {
      jv str = jv_dump_string(result, 0);
      str = jv_string_append_buf(str, "\n", 1);
      callback(std::string_view(jv_string_value(str), jv_string_length_bytes(jv_copy(str))));
      jv_free(str);
    }

    jv_free(result);
    jv_free(parsed_input);
  }

private:
  jq_state *jq;
};