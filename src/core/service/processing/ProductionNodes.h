// =============================================================================
//  ProductionNodes.h
//  作者：工程师 ly（自动生成）
//
//  基于甲方节点规格文档生成的真实节点注册。
//  所有节点通过 ExternalProcessNode 调用后端算法 EXE。
// =============================================================================
#ifndef PRODUCTION_NODES_H
#define PRODUCTION_NODES_H

#include "NodeRegistry.h"

namespace processing {
namespace production {

/**
 * @brief 注册所有真实算法节点到 NodeRegistry。
 * 在 ProcessingPage 初始化时调用一次。
 */
void registerProductionNodes();

} // namespace production
} // namespace processing

#endif // PRODUCTION_NODES_H