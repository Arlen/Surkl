#pragma once

#include <QtWidgets/QGraphicsView>


class QVariantAnimation;

namespace core
{
    class SceneBookmarkItem;
}

namespace gui::view
{
    class QuadrantButton;

    class GraphicsView : public QGraphicsView
    {
        Q_OBJECT

        constexpr static auto MOUSE_POSITION_PROPERTY = "MOUSE_POSITION_PROPERTY";
        constexpr static auto MOUSE_LAST_POSITION_PROPERTY = "MOUSE_LAST_POSITION_PROPERTY";

    public:
        GraphicsView(QGraphicsScene* scene, QWidget* parent = nullptr);

    public slots:
        void requestSceneBookmark();
        void focusQuadrant1(); // top right
        void focusQuadrant2(); // top left
        void focusQuadrant3(); // bottom left
        void focusQuadrant4(); // bottom right
        void focusAllQuadrants(); // center

    protected:
        void enterEvent(QEnterEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    private:
        void configure();

        QPoint mouseMoveVelocity() const;
        QPoint mousePosition() const;
        void saveMousePosition(const QPoint& pos);
        void scaleView(qreal factor);
        void zoom();
        void zoomIn();
        void zoomOut();
        void togglePanOrZoom(Qt::KeyboardModifiers modifiers);

        void destroyBookmarkAnimation();
        void drawBookmarkingCursorAnimation(QPainter& p) const;
        void addSceneBookmark(const QPoint& pos);

        void processSelection();

        QVariantAnimation* _bookmarkAnimation = nullptr;
        QuadrantButton* _quadrantButton = nullptr;
        core::SceneBookmarkItem* _selectedSceneBookmark = nullptr;
    };
}
