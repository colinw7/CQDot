#ifndef CQGraphVizTest_H
#define CQGraphVizTest_H

#include <QWidget>
#include <CQDisplayRange2D.h>

namespace CQGraphViz {
class App;
class Object;
}

class CQGraphVizTest : public QWidget {
 public:
  enum class Format {
    JSON,
    DOT,
    XDOT
  };

 public:
  CQGraphVizTest();

  bool processFile(const std::string &filename, Format format);

  bool isDebug() const;
  void setDebug(bool b);

 private:
  void resizeEvent(QResizeEvent *) override;

  void paintEvent(QPaintEvent *) override;

  void mouseMoveEvent(QMouseEvent *e) override;

  void keyPressEvent(QKeyEvent *e) override;

  bool event(QEvent *e) override;

  QSize sizeHint() const override;

 private:
  QPainterPath windowToPixel(const QPainterPath &p) const;
  QPointF      windowToPixel(const QPointF &p) const;
  QRectF       windowToPixel(const QRectF &r) const;
  QPointF      pixelToWindow(const QPointF &p) const;

  double windowToPixelWidth (double w) const;
  double windowToPixelHeight(double h) const;

  CQGraphViz::Object *findObjectAt(const QPointF &p);

 private:
  CQGraphViz::App* dot_ { nullptr };
  CQDisplayRange2D range_;
  QPointF          mousePos_;
};

#endif
