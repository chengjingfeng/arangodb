////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2018 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Tobias Goedderz
/// @author Michael Hackstein
/// @author Heiko Kernbach
/// @author Jan Christoph Uhde
////////////////////////////////////////////////////////////////////////////////

#ifndef ARANGOD_AQL_LIMIT_EXECUTOR_H
#define ARANGOD_AQL_LIMIT_EXECUTOR_H

#include "Aql/ExecutionBlock.h"
#include "Aql/ExecutionBlockImpl.h"
#include "Aql/ExecutionState.h"
#include "Aql/ExecutorInfos.h"
#include "Aql/LimitStats.h"
#include "Aql/OutputAqlItemRow.h"
#include "Aql/types.h"

#include <iosfwd>
#include <memory>

namespace arangodb {
namespace aql {

class InputAqlItemRow;
class ExecutorInfos;
template <bool>
class SingleRowFetcher;

class LimitExecutorInfos : public ExecutorInfos {
 public:
  LimitExecutorInfos(RegisterId nrInputRegisters, RegisterId nrOutputRegisters,
                     std::unordered_set<RegisterId> registersToClear,
                     std::unordered_set<RegisterId> registersToKeep,
                     size_t offset, size_t limit, bool fullCount);

  LimitExecutorInfos() = delete;
  LimitExecutorInfos(LimitExecutorInfos&&) = default;
  LimitExecutorInfos(LimitExecutorInfos const&) = delete;
  ~LimitExecutorInfos() = default;

  size_t getOffset() const noexcept { return _offset; };
  size_t getLimit() const noexcept { return _limit; };
  size_t getLimitPlusOffset() const noexcept { return _offset + _limit; };
  bool isFullCountEnabled() const noexcept { return _fullCount; };

 private:
  /// @brief the remaining offset
  size_t const _offset;

  /// @brief the limit
  size_t const _limit;

  /// @brief whether or not the node should fully count what it limits
  bool const _fullCount;
};

/**
 * @brief Implementation of Limit Node
 */

class LimitExecutor {
 public:
  struct Properties {
    static const bool preservesOrder = true;
    static const bool allowsBlockPassthrough = true;
    static const bool inputSizeRestrictsOutputSize = false;
  };
  using Fetcher = SingleRowFetcher<Properties::allowsBlockPassthrough>;
  using Infos = LimitExecutorInfos;
  using Stats = LimitStats;

  LimitExecutor() = delete;
  LimitExecutor(LimitExecutor&&) = default;
  LimitExecutor(LimitExecutor const&) = delete;
  LimitExecutor(Fetcher& fetcher, Infos&);
  ~LimitExecutor();

  /**
   * @brief produce the next Row of Aql Values.
   *
   * @return ExecutionState, and if successful exactly one new Row of AqlItems.
   */
  std::pair<ExecutionState, Stats> produceRows(OutputAqlItemRow& output);

  /**
   * @brief Custom skipRows() implementation. This is obligatory to increase
   * _counter!
   *
   * Semantically, we first skip until our local offset. We may not report the
   * number of rows skipped this way. Second, we skip up to the number of rows
   * requested; but at most up to our limit.
   */
  std::tuple<ExecutionState, Stats, size_t> skipRows(size_t toSkipRequested);

  std::tuple<ExecutionState, LimitStats, SharedAqlItemBlockPtr> fetchBlockForPassthrough(size_t atMost);

 private:
  Infos const& infos() const noexcept { return _infos; };

  size_t maxRowsLeftToFetch() const noexcept {
    // counter should never exceed this count!
    TRI_ASSERT(infos().getLimitPlusOffset() >= _counter);
    return infos().getLimitPlusOffset() - _counter;
  }

  size_t maxRowsLeftToSkip() const noexcept {
    // should not be called after skipping the offset!
    TRI_ASSERT(infos().getOffset() >= _counter);
    return infos().getOffset() - _counter;
  }

  enum class LimitState {
    // state is SKIPPING until the offset is reached
    SKIPPING,
    // state is RETURNING until the limit is reached
    RETURNING,
    // state is RETURNING_LAST_ROW if we've seen the second to last row before
    // the limit is reached
    RETURNING_LAST_ROW,
    // state is COUNTING when the limit is reached and fullcount is enabled
    COUNTING,
    // state is LIMIT_REACHED only if fullCount is disabled, and we've seen all
    // rows up to limit
    LIMIT_REACHED,
  };

  /**
   * @brief Returns the current state of the executor, based on _counter (i.e.
   * number of lines seen), limit, offset and fullCount.
   * @return See LimitState comments for a description.
   */
  LimitState currentState() const noexcept {
    // Note that not only offset, but also limit can be zero. Thus the order
    // of all following checks is important, even the first two!

    if (_counter < infos().getOffset()) {
      return LimitState::SKIPPING;
    }
    if (_counter + 1 == infos().getLimitPlusOffset()) {
      return LimitState::RETURNING_LAST_ROW;
    }
    if (_counter < infos().getLimitPlusOffset()) {
      return LimitState::RETURNING;
    }
    if (infos().isFullCountEnabled()) {
      return LimitState::COUNTING;
    }

    return LimitState::LIMIT_REACHED;
  }

  std::pair<ExecutionState, Stats> skipOffset();
  std::pair<ExecutionState, Stats> skipRestForFullCount();

 private:
  Infos const& _infos;
  Fetcher& _fetcher;
  InputAqlItemRow _lastRowToOutput;
  ExecutionState _stateOfLastRowToOutput;
  // Number of input lines seen
  size_t _counter = 0;
};

}  // namespace aql
}  // namespace arangodb

#endif
