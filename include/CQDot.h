#ifndef CQDot_H
#define CQDot_H

#include <QPainterPath>
#include <QRectF>
#include <QPointF>
#include <QColor>

#include <vector>
#include <set>
#include <memory>

namespace CQDot {

class Object;

using ObjectP = std::shared_ptr<Object>;
using EdgeP   = std::shared_ptr<Object>;

using Objects = std::vector<ObjectP>;
using Edges   = std::vector<EdgeP>;

using ObjectSet = std::set<Object *>;

class App {
 public:
  App();

  bool processFile(const std::string &filename);

 private:
  Object *findObject(int id);

  void errorMsg(const std::string &str) const;
//void debugMsg(const std::string &str) const;

 public:
  using Points = std::vector<QPointF>;

  struct ColorData {
    std::string grad;
    QColor      bg { Qt::transparent };
    QColor      fg { Qt::transparent };

    void reset() {
      bg = QColor(Qt::transparent);
      fg = QColor(Qt::transparent);
    }
  };

 public:
  const QRectF &bbox() const { return bbox_; }

  const ObjectP &root() const { return root_; }

  const Objects &objects() const { return objects_; }

  const Edges &edges() const { return edges_; }

  bool isDirected() const { return directed_; }

 private:
  QRectF  bbox_;
  ObjectP root_;
  Objects objects_;
  Edges   edges_;
  bool    directed_ { false };
};

//---

enum class LineStyle {
  NONE,
  SOLID,
  DOTTED,
  DASHED
};

struct StyleData {
  LineStyle lineStyle { LineStyle::NONE };
  int       lineWidth { -1 };
};

struct PathData {
  QPainterPath path;
  bool         closed { false };
  QColor       bg;
  QColor       fg;
  StyleData    style;

  void closePath() const {
    if (! closed) {
      auto *th = const_cast<PathData *>(this);

      th->path.closeSubpath();

      th->closed = true;
    }
  }
};

using Paths = std::vector<PathData>;

struct TextData {
  double        size;
  QString       face;
  QPointF       pos;
  Qt::Alignment align { Qt::AlignHCenter };
  double        width { 0.0 };
  QString       text;
  QColor        fg;
};

using Texts = std::vector<TextData>;

//---

class Object {
 public:
  enum class Type {
    NONE,
    OBJECT,
    EDGE
  };

 public:
  Object() { }

  const Type &type() const { return type_; }
  void setType(const Type &v) { type_ = v; }

  int id() const { return id_; }
  void setId(int i) { id_ = i; }

  int headId() const { return headId_; }
  void setHeadId(int i) { headId_ = i; }

  int tailId() const { return tailId_; }
  void setTailId(int i) { tailId_ = i; }

  const QString &name() const { return name_; }
  void setName(const QString &v) { name_ = v; }

  const QString &label() const { return label_; }
  void setLabel(const QString &v) { label_ = v; }

  const QPointF &pos() const { return pos_; }
  void setPos(const QPointF &v) { pos_ = v; }

  double width() const { return width_; }
  void setWidth(double r) { width_ = r; }

  double height() const { return height_; }
  void setHeight(double r) { height_ = r; }

  const QRectF &rect() const { return rect_; }
  void setRect(const QRectF &v) { rect_ = v; }

  bool isInside() const { return inside_; }
  void setInside(bool b) { inside_ = b; }

  //---

  bool isInside(const QPointF &p) const {
    return rect_.contains(p);
  }

  //----

  const Paths &paths() const { return paths_; }

  void addPath(const PathData &pathData) {
    paths_.push_back(pathData);
  }

  const Paths &lines() const { return lines_; }

  void addLine(const PathData &pathData) {
    lines_.push_back(pathData);
  }

  const Texts &texts() const { return texts_; }

  void addText(const TextData &textData) {
    texts_.push_back(textData);
  }

  //---

  const ObjectSet &srcEdges() const { return srcEdges_; }

  void addSrcEdge(Object *obj) {
    srcEdges_.insert(obj);
  }

  const ObjectSet &destEdges() const { return destEdges_; }

  void addDestEdge(Object *obj) {
    destEdges_.insert(obj);
  }

 private:
  Type      type_   { Type::NONE };
  int       id_     { -1 };
  int       headId_ { -1 };
  int       tailId_ { -1 };
  QString   name_;
  QString   label_;
  QPointF   pos_;
  double    width_  { -1 };
  double    height_ { -1 };
  QRectF    rect_;
  Paths     paths_;
  Paths     lines_;
  Texts     texts_;
  bool      inside_ { false };
  ObjectSet srcEdges_;
  ObjectSet destEdges_;
};

//---

}

#endif
