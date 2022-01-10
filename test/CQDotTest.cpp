#include <CQDotTest.h>
#include <CQDot.h>

#include <CQPathVisitor.h>

#include <QApplication>
#include <QPainter>
#include <QMouseEvent>
#include <QToolTip>

int
main(int argc, char **argv)
{
  QApplication app(argc, argv);

  auto f = app.font();
  f.setPointSize(20);
  app.setFont(f);

  std::vector<std::string> files;

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      auto arg = std::string(&argv[i][1]);
    }
    else
      files.push_back(argv[i]);
  }

  auto *dot = new CQDotTest;

  for (const auto &file : files)
    dot->processFile(file);

  dot->show();

  return app.exec();
}

//---

//---

CQDotTest::
CQDotTest()
{
  setMouseTracking(true);

  dot_ = new CQDot::App;
}

bool
CQDotTest::
processFile(const std::string &filename)
{
  setWindowTitle(QString("CQDotTest %1").arg(filename.c_str()));

  return dot_->processFile(filename);
}

CQDot::Object *
CQDotTest::
findObjectAt(const QPointF &p)
{
  for (auto &object : dot_->objects())
    if (object->isInside(p))
      return object.get();

  return nullptr;
}

void
CQDotTest::
paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  painter.setRenderHint(QPainter::Antialiasing);
  painter.setRenderHint(QPainter::TextAntialiasing);

  painter.fillRect(rect(), QBrush(QColor(200, 200, 200)));

  auto setPixelLineWidth = [&](double pw) {
    auto pen = painter.pen();

    pen.setWidth(pw);

    painter.setPen(pen);
  };

  auto setLineDotted = [&]() {
    auto pen = painter.pen();

    pen.setStyle(Qt::DotLine);

    painter.setPen(pen);
  };

  auto setLineDashed = [&]() {
    auto pen = painter.pen();

    pen.setStyle(Qt::DashLine);

    painter.setPen(pen);
  };

  auto setWindowLineWidth = [&](double lw) {
    setPixelLineWidth(windowToPixelWidth(lw > 0 ? lw : 1));
  };

  auto setWindowLineStyle = [&](const CQDot::StyleData &style) {
    setWindowLineWidth(style.lineWidth);

    if      (style.lineStyle == CQDot::LineStyle::DOTTED)
      setLineDotted();
    else if (style.lineStyle == CQDot::LineStyle::DASHED)
      setLineDashed();
  };

  auto drawObject = [&](const CQDot::ObjectP &object) {
    for (auto &path : object->paths()) {
      path.closePath();

      auto path1 = windowToPixel(path.path);

      if (! object->isInside()) {
        painter.setBrush(path.bg);
        painter.setPen  (path.fg);

        setWindowLineStyle(path.style);
      }
      else {
        painter.setBrush(Qt::white);
        painter.setPen  (Qt::red);

        setPixelLineWidth(4);
      }

      painter.drawPath(path1);
    }

    for (const auto &line : object->lines()) {
      auto line1 = windowToPixel(line.path);

      if (! object->isInside()) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen  (line.fg);

        setWindowLineStyle(line.style);
      }
      else {
        painter.setBrush(Qt::NoBrush);
        painter.setPen  (Qt::red);

        setPixelLineWidth(4);
      }

      painter.drawPath(line1);
    }
  };

  drawObject(dot_->root());

  for (auto &object : dot_->objects())
    drawObject(object);

  for (auto &edge : dot_->edges())
    drawObject(edge);

  auto scaleFontToRect = [&](const QRectF &r, const QString &text) {
    auto f = font();

    double w = r.width ();
    double h = r.height();

    for (int i = 0; i < 8; ++i) {
      QFontMetricsF fm(f);

      auto tw = fm.width(text);
      auto th = fm.height();

      auto s = std::min(w/tw, h/th);

      f.setPointSizeF(s*f.pointSizeF());
    }

    return f;
  };

  auto drawObjectText = [&](const CQDot::ObjectP &object) {
    for (const auto &text : object->texts()) {
      auto pp = windowToPixel(text.pos);

      double pw = windowToPixelWidth (text.width);
      double ph = windowToPixelHeight(text.size );

      QRectF rect;

      if      (text.align & Qt::AlignLeft)
        rect = QRectF(pp.x()         , pp.y() - ph, pw, ph);
      else if (text.align & Qt::AlignRight)
        rect = QRectF(pp.x() - pw    , pp.y() - ph, pw, ph);
      else
        rect = QRectF(pp.x() - pw/2.0, pp.y() - ph, pw, ph);

      auto f = scaleFontToRect(rect, text.text);

      painter.setFont(f);

      if (! object->isInside())
        painter.setPen(text.fg);
      else
        painter.setPen(Qt::red);

      //painter.drawText(rect, Qt::AlignCenter, text.text);

      QFontMetricsF fm(painter.font());

      double tx = rect.left();
      double ty = rect.center().y() + (fm.ascent() - fm.descent())/2.0;

      painter.drawText(tx, ty, text.text);

      //painter.setPen(Qt::red);
      //painter.setBrush(Qt::NoBrush);
      //painter.drawRect(rect);
    }

    auto op = windowToPixel(object->pos());

    double ow = windowToPixelWidth (object->width ());
    double oh = windowToPixelHeight(object->height());

    auto rect = QRectF(op.x() - ow/2.0, op.y() - oh, ow, oh);

    painter.setPen(Qt::red);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(rect);
  };

  drawObjectText(dot_->root());

  for (auto &object : dot_->objects())
    drawObjectText(object);

  for (auto &edge : dot_->edges())
    drawObjectText(edge);

  //---

  // draw mouse pos
  painter.setFont(font());

  QFontMetricsF fm(painter.font());

  painter.setPen(Qt::black);

  int tx = width () - fm.width("XXX.XXX XXX.XXX") - 1;
  int ty = height() - fm.descent() - 1;

  painter.drawText(tx, ty, QString("%1 %2").arg(mousePos_.x()).arg(mousePos_.y()));
}

