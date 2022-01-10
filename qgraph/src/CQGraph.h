#ifndef CQGraph_H
#define CQGraph_H

#include <QFrame>

#include <CDotParse.h>

class CQGraphDotParse;
class CQGraphCanvas;
class CQGraphStatus;

class CForceDirectedMgr;
class GraphPlacer;
class GraphPlacerGraph;

class QTimer;

class CQGraph : public QFrame {
  Q_OBJECT

 public:
  enum class PackType {
    NONE,
    FORCE_DIRECTED,
    CIRCLE_PACK,
    GRAPH_PLACER
  };

 public:
  CQGraph();
 ~CQGraph();

  CQGraphDotParse *parse() const { return parse_; }

  bool isDebug() const { return debug_; }
  void setDebug(bool b) { debug_ = b; }

  const PackType &packType() const { return packType_; }
  void setPackType(const PackType &t) { packType_ = t; }

  void loadFile(const std::string &filename);

  void init();

  CForceDirectedMgr *forceDirected() const { return forceDirected_; }

  void *circlePack() const { return circlePack_; }

  GraphPlacer *graphPlacer() const { return graphPlacer_; }

  GraphPlacerGraph *graphPlacerGraph() const { return graphPlacerGraph_; }

  CDotParse::GraphP minGraph() const { return minGraph_; }

  CDotParse::Graph::NodeArray shortestPath() const { return shortestPath_; }

  CQGraphCanvas *canvas() const { return canvas_; }
  CQGraphStatus *status() const { return status_; }

  double margin() const { return margin_; }

  //---

  void initForceDirected();
  void initCirclePack();

  void createGraphPlacer();
  void initGraphPlacer();

 private slots:
  void animate();

 private:
  bool                        debug_            { false };
  PackType                    packType_         { PackType::NONE };
  CQGraphDotParse*            parse_            { nullptr };
  CForceDirectedMgr*          forceDirected_    { nullptr };
  void*                       circlePack_       { nullptr };
  GraphPlacer*                graphPlacer_      { nullptr };
  GraphPlacerGraph*           graphPlacerGraph_ { nullptr };
  CDotParse::GraphP           minGraph_;
  CDotParse::Graph::NodeArray shortestPath_;
  QTimer*                     timer_            { nullptr };
  CQGraphCanvas*              canvas_           { nullptr };
  CQGraphStatus*              status_           { nullptr };
  double                      margin_           { 32 };
};

class CQGraphCanvas : public QFrame {
  Q_OBJECT

 public:
  CQGraphCanvas(CQGraph *graph);
 ~CQGraphCanvas();

  void mouseMoveEvent(QMouseEvent *) override;

  void resizeEvent(QResizeEvent *) override;

  void paintEvent(QPaintEvent *) override;

  void drawForceDirected(QPainter *painter);
  void drawCirclePack(QPainter *painter);
  void drawGraphPlacer(QPainter *painter);

  QSize sizeHint() const override { return QSize(1536, 1024); }

  QPoint mousePos() const { return mousePos_; }

 private:
  CQGraph *graph_  { nullptr };
  QPoint   mousePos_;
};

class CQGraphStatus : public QFrame {
  Q_OBJECT

 public:
  CQGraphStatus(CQGraph *graph);
 ~CQGraphStatus();

  void paintEvent(QPaintEvent *) override;

  QSize sizeHint() const override;

 private:
  CQGraph *graph_ { nullptr };
};

#endif
