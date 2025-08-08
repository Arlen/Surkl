/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "EdgeItem.hpp"

#include <QGraphicsItem>
#include <QPersistentModelIndex>

#include <deque>
#include <numbers>
#include <ranges>
#include <unordered_map>
#include <vector>


class QVariantAnimation;
class QSequentialAnimationGroup;

namespace  core
{
    class FileSystemScene;

    enum class Rotation
    {
        CCW, CW ///, NoRotation
    };

    enum NodeType
    {
        OpenNode       = 0x0001,
        ClosedNode     = 0x0002,
        /// half-closed b/c half opening doesn't make sense.
        /// Closing a folder closes the entire subtree.
        HalfClosedNode = 0x0004,
        DirNode        = HalfClosedNode | ClosedNode | OpenNode,
        FileNode       = 0x0010,
        LinkNode       = 0x0020,
    };

    Q_DECLARE_FLAGS(NodeFlags, NodeType)


    /// RootItem is just for visualizes so that the parent InodeEdge of the
    /// actual root Inode does not hang by itself.
    class RootItem final : public QGraphicsEllipseItem
    {
    public:
        enum { Type = UserType + 3 };

        explicit RootItem();
        void setChildEdge(EdgeItem* edge);
        void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

        [[nodiscard]] int type() const override { return Type; }

    protected:
        QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    private:
        EdgeItem* _childEdge{nullptr};
    };


    class KnotItem final : public QGraphicsEllipseItem
    {
    public:
        enum { Type = UserType + 4 };

        explicit KnotItem(QGraphicsItem* parent = nullptr);
        void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

        [[nodiscard]] int type() const override { return Type; }
    };


    using EdgeDeque = std::deque<EdgeItem*>;

    struct InternalRotationAnimationData
    {
        Rotation rot;
        QGraphicsItem* node{nullptr};
        EdgeItem* toGrow{nullptr};
        EdgeItem* toShrink{nullptr};
        float toGrowLength{1};
        float toShrinkLength{1};
        QHash<QGraphicsItem*, qreal> angularDisplacement;
        QHash<QGraphicsItem*, qreal> angles;
    };

    struct SpreadAnimationData
    {
        struct Movement
        {
            QPointF oldPos;
            QPointF newPos;
        };

        QHash<QGraphicsItem*, Movement> movement;
    };

    struct NodeData
    {
        QPersistentModelIndex index;
        NodeType type;
        int firstRow;
        QPointF pos;
        qreal length;
        qreal rotation;
        EdgeItem* edge{nullptr};
    };

    class NodeItem final : public QGraphicsItem
    {
        static constexpr qreal GOLDEN                     = 1.0 / std::numbers::phi;
        static constexpr qreal NODE_OPEN_RADIUS           = 32.0;
        static constexpr qreal NODE_OPEN_DIAMETER         = NODE_OPEN_RADIUS * 2.0;
        //static constexpr qreal NODE_CLOSED_RADIUS       = NODE_OPEN_RADIUS * GOLDEN;
        static constexpr qreal NODE_CLOSED_DIAMETER       = NODE_OPEN_DIAMETER * GOLDEN;
        static constexpr qreal NODE_HALF_CLOSED_DIAMETER  = NODE_OPEN_DIAMETER * (1.0 - GOLDEN*GOLDEN*GOLDEN);
        static constexpr qreal EDGE_WIDTH                 = 4.0;
        static constexpr qreal NODE_OPEN_PEN_WIDTH        = 4.0;
        static constexpr qreal NODE_CLOSED_PEN_WIDTH      = EDGE_WIDTH * GOLDEN;
        static constexpr qreal NODE_HALF_CLOSED_PEN_WIDTH = NODE_OPEN_PEN_WIDTH * (1.0 - GOLDEN*GOLDEN*GOLDEN);

    public:
        static constexpr int NODE_CHILD_COUNT      = 24;  /// fixed for now.
        static constexpr float NODE_MIN_LENGTH     = 128;
        static constexpr float NODE_MAX_LENGTH     = 512;
        static constexpr float NODE_DEFAULT_LENGTH = 150;
        static constexpr float NODE_DEFAULT_EXTENT = NODE_DEFAULT_LENGTH + NODE_OPEN_RADIUS;

        enum
        {
            FileSizeKey = 0,
        };

        enum { Type = UserType + 2 };

        explicit NodeItem();
        ~NodeItem() override;

        [[nodiscard]] static EdgeItem* createNode(const QPersistentModelIndex& targetIndex, QGraphicsItem* source);
        [[nodiscard]] static EdgeItem* createRootNode(const QPersistentModelIndex& index);
        void createChildNodes();
        void createChildNodes(QList<NodeData>& data);

        void onRowsInserted(int start, int end);
        void onRowsRemoved(int start, int end);
        void onRowsAboutToBeRemoved(int start, int end);

        void setIndex(const QPersistentModelIndex& index);
        [[nodiscard]] QString name() const;
        [[nodiscard]] QRectF boundingRect() const override;
        [[nodiscard]] QPainterPath shape() const override;
        [[nodiscard]] bool hasOpenOrHalfClosedChild() const;

