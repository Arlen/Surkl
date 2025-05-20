/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "EdgeItem.hpp"

#include <QGraphicsItem>
#include <QPersistentModelIndex>

#include <deque>
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


    using EdgeVector = std::deque<EdgeItem*>;


    class NodeItem final : public QGraphicsItem
    {
    public:
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
        [[nodiscard]] const EdgeVector& childEdges() const       { return _childEdges; }

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
        void doInternalRotation(Rotation rot);
        void skipTo(int row);
        void doSkipTo(int row);

        void spread(QPointF dxy = QPointF(0,0));

        FolderState _state{FolderState::Closed};
        QPersistentModelIndex _index;
        EdgeItem* _parentEdge{nullptr};
        EdgeVector _childEdges;
        KnotItem* _knot{nullptr};

        inline static std::vector<std::pair<QGraphicsItem*, QPointF>> _ancestorPos;
    };

    using NodeVector = std::vector<NodeItem*>;

    void extend(NodeItem* node, qreal distance = 144.0);
    void shrink(NodeItem* node, qreal distance = 144.0);
    void adjustAllEdges(const NodeItem* node);
    void updateAllChildNodes(const NodeItem* node);
    void setAllEdgeState(const NodeItem* node, EdgeItem::State state);
}