void
CQDotTest::
resizeEvent(QResizeEvent *)
{
  auto bbox = dot_->bbox();

  range_ = CDisplayRange2D(0, 0, width() - 1, height() - 1,
                           bbox.left(), bbox.top(), bbox.right(), bbox.bottom());

  range_.setEqualScale(true);
}

void
CQDotTest::
mouseMoveEvent(QMouseEvent *e)
{
  mousePos_ = pixelToWindow(e->pos());

  for (auto &object : dot_->objects()) {
    object->setInside(object->isInside(mousePos_));

    for (auto *obj : object->srcEdges())
      obj->setInside(false);

    for (auto *obj : object->destEdges())
      obj->setInside(false);
  }

  for (auto &object : dot_->objects()) {
    if (object->isInside()) {
      for (auto *obj : object->srcEdges())
        obj->setInside(true);

      for (auto *obj : object->destEdges())
        obj->setInside(true);
    }
  }

  update();
}

void
CQDotTest::
keyPressEvent(QKeyEvent *e)
{
  if      (e->key() == Qt::Key_Plus) {
    range_.zoomIn();
  }
  else if (e->key() == Qt::Key_Minus) {
    range_.zoomOut();
  }
  else if (e->key() == Qt::Key_Up) {
    range_.scrollY(-0.1*range_.getWindowHeight());
  }
  else if (e->key() == Qt::Key_Down) {
    range_.scrollY(0.1*range_.getWindowHeight());
  }
  else if (e->key() == Qt::Key_Left) {
    range_.scrollX(0.1*range_.getWindowWidth());
  }
  else if (e->key() == Qt::Key_Right) {
    range_.scrollX(-0.1*range_.getWindowWidth());
  }

  update();
}