        [[nodiscard]] bool isDir() const            { return _nodeFlags.testAnyFlag(DirNode); }
        [[nodiscard]] bool isFile() const           { return _nodeFlags.testAnyFlag(FileNode); }
        [[nodiscard]] bool isClosed() const         { return _nodeFlags.testAnyFlag(ClosedNode); }
        [[nodiscard]] bool isOpen() const           { return _nodeFlags.testAnyFlag(OpenNode); }
        [[nodiscard]] bool isHalfClosed() const     { return _nodeFlags.testAnyFlag(HalfClosedNode); }
        [[nodiscard]] bool isLink() const           { return _nodeFlags.testAnyFlag(LinkNode); }
        [[nodiscard]] NodeFlags nodeFlags() const   { return _nodeFlags; }
        [[nodiscard]] bool hasChildren() const      { return !_childEdges.empty(); }
        [[nodiscard]] int type() const override     { return Type; }
        [[nodiscard]] EdgeItem* parentEdge() const  { return _parentEdge; }
        [[nodiscard]] KnotItem* knot() const        { return _knot; }
        [[nodiscard]] int firstRow() const          { return _firstRow; }
        [[nodiscard]] float length() const          { return _length; }

        [[nodiscard]] const QPersistentModelIndex& index() const { return _index; }
        [[nodiscard]] const EdgeDeque& childEdges() const        { return _childEdges; }

        void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
        void close();
        void halfClose();
        void closeOrHalfClose(bool forceClose = false);
        void open();
        void rotate(Rotation rot);
        void rotatePage(Rotation rot);
        void skipTo(int row);
        void grow(float amount);
        void growChildren(float amount);
        float childLength(const QPersistentModelIndex& index) const;

    protected:
        QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
        void setNodeFlags(NodeFlags flags);

    private:
        FileSystemScene* fsScene() const;
        void destroyChildren();
        void updateFirstRow();

        void repositionAfterClose(EdgeItem* closed);
        InternalRotationAnimationData doInternalRotation(Rotation rot);

        void spread(const QPointF& dxy = QPointF(0, 0));
        void spread(const NodeItem* child);

        void relayoutParent() const;

        NodeFlags _nodeFlags{NodeType::ClosedNode};
        int _firstRow{-1};
        float _length{NODE_DEFAULT_LENGTH};
        QPersistentModelIndex _index;
        EdgeItem* _parentEdge{nullptr};
        KnotItem* _knot{nullptr};
        EdgeItem* _extra{nullptr};
        EdgeDeque _childEdges;
        QHash<QPersistentModelIndex, float> _childLengths;

        inline static std::vector<std::pair<QGraphicsItem*, QPointF>> _ancestorPos;

        friend class Animator;
    };

    using NodeVector = std::vector<NodeItem*>;

    void extend(NodeItem* node, qreal distance = NodeItem::NODE_DEFAULT_EXTENT);
    void shrink(NodeItem* node, qreal distance = NodeItem::NODE_DEFAULT_EXTENT);
    void adjustAllEdges(const NodeItem* node);
    void updateAllChildNodes(const NodeItem* node);
    void setAllEdgeState(const NodeItem* node, EdgeItem::State state);
    SpreadAnimationData spreadWithAnimation(const NodeItem* parent);


    class Animator final : public QObject
    {
    public:
        void animateRotation(NodeItem* node, Rotation rot);
        void animatePageRotation(NodeItem* node, Rotation rot, int page);
        void animateRelayout(NodeItem* node, EdgeItem* closedEdge);
        void clearAnimations(NodeItem* node);

    private:
        void startAnimation(const NodeItem* node);
        void addRotation(NodeItem* node, const Rotation& rot, QVariantAnimation* va);
        void addRelayout(NodeItem* node, EdgeItem* closedEdge, QVariantAnimation* va);

        void startRotation(NodeItem* node, Rotation rot, QVariantAnimation* va);
        void startRelayout(NodeItem* node, EdgeItem* closedEdge, QVariantAnimation* va);

        QSequentialAnimationGroup* getSeq(const NodeItem* node);
        void clearSequence(const NodeItem* node);
        QVariantAnimation* createVariantAnimation(int duration);

        static void fastforward(const QSequentialAnimationGroup* seq);
        static void interpolate(qreal t, const InternalRotationAnimationData& data);

        std::unordered_map<const NodeItem*, QSequentialAnimationGroup*> _seqs;
        std::unordered_map<const QVariantAnimation*, QVariant> _animData;
    };


    inline NodeItem* asNodeItem(QGraphicsItem* item)
    {
        Q_ASSERT(item != nullptr);

        return qgraphicsitem_cast<NodeItem*>(item);
    }

    inline auto isFileOrClosed = [](const EdgeItem* item) -> bool
    {
        const auto* node = asNodeItem(item->target());

        return node->isFile() || node->isClosed();
    };

    inline auto asTargetNode
        = std::views::transform(&EdgeItem::target)
        | std::views::transform(&asNodeItem)
        ;

    inline auto asFilesOrClosedTargetNodes
        = asTargetNode
        | std::views::filter([](const NodeItem* node)
            { return node->isFile() || node->isClosed(); })
        ;

    inline auto asNotClosedTargetNodes
        = asTargetNode
        | std::views::filter(&NodeItem::isDir)
        | std::views::filter(std::not_fn(&NodeItem::isClosed))
        ;

    inline auto asFilesOrClosedEdges
        = asFilesOrClosedTargetNodes
        | std::views::transform(&NodeItem::parentEdge)
        ;

    inline auto asIndex = std::views::transform(&NodeItem::index);
    inline auto asIndexRow
        = asIndex
        | std::views::transform(&QPersistentModelIndex::row)
        ;

    inline auto asTargetNodeIndex
        = asTargetNode
        | asIndex
        ;
}
