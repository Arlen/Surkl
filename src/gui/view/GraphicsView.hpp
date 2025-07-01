/// Copyright (C) 2025 Arlen Avakian
/// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QtWidgets/QGraphicsView>


class QTimeLine;
class QVariantAnimation;

namespace core
{
    class SceneBookmarkItem;
    class FileSystemScene;
}

namespace gui::view
{
    class QuadrantButton;

    class GraphicsView : public QGraphicsView
    {
        Q_OBJECT

        constexpr static auto MOUSE_POSITION_PROPERTY = "MOUSE_POSITION_PROPERTY";
        constexpr static auto MOUSE_LAST_POSITION_PROPERTY = "MOUSE_LAST_POSITION_PROPERTY";

    signals:
        void deletePressed();
        void sceneBookmarkRequested(const QPoint& pos, const QString& name);

    public:
        explicit GraphicsView(core::FileSystemScene* scene, QWidget* parent = nullptr);

    public slots:
        void requestSceneBookmark();
        void focusQuadrant1(); // top right
        void focusQuadrant2(); // top left
        void focusQuadrant3(); // bottom left
        void focusQuadrant4(); // bottom right
        void focusAllQuadrants(); // center

    protected:
        void dragEnterEvent(QDragEnterEvent *event) override;
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
        void centerTargetOn(const core::SceneBookmarkItem* bm, const QPointF& target);

        void pickSceneBookmark();
        QList<core::SceneBookmarkItem*> selectedSceneBookmarks() const;

        QVariantAnimation* _bookmarkAnimation{nullptr};
        QuadrantButton* _quadrantButton{nullptr};
        QTimeLine* _timeline{nullptr};
        QLineF _zoomAnchor;
    };
}
