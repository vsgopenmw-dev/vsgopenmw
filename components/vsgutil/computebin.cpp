#include "computebin.hpp"

namespace vsgUtil
{
    void ComputeBin::traverse(vsg::RecordTraversal& visitor) const
    {
        if (_elements.empty())
            return;

        // adapter class used to efficiently traverse Bin elements within vkBeginCommandBuffer/vkEndCommandBuffer pair without creating ref_ptr<> copies of the Bin elements.
        class TraverseAdapter : public vsg::Inherit<vsg::Node, TraverseAdapter>
        {
        public:
            const std::vector<Element>& elements;
            TraverseAdapter(const std::vector<vsg::Bin::Element>& in_elements) : elements(in_elements) {}
            void traverse(vsg::RecordTraversal& visitor) const override
            {
                for (auto& element : elements)
                {
                    element.child->accept(visitor);
                }
            }
        };

        auto adapter = TraverseAdapter::create(_elements);
        commandGraph->children = { adapter };
        commandGraph->accept(visitor);
    }
}
