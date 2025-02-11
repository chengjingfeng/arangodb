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

#ifndef ARANGOD_AQL_ENUMERATECOLLECTION_EXECUTOR_H
#define ARANGOD_AQL_ENUMERATECOLLECTION_EXECUTOR_H

#include "Aql/ExecutionEngine.h"
#include "Aql/ExecutionState.h"
#include "Aql/ExecutorInfos.h"
#include "Aql/OutputAqlItemRow.h"
#include "Aql/Stats.h"
#include "Aql/types.h"
#include "DocumentProducingHelper.h"
#include "Utils/OperationCursor.h"

#include <memory>

namespace arangodb {
namespace aql {

class InputAqlItemRow;
class ExecutorInfos;

template <bool>
class SingleRowFetcher;

class EnumerateCollectionExecutorInfos : public ExecutorInfos {
 public:
  EnumerateCollectionExecutorInfos(
      RegisterId outputRegister, RegisterId nrInputRegisters,
      RegisterId nrOutputRegisters, std::unordered_set<RegisterId> registersToClear,
      std::unordered_set<RegisterId> registersToKeep, ExecutionEngine* engine,
      Collection const* collection, Variable const* outVariable, bool produceResult,
      std::vector<std::string> const& projections, transaction::Methods* trxPtr,
      std::vector<size_t> const& coveringIndexAttributePositions,
      bool useRawDocumentPointers, bool random);

  EnumerateCollectionExecutorInfos() = delete;
  EnumerateCollectionExecutorInfos(EnumerateCollectionExecutorInfos&&) = default;
  EnumerateCollectionExecutorInfos(EnumerateCollectionExecutorInfos const&) = delete;
  ~EnumerateCollectionExecutorInfos() = default;

  ExecutionEngine* getEngine() { return _engine; };
  Collection const* getCollection() const { return _collection; }
  Variable const* getOutVariable() const { return _outVariable; }
  std::vector<std::string> const& getProjections() const noexcept { return _projections; }
  transaction::Methods* getTrxPtr() const { return _trxPtr; }
  std::vector<size_t> const& getCoveringIndexAttributePositions() const noexcept {
    return _coveringIndexAttributePositions;
  }
  bool getProduceResult() const { return _produceResult; }
  bool getUseRawDocumentPointers() const { return _useRawDocumentPointers; }
  bool getRandom() const { return _random; }
  RegisterId getOutputRegisterId() const { return _outputRegisterId; }

 private:
  ExecutionEngine* _engine;
  Collection const* _collection;
  Variable const* _outVariable;
  transaction::Methods* _trxPtr;
  std::vector<std::string> const& _projections;
  std::vector<size_t> const& _coveringIndexAttributePositions;
  RegisterId _outputRegisterId;
  bool _useRawDocumentPointers;
  bool _produceResult;
  bool _random;
};

/**
 * @brief Implementation of EnumerateCollection Node
 */
class EnumerateCollectionExecutor {
 public:
  struct Properties {
    static const bool preservesOrder = true;
    static const bool allowsBlockPassthrough = false;
    /* With some more modifications this could be turned to true. Actually the
   output of this block is input * itemsInCollection */
    static const bool inputSizeRestrictsOutputSize = false;
  };
  using Fetcher = SingleRowFetcher<Properties::allowsBlockPassthrough>;
  using Infos = EnumerateCollectionExecutorInfos;
  using Stats = EnumerateCollectionStats;

  EnumerateCollectionExecutor() = delete;
  EnumerateCollectionExecutor(EnumerateCollectionExecutor&&) = default;
  EnumerateCollectionExecutor(EnumerateCollectionExecutor const&) = delete;
  EnumerateCollectionExecutor(Fetcher& fetcher, Infos&);
  ~EnumerateCollectionExecutor();

  /**
   * @brief produce the next Row of Aql Values.
   *
   * @return ExecutionState, and if successful exactly one new Row of AqlItems.
   */

  std::pair<ExecutionState, Stats> produceRows(OutputAqlItemRow& output);
  std::tuple<ExecutionState, EnumerateCollectionStats, size_t> skipRows(size_t atMost);

  void setProducingFunction(DocumentProducingFunction const& documentProducer) {
    _documentProducer = documentProducer;
  }

  void initializeCursor();

 private:
  bool waitForSatellites(ExecutionEngine* engine, Collection const* collection) const;

  void setAllowCoveringIndexOptimization(bool const allowCoveringIndexOptimization) {
    _documentProducingFunctionContext.setAllowCoveringIndexOptimization(allowCoveringIndexOptimization);
  }

  /// @brief whether or not we are allowed to use the covering index
  /// optimization in a callback
  bool getAllowCoveringIndexOptimization() const noexcept {
    return _documentProducingFunctionContext.getAllowCoveringIndexOptimization();
  }

 private:
  Infos& _infos;
  Fetcher& _fetcher;
  DocumentProducingFunction _documentProducer;
  DocumentProducingFunctionContext _documentProducingFunctionContext;
  ExecutionState _state;
  bool _cursorHasMore;
  InputAqlItemRow _input;
  std::unique_ptr<OperationCursor> _cursor;
};

}  // namespace aql
}  // namespace arangodb

#endif
