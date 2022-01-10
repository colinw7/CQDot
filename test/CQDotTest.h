#ifndef CQDotTest_H
#define CQDotTest_H

#include <QWidget>
#include <CDisplayRange2D.h>

namespace CQDot {
class App;
class Object;
}

class CQDotTest : public QWidget {
 public:
  CQDotTest();

  bool processFile(const std::string &filename);

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
  QPointF      pixelToWindow(const QPointF &p) const;

  double windowToPixelWidth (double w) const;
  double windowToPixelHeight(double h) const;

  CQDot::Object *findObjectAt(const QPointF &p);

 private:
  CQDot::App*     dot_ { nullptr };
  CDisplayRange2D range_;
  QPointF         mousePos_;
};

#endif