bool
CQDotTest::
event(QEvent *e)
{
  auto escapeText = [&](const QString &str) {
    QString str1;

    int i   = 0;
    int len = str.length();

    while (i < len) {
      if      (str[i] == '<') {
        str1 += "&lt;"; ++i;
      }
      else if (str[i] == '>') {
        str1 += "&gt;"; ++i;
      }
      else if (str[i] == '"') {
        str1 += "&quot;"; ++i;
      }
      else if (str[i] == '&') {
        str1 += "&amp;"; ++i;
      }
      else
        str1 += str[i++];
    }

    return str1;
  };

  if (e->type() == QEvent::ToolTip) {
    auto *helpEvent = static_cast<QHelpEvent *>(e);

    auto p = pixelToWindow(helpEvent->pos());

    auto *obj = findObjectAt(p);

    if (obj) {
      auto tipText = QString("<table>");

      tipText += QString("<tr><th style='text-align:right'>%1</th>"
                         "<td>&nbsp;</td><td>%2</td></tr>\n").
                         arg("Id").arg(obj->id());
      tipText += QString("<tr><th style='text-align:right'>%1</th>"
                         "<td>&nbsp;</td><td>%2</td></tr>\n").
                         arg("Name").arg(escapeText(obj->name()));
      tipText += QString("<tr><th style='text-align:right'>%1</th>"
                         "<td>&nbsp;</td><td>%2</td></tr>\n").
                         arg("Label").arg(escapeText(obj->label()));

      tipText += QString("</table>");

      QToolTip::showText(helpEvent->globalPos(), tipText);
    }
    else
      QToolTip::hideText();

    e->ignore();

    return true;
  }

  return QWidget::event(e);
}

QPainterPath
CQDotTest::
windowToPixel(const QPainterPath &path) const
{
  class PathVisitor : public CQPathVisitor::Base {
   public:
    PathVisitor(const CQDotTest *dot) :
     dot_(dot) {
    }

    void moveTo(const QPointF &p) override {
      auto pp = dot_->windowToPixel(p);

      path_.moveTo(pp);
    }

    void lineTo(const QPointF &p) override {
      auto pp = dot_->windowToPixel(p);

      path_.lineTo(pp);
    }

    void quadTo(const QPointF &p1, const QPointF &p2) override {
      auto pp1 = dot_->windowToPixel(p1);
      auto pp2 = dot_->windowToPixel(p2);

      path_.quadTo(pp1, pp2);
    }

    void curveTo(const QPointF &p1, const QPointF &p2, const QPointF &p3) override {
      auto pp1 = dot_->windowToPixel(p1);
      auto pp2 = dot_->windowToPixel(p2);
      auto pp3 = dot_->windowToPixel(p3);

      path_.cubicTo(pp1, pp2, pp3);
    }

    const QPainterPath &path() const { return path_; }

   private:
    const CQDotTest* dot_ { nullptr };
    QPainterPath     path_;
  };

  PathVisitor visitor(this);

  CQPathVisitor::visitPath(path, visitor);

  return visitor.path();
}

double
CQDotTest::
windowToPixelWidth(double w) const
{
  auto p1 = windowToPixel(QPointF(0, 0));
  auto p2 = windowToPixel(QPointF(w, 0));

  return std::abs(p2.x() - p1.x());
}

double
CQDotTest::
windowToPixelHeight(double h) const
{
  auto p1 = windowToPixel(QPointF(0, 0));
  auto p2 = windowToPixel(QPointF(0, h));

  return std::abs(p2.y() - p1.y());
}

QPointF
CQDotTest::
windowToPixel(const QPointF &p) const
{
  double x, y;

  range_.windowToPixel(p.x(), p.y(), &x, &y);

  return QPointF(x, y);
}

QPointF
CQDotTest::
pixelToWindow(const QPointF &p) const
{
  double x, y;

  range_.pixelToWindow(p.x(), p.y(), &x, &y);

  return QPointF(x, y);
}

QSize
CQDotTest::
sizeHint() const
{
  return QSize(1280, 1024);
}
