#include "DemoNodes.h"
#include "NodeRegistry.h"

namespace processing
{
    namespace demo
    {
        // Demo节点已移除，所有节点由ProductionNodes.cpp统一注册
        // 保留空实现以保证向后兼容
        void registerDemoNodes()
        {
            // 所有真实节点已在 ProductionNodes.cpp 中注册
            // 此函数保留空实现，避免修改调用点
        }
    }
} // namespace processing::demo
