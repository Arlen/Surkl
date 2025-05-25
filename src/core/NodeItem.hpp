/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "EdgeItem.hpp"

#include <QGraphicsItem>
#include <QPersistentModelIndex>

#include <deque>
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

    enum class FolderState
    {
        Open,
        Closed,
        /// half-closed b/c half opening doesn't make sense.
        /// Closing a folder closes the entire subtree.
        HalfClosed
    };


    /// RootItem is just for visualizes so that the parent InodeEdge of the
    /// actual root Inode does not hang by itself.
    class RootItem final : public QGraphicsEllipseItem
    {
    public:
        enum { Type = UserType + 3 };

        explicit RootItem(QGraphicsItem* parent = nullptr);
        void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

        [[nodiscard]] int type() const override { return Type; }

    protected:
        QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
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

    struct InternalRotation
    {
        Rotation rot;
        QGraphicsItem* node{nullptr};
        EdgeItem* toGrow{nullptr};
        EdgeItem* toShrink{nullptr};
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

    class NodeItem final : public QGraphicsItem
    {
    public:
        /// fixed for now.
        static constexpr int NODE_CHILD_COUNT = 6;

        enum { Type = UserType + 2 };

        explicit NodeItem(const QPersistentModelIndex& index);
        [[nodiscard]] static NodeItem* createRootItem(core::FileSystemScene* scene);
        [[nodiscard]] static NodeItem* createNode(QGraphicsScene* scene, QGraphicsItem* parent);

        ~NodeItem() override;
        void init();
        void reload(int start, int end);
        void unload(int start, int end);

        void setIndex(const QPersistentModelIndex& index);
        [[nodiscard]] QString name() const;
        [[nodiscard]] QRectF boundingRect() const override;
        [[nodiscard]] QPainterPath shape() const override;
        [[nodiscard]] bool hasOpenOrHalfClosedChild() const;
        [[nodiscard]] bool isDir() const;

        [[nodiscard]] bool isClosed() const         { return _state == FolderState::Closed; }
        [[nodiscard]] bool isOpen() const           { return _state == FolderState::Open; }
        [[nodiscard]] bool isHalfClosed() const     { return _state == FolderState::HalfClosed; }
        [[nodiscard]] bool hasChildren() const      { return !_childEdges.empty(); }
        [[nodiscard]] int type() const override     { return Type; }
        [[nodiscard]] EdgeItem* parentEdge() const  { return _parentEdge; }
        [[nodiscard]] KnotItem* knot() const        { return _knot; }

        [[nodiscard]] const QPersistentModelIndex& index() const { return _index; }
        [[nodiscard]] const EdgeDeque& childEdges() const        { return _childEdges; }

        void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
        void close();
        void halfClose();
        void closeOrHalfClose(bool forceClose = false);
        void open();
        void rotate(Rotation rot);
        void rotatePage(Rotation rot);

    protected:
        QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
        void keyPressEvent(QKeyEvent *event) override;
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
        void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
        void setState(FolderState state);

    private:
        FileSystemScene* fsScene() const;
        void destroyChildren();

        void internalRotationAfterClose(EdgeItem* closedEdge);
        void doInternalRotationAfterClose(EdgeItem* closedEdge);
        InternalRotation doInternalRotation(Rotation rot);
        void skipTo(int row);

        void spread(QPointF dxy = QPointF(0,0));

        FolderState _state{FolderState::Closed};
        QPersistentModelIndex _index;
        EdgeItem* _parentEdge{nullptr};
        EdgeDeque _childEdges;
        KnotItem* _knot{nullptr};
        EdgeItem* _extra{nullptr};

        inline static std::vector<std::pair<QGraphicsItem*, QPointF>> _ancestorPos;

        friend class Animator;
    };

    using NodeVector = std::vector<NodeItem*>;

    void extend(NodeItem* node, qreal distance = 144.0);
    void shrink(NodeItem* node, qreal distance = 144.0);
    void adjustAllEdges(const NodeItem* node);
    void updateAllChildNodes(const NodeItem* node);
    void setAllEdgeState(const NodeItem* node, EdgeItem::State state);
    auto spreadWithAnimation(const NodeItem* node);


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
        static void interpolate(qreal t, const InternalRotation& data);

        std::unordered_map<const NodeItem*, QSequentialAnimationGroup*> _seqs;
        std::unordered_map<const QVariantAnimation*, QVariant> _varData;
    };


    inline NodeItem* asNodeItem(QGraphicsItem* item)
    {
        Q_ASSERT(item != nullptr);

        return qgraphicsitem_cast<NodeItem*>(item);
    }

    inline auto isFileOrClosed = [](const EdgeItem* item) -> bool
    {
        const auto* node = asNodeItem(item->target());

        return !node->isDir() || node->isClosed();
    };

    inline auto asTargetNodes
        = std::views::transform(&EdgeItem::target)
        | std::views::transform(&asNodeItem)
        ;

    inline auto asFilesOrClosedTargetNodes
        = asTargetNodes
        | std::views::filter([](const NodeItem* node)
            { return !node->isDir() || node->isClosed(); })
        ;

    inline auto asNotClosedTargetNodes
        = asTargetNodes
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
        = asTargetNodes
        | asIndex
        ;
}
