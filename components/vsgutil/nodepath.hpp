#ifndef VSGOPENMW_VSGUTIL_NODEPATH_H
#define VSGOPENMW_VSGUTIL_NODEPATH_H

#include <algorithm>
#include <cassert>

/*
 * Operates on node paths typically accumulated during visitor traversal.
 */
namespace vsgUtil
{
    /*
     * Conveniently adds node to node path for the duration of the current scope.
     */
    template <class NodePath>
    struct PushPopNode
    {
        NodePath& nodePath;
        template <class Node>
        PushPopNode(NodePath& np, Node node) : nodePath(np) { nodePath.emplace_back(node); }
        ~PushPopNode() { nodePath.pop_back(); }
    };

    template <class NodePath, class Node, class Visitor>
    void accumulateAndTraverse(NodePath& path, Node& node, Visitor& visitor)
    {
        PushPopNode ppn(path, &node);
        node.traverse(visitor);
    }

    template <class Node>
    struct AccumulatePath
    {
        std::vector<Node> path;
        template <class N>
        PushPopNode<std::vector<Node>> pushPop(N node)
        {
            return PushPopNode(path, node);
        }
        template <class N, class Visitor>
        void accumulateAndTraverse(N& node, Visitor& v)
        {
            vsgUtil::accumulateAndTraverse(path, node, v);
        }
    };

    /*
     * Traverses until path is found.
     */
    template <class Node>
    struct SearchPath : public AccumulatePath<Node>
    {
        std::vector<Node> foundPath;

        template <class N, class Visitor>
        void traverseNode(N& n, Visitor& v)
        {
            if (this->foundPath.empty())
                n.traverse(v);
        }

        template <class N, class Visitor>
        void accumulateAndTraverse(N& node, Visitor& v)
        {
            if (this->foundPath.empty())
                vsgUtil::accumulateAndTraverse(this->path, node, v);
        }

        template <class N, class Visitor>
        void accumulateCastAndTraverse(N& node, Visitor& v)
        {
            if (this->foundPath.empty())
            {
                if (auto derived = dynamic_cast<Node>(&node))
                {
                    auto ppn = this->pushPop(derived);
                    traverseNode(node, v);
                }
                else
                    traverseNode(node, v);
            }
        }
    };

    /*
     * Extracts particular type of node.
     */
    template <class Node, class Src>
    std::vector<Node*> extract(const std::vector<Src>& src)
    {
        std::vector<Node*> ret;
        for (auto& node : src)
            if (auto derived = dynamic_cast<Node*>(&*node))
                ret.emplace_back(derived);
        return ret;
    }

    /*
     * Concatenates paths.
     */
    template<class Dst, class Src>
    void addPath(std::vector<Dst>& dst, const std::vector<Src>& src)
    {
        for (auto& node : src)
            dst.emplace_back(node);
    }

    /*
     * Conveniently adapts node qualifiers.
     */
    template<class Dst, class Src>
    std::vector<Dst> path(const std::vector<Src>& src)
    {
        std::vector<Dst> dst;
        addPath(dst, src);
        return dst;
    }

    /*
     * Removes leading n nodes.
     */
    template <class Path>
    void trim(Path& p, size_t n)
    {
        assert(p.size() >= n);
        Path newPath;
        newPath.reserve(p.size() - n);
        newPath.insert(newPath.end(), p.begin() + n, p.end());
        newPath.swap(p);
    }

    /*
     * Trims paths to their lowest common ancestor.
     */
    template <class Path>
    void trim(Path& p1, Path& p2)
    {
        size_t i = 0;
        for (; i < std::min(p1.size(), p2.size()); ++i)
        {
            if (p1[i] != p2[i])
                break;
        }
        trim(p1, i);
        trim(p2, i);
    }
}

#endif
