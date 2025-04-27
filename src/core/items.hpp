#pragma once

#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <QPersistentModelIndex>

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

    enum class LabelFade
    {
        FadeIn, FadeOut
    };

    enum class FolderState
    {
        Open,
        Closed,
        /// half-closed b/c half opening doesn't make sense.
        /// Closing a folder closes the entire subtree.
        HalfClosed
    };

    enum class InternalRotationStatus
    {
        EndReachedCW, EndReachedCCW,

        /// There is _winSize or fewer items in the list OR all folders
        /// are open; therefore, movement is not possible.
        MovementImpossible,

        Normal,
    };

    enum class Order
    {
        Increasing, Decreasing
    };

    inline QString to_string(const InternalRotationStatus status)
    {
        switch (status) {
            case InternalRotationStatus::EndReachedCW: return "EndReachedCW";
            case InternalRotationStatus::EndReachedCCW: return "EndReachedCCW";
            case InternalRotationStatus::MovementImpossible: return "MovementImpossible";
            case InternalRotationStatus::Normal: return "Normal";
        }
        return "Unknown";
    }


    class EdgeLabel final : public QGraphicsSimpleTextItem
    {
    public:
        explicit EdgeLabel(QGraphicsItem* parent = nullptr);
        void alignToAxis(const QLineF& line);
        void alignToAxis(const QLineF& line, const QString& newText);
        void updatePos();
        void updatePosCW(qreal t, LabelFade fade);
        void updatePosCCW(qreal t, LabelFade fade);

        [[nodiscard]] const QLineF& normal() const { return _normal; }

    private:
        void setGradient(const QPointF& a, const QPointF& b, LabelFade fade, qreal t01);

        QLineF _normal;
        QLineF _axis;
        QString _text;
        qreal _t{0};
    };


    class Edge final : public QGraphicsLineItem
    {
    public:
        enum { Type = UserType + 1 };
        enum State { ActiveState, CollapsedState, InactiveState };

        explicit Edge(QGraphicsItem* source = nullptr, QGraphicsItem* target = nullptr);
        void setName(const QString& name) const;
        void adjust();
        void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
        void setState(State state);

        [[nodiscard]] State state() const { return _state; }
        [[nodiscard]] QGraphicsItem *source() const { return _source; }
        [[nodiscard]] QGraphicsItem *target() const { return _target; }
        [[nodiscard]] EdgeLabel* currLabel() const { return _currLabel; }
        [[nodiscard]] EdgeLabel* nextLabel() const { return _nextLabel; }
        [[nodiscard]] QLineF lineWithMargin() const { return _lineWithMargin; }
        [[nodiscard]] QPainterPath shape() const override;
        [[nodiscard]] int type() const override { return Type; }
        void swapLabels() { std::swap(_currLabel, _nextLabel); }

    private:
        State _state{ActiveState};
        QLineF _lineWithMargin;
        QGraphicsItem* _source{nullptr};
        QGraphicsItem* _target{nullptr};
        EdgeLabel* _currLabel{nullptr};
        EdgeLabel* _nextLabel{nullptr};
    };


    /// RootNode is just for visualizes so that the parent InodeEdge of the
    /// actual root Inode does not hang by itself.
    class RootNode final : public QGraphicsEllipseItem
    {
    public:
        enum { Type = UserType + 3 };

        explicit RootNode(QGraphicsItem* parent = nullptr);
        void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

        [[nodiscard]] int type() const override     { return Type; }

    protected:
        QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
    };


    class NewFolderNode final : public QGraphicsEllipseItem
    {
    public:
        enum { Type = UserType + 4 };

        explicit NewFolderNode(QGraphicsItem* parent = nullptr);
        void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
        void setEdge(Edge* edge) { _parentEdge = edge; }

        [[nodiscard]] int type() const override { return Type; }

    protected:
        QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

    private:
        Edge* _parentEdge{nullptr};
    };


    struct StringRotation
    {
        QString text;
        Rotation rot;
    };

    using EdgeStringMap = std::unordered_map<Edge*, StringRotation>;
    using SharedVariantAnimation = QSharedPointer<QVariantAnimation>;
    using SharedSequentialAnimation = QSharedPointer<QSequentialAnimationGroup>;
    using EdgeVector    = std::vector<Edge*>;

    void animateRotation(const QVariantAnimation* animation, const EdgeStringMap& input);


    struct InternalRotState
    {
        InternalRotationStatus status{InternalRotationStatus::MovementImpossible};
        EdgeStringMap changes;
    };


    class Node final : public QGraphicsItem
    {
    public:
        enum { Type = UserType + 2 };

        explicit Node(const QPersistentModelIndex& index);
        [[nodiscard]] static Node* createRootNode(core::FileSystemScene* scene);
        [[nodiscard]] static Node* createNode(QGraphicsScene* scene, QGraphicsItem* parent);

        ~Node() override;
        void init();
        void reload(int start, int end);
        void unload(int start, int end);

        void setIndex(const QPersistentModelIndex& index);
        [[nodiscard]] QString name() const;
        [[nodiscard]] QRectF boundingRect() const override;
        [[nodiscard]] QPainterPath shape() const override;
        [[nodiscard]] bool hasOpenOrHalfClosedChild() const;

        [[nodiscard]] bool isClosed() const         { return _state == FolderState::Closed; }
        [[nodiscard]] bool isOpen() const           { return _state == FolderState::Open; }
        [[nodiscard]] bool isHalfClosed() const     { return _state == FolderState::HalfClosed; }
        [[nodiscard]] bool hasChildren() const      { return !_childEdges.empty(); }
        [[nodiscard]] EdgeVector childEdges() const { return _childEdges; }
        [[nodiscard]] int type() const override     { return Type; }
        [[nodiscard]] Edge* parentEdge() const { return _parentEdge; }
        [[nodiscard]] const QPersistentModelIndex& index() const { return _index; }

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

        void internalRotationAfterClose(Edge* closedEdge);
        InternalRotState doInternalRotationAfterClose(Edge* closedEdge);
        void doInternalRotation(Rotation rot, InternalRotState& result);
        void skipTo(int row, InternalRotState& result);
        void skipTo(int row);

        void spread(QPointF dxy = QPointF(0,0));

        FolderState _state{FolderState::Closed};
        QPersistentModelIndex _index;
        Edge* _parentEdge{nullptr};
        EdgeVector _childEdges;

        SharedVariantAnimation _singleRotAnimation;
        SharedSequentialAnimation _seqRotAnimation;

        inline static std::vector<std::pair<QGraphicsItem*, QPointF>> _ancestorPos;
    };

    void extend(Node* node, qreal distance = 144.0);
    void shrink(Node* node, qreal distance = 144.0);
    void adjustAllEdges(const Node* node);
    void setAllEdgeState(const Node* node, Edge::State state);
}
