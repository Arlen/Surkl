#pragma once

#include <QDir>
#include <QGraphicsItem>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>


class QVariantAnimation;
class QSequentialAnimationGroup;

namespace  gui::view
{
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

    inline QString to_string(InternalRotationStatus status)
    {
        switch (status) {
            case InternalRotationStatus::EndReachedCW: return "EndReachedCW";
            case InternalRotationStatus::EndReachedCCW: return "EndReachedCCW";
            case InternalRotationStatus::MovementImpossible: return "MovementImpossible";
            case InternalRotationStatus::Normal: return "Normal";
        }
        return "Unknown";
    }


    class InodeEdgeLabel final : public QGraphicsSimpleTextItem
    {
    public:
        explicit InodeEdgeLabel(QGraphicsItem* parent = nullptr);
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


    class InodeEdge final : public QGraphicsLineItem
    {
    public:
        enum { Type = UserType + 1 };

        explicit InodeEdge(QGraphicsItem* source = nullptr, QGraphicsItem* target = nullptr);
        void setName(const QString& name) const;
        void adjust();
        void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

        [[nodiscard]] QGraphicsItem *source() const { return _source; }
        [[nodiscard]] QGraphicsItem *target() const { return _target; }
        [[nodiscard]] InodeEdgeLabel* currLabel() const { return _currLabel; }
        [[nodiscard]] InodeEdgeLabel* nextLabel() const { return _nextLabel; }
        [[nodiscard]] QPainterPath shape() const override;
        [[nodiscard]] int type() const override { return Type; }
        void swapLabels() { std::swap(_currLabel, _nextLabel); }

    private:
        QGraphicsItem* _source{nullptr};
        QGraphicsItem* _target{nullptr};
        InodeEdgeLabel* _currLabel{nullptr};
        InodeEdgeLabel* _nextLabel{nullptr};
    };


    struct StringRotation
    {
        QString text;
        Rotation rot;
    };

    using EdgeStringMap = std::unordered_map<InodeEdge*, StringRotation>;
    using SharedVariantAnimation = QSharedPointer<QVariantAnimation>;
    using SharedSequentialAnimation = QSharedPointer<QSequentialAnimationGroup>;
    using InodeEdges    = std::vector<std::pair<InodeEdge*, qsizetype>>;

    void animateRotation(SharedVariantAnimation animation, const EdgeStringMap& input);


    struct InternalRotState
    {
        InternalRotationStatus status{InternalRotationStatus::MovementImpossible};
        EdgeStringMap changes;
    };


    class Inode final : public QGraphicsItem
    {
    public:
        enum { Type = UserType + 2 };

        explicit Inode(const QDir& dir);
        void init();
        void setDir(const QDir& dir);
        [[nodiscard]] QString name() const;
        [[nodiscard]] QRectF boundingRect() const override;
        [[nodiscard]] QPainterPath shape() const override;

        [[nodiscard]] bool isClosed() const         { return _state == FolderState::Closed; }
        [[nodiscard]] bool isOpen() const           { return _state == FolderState::Open; }
        [[nodiscard]] bool isHalfClosed() const     { return _state == FolderState::HalfClosed; }
        [[nodiscard]] bool hasChildren() const      { return !_childEdges.empty(); }
        [[nodiscard]] InodeEdges childEdges() const { return _childEdges; }
        [[nodiscard]] int type() const override     { return Type; }

        void paint(QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
        void close();
        void halfClose();
        void open();
        void rotate(Rotation rot);

    protected:
        QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;
        void keyPressEvent(QKeyEvent *event) override;
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
        void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    private:
        void onChildInodeOpened(const InodeEdge* inode);
        void onChildInodeClosed(const InodeEdge* inode);

        void internalRotationAfterClose(InodeEdge* closedEdge);
        InternalRotState doInternalRotationAfterClose(InodeEdge* closedEdge);
        void doInternalRotation(int begin, int end, Rotation rot, InternalRotState& result);
        StringRotation setEdgeInodeIndex(int edgeIndex, qsizetype inodeIndex);

        void spread(InodeEdge* ignoredChild = nullptr);
        void extend(float distance = 128.0);
        void reduce(float distance = 128.0);

        FolderState _state{FolderState::Closed};
        qsizetype _winSize{8};
        InodeEdge* _parentEdge{nullptr};
        InodeEdges _childEdges;
        std::set<qsizetype> _openEdges;
        QDir _dir;

        SharedVariantAnimation _singleRotAnimation;
        SharedSequentialAnimation _seqRotAnimation;
    };
}
