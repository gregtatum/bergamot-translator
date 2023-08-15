/*
 * Bindings for Response class
 *
 */

#include <emscripten/bind.h>

#include <vector>

#include "response.h"
#include "wrappers.h"

using Response = marian::bergamot::Response;
using ByteRange = marian::bergamot::ByteRange;
using ResponseWrapper = marian::bergamot::ResponseWrapper;

using namespace emscripten;


// Binding code
EMSCRIPTEN_BINDINGS(byte_range) {
  value_object<ByteRange>("ByteRange").field("begin", &ByteRange::begin).field("end", &ByteRange::end);
}

EMSCRIPTEN_BINDINGS(response) {
  class_<ResponseWrapper>("ResponseWrapper")
      .smart_ptr_constructor("ResponseWrapper", &std::make_shared<ResponseWrapper>)
      .function("size", &ResponseWrapper::size)
      .function("getOriginalText", &ResponseWrapper::getOriginalText)
      .function("getTranslatedText", &ResponseWrapper::getTranslatedText)
      .function("getSourceSentence", &ResponseWrapper::getSourceSentenceAsByteRange)
      .function("getTranslatedSentence", &ResponseWrapper::getTargetSentenceAsByteRange)
      .function("getAlignments", &ResponseWrapper::getAlignments);

  register_vector<ResponseWrapper>("VectorResponseWrapper");
}
