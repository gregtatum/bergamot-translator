#pragma once
#include <emscripten/bind.h>
#include <emscripten/val.h>

#include "service.h"
#include "response.h"

using namespace emscripten;

namespace marian::bergamot {

/// A class to wrap a Response so that JS objects can be created for the bindings.
class ResponseWrapper {
public:
  ResponseWrapper() : response_() {}

  /// Implicitly wrap a Response.
  ResponseWrapper(Response &&response) : response_(std::move(response)) {}

  /// Returns the source sentence (in terms of byte range) corresponding to sentenceIdx.
  ///
  /// @param [in] sentenceIdx: The index representing the sentence where 0 <= sentenceIdx < Response::size()
  ByteRange getSourceSentenceAsByteRange(size_t sentenceIdx) const { return response_.source.sentenceAsByteRange(sentenceIdx); }

  /// Returns the translated sentence (in terms of byte range) corresponding to sentenceIdx.
  ///
  /// @param [in] sentenceIdx: The index representing the sentence where 0 <= sentenceIdx < Response::size()
  ByteRange getTargetSentenceAsByteRange(size_t sentenceIdx) const { return response_.target.sentenceAsByteRange(sentenceIdx); }

  const std::string &getOriginalText() const { return response_.source.text; }

  const std::string &getTranslatedText() const { return response_.target.text; }

  const size_t size() const { return response_.source.numSentences(); }

private:

  /// Converts a sentence into a JavaScript `Array<string>` where each string is a token
  /// in the sentence.
  static val getTokensForSentence(AnnotatedText text, size_t sentenceIdx) {
    size_t numWords = text.numWords(sentenceIdx);
    val tokensJs = val::array();
    for (size_t wordIdx = 0; wordIdx < numWords; wordIdx++) {
      auto word = text.word(sentenceIdx, wordIdx);
      std::string token {word.begin(), word.end()};
      tokensJs.call<void>("push", token);
    }
    return tokensJs;
  }

  /// Returns the a JavaScript array containing the alignments for a sentence.
  /// `Array<number[]>`. The outer array is the length of the target translation, and
  /// the inner array is the length of the source translation. This represents the
  /// matrix of weights for how well a source token maps to a target token.
  val getAlignmentsForSentence(size_t sentenceIdx) const {
    val tokensJs = val::array();
    auto sentence = response_.alignments[sentenceIdx];
    for (const auto alignment : sentence) {
      val alignmentJs = val::array();
      for (const auto &value : alignment) {
        alignmentJs.call<void>("push", value);
      }
      tokensJs.call<void>("push", alignmentJs);
    }
    return tokensJs;
  }

public:

  /// Return the alignments for a translation.
  /// `Array<{ alignments: Array<number[]>, source: string[], target[] }`
  val getAlignments() const {
    val result = val::array();
    size_t numSentences = response_.source.numSentences();

    for (size_t sentenceIdx = 0; sentenceIdx < numSentences; sentenceIdx++) {
      val annotations = val::object();
      annotations.set("source", ResponseWrapper::getTokensForSentence(response_.source, sentenceIdx));
      annotations.set("target", ResponseWrapper::getTokensForSentence(response_.target, sentenceIdx));
      annotations.set("alignments", getAlignmentsForSentence(sentenceIdx));
      result.call<void>("push", annotations);
    }

    return result;
  }

private:

  Response response_;
};


class BlockingServiceWrapper {
public:
  BlockingServiceWrapper(const BlockingService::Config& config) : _blockingService(config) {}

  std::vector<ResponseWrapper> translate(std::shared_ptr<TranslationModel> translationModel,
                std::vector<std::string> &&sources,
                const std::vector<ResponseOptions> &responseOptions
                ) {
    std::vector<Response> responses = _blockingService.translateMultiple(translationModel, std::move(sources), responseOptions);
    std::vector<ResponseWrapper> wrappedResponses{};
    for (Response& response : responses) {
      wrappedResponses.emplace_back(std::move(response));
    }
    return wrappedResponses;
  }

  std::vector<ResponseWrapper> translateViaPivoting(std::shared_ptr<TranslationModel> first,
                                                    std::shared_ptr<TranslationModel> second,
                                                    std::vector<std::string> &&sources,
                                                    const std::vector<ResponseOptions> &responseOptions) {
    std::vector<Response> responses = _blockingService.pivotMultiple(first, second, std::move(sources), responseOptions);
    std::vector<ResponseWrapper> wrappedResponses{};
    for (Response& response : responses) {
      wrappedResponses.emplace_back(std::move(response));
    }
    return wrappedResponses;
  }

private:
  BlockingService _blockingService;
};

}
