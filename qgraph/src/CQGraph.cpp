#include <CQGraph.h>
#include <CDotParse.h>
#include <CForceDirected.h>
#include <CirclePack.h>
#include <CGraphPlacer.h>
#include <CDisplayRange2D.h>

#include <QApplication>
#include <QVBoxLayout>
#include <QPainter>
#include <QTimer>
#include <QMouseEvent>

#include <iostream>

class CForceDirectedDotNode : public CDotParse::Node  {
 public:
  CForceDirectedDotNode(CDotParse::Parse *parse, const std::string &name="") :
   CDotParse::Node(parse->currentGraph(), name) {
    static int s_id;

    id_ = ++s_id;
  }

  virtual ~CForceDirectedDotNode() { }

  CDotParse::Parse *parse() const { return parse_; }

  int id() const { return id_; }

 private:
  CDotParse::Parse *parse_ { nullptr };
  int               id_    { 0 };
};

class CForceDirectedSpringNode : public Springy::Node  {
 public:
  CForceDirectedSpringNode(int id, CDotParse::Parse *parse, const std::string &name="") :
   Springy::Node(id), parse_(parse), name_(name) {
  }

  virtual ~CForceDirectedSpringNode() { }

  CDotParse::Parse *parse() const { return parse_; }

  const std::string &name() const { return name_; }

 private:
  CDotParse::Parse *parse_ { nullptr };
  std::string       name_;
};

class CForceDirectedDotEdge : public CDotParse::Edge {
 public:
  using NodeP = Springy::NodeP;

 public:
  CForceDirectedDotEdge(CDotParse::Node *fromNode=nullptr, CDotParse::Node *toNode=nullptr) :
   CDotParse::Edge(fromNode, toNode) {
    static int s_id;

    id_ = ++s_id;
  }

  virtual ~CForceDirectedDotEdge() { }

  int id() const { return id_; }

 private:
  int id_ { 0 };
};

class CForceDirectedSpringEdge : public Springy::Edge  {
 public:
  using NodeP = Springy::NodeP;

 public:
  CForceDirectedSpringEdge(int id, NodeP fromNode=NodeP(), NodeP toNode=NodeP()) :
   Springy::Edge(id, fromNode, toNode) {
  }

  virtual ~CForceDirectedSpringEdge() { }
};

class CForceDirectedSpringGraph : public Springy::Graph {
 public:
  explicit CForceDirectedSpringGraph(CDotParse::Parse *parse) :
   Springy::Graph(), parse_(parse) {
  }

  virtual ~CForceDirectedSpringGraph() { }

  Springy::NodeP makeNode() const override {
    return Springy::NodeP(new CForceDirectedSpringNode(++nextNodeId_, parse_));
  }

  Springy::EdgeP makeEdge() const override {
    return Springy::EdgeP(new CForceDirectedSpringEdge(++nextEdgeId_));
  }

 private:
  CDotParse::Parse *parse_ { nullptr };
};

class CForceDirectedMgr : public CForceDirected {
 public:
  CForceDirectedMgr(CDotParse::Parse *parse) :
   CForceDirected(), parse_(parse) {
  }

  virtual ~CForceDirectedMgr() { }

  Springy::GraphP makeGraph() const override {
    return Springy::GraphP(new CForceDirectedSpringGraph(parse_));
  }

 private:
  CDotParse::Parse *parse_ { nullptr };
};

//------

class CirclePackNode : public CDotParse::Node, public CCircleNode {
 public:
  CirclePackNode(CDotParse::Parse *parse, const std::string &name="") :
   CDotParse::Node(parse->currentGraph(), name),
   CCircleNode    () {
  }

  virtual ~CirclePackNode() { }
};

using CirclePack = CCirclePack<CirclePackNode>;

//------

class CQGraphDotParse;

class GraphPlacer : public CGraphPlacer {
 public:
  GraphPlacer(CQGraph *graph) :
   graph_(graph) {
    setSize(100, 100);
  }

  virtual ~GraphPlacer() { }

  CQGraph *graph() const { return graph_; }

  void setSize(int w, int h) {
    int m = graph_->margin();

    range_ = CDisplayRange2D(m, m, w - m - 1, h - m - 1, -1.0, -1.0, 1.0, 1.0);
  }

  Graph *makeGraph(const std::string &name) const override;

  Node *makeNode(const std::string &name) const override;

  Edge *makeEdge(const OptReal &value, Node *srcNode, Node *destNode) const override;

  double lengthDeviceWidth(const Length &l) override {
    if      (l.isPixel())
      return pixelToDeviceWidth(l.pxValue());
    else if (l.isPercent())
      return l.percentValue()/100.0;
    else
      assert(false);

    return l.pxValue();
  }

  double lengthDeviceHeight(const Length &l) override {
    if      (l.isPixel())
      return pixelToDeviceHeight(l.pxValue());
    else if (l.isPercent())
      return l.percentValue()/100.0;
    else
      assert(false);

    return l.pxValue();
  }

  double deviceToPixelWidth(double x) override {
    double px1, px2, py;
    windowToPixel(0.0, 0.0, px1, py);
    windowToPixel(x  , 0.0, px2, py);
    return std::abs(px2 - px1);
  }

  double deviceToPixelHeight(double y) override {
    double px, py1, py2;
    windowToPixel(0.0, 0.0, px, py1);
    windowToPixel(0.0, y  , px, py2);
    return std::abs(py2 - py1);
  }

  void windowToPixel(double wx, double wy, double &px, double &py) {
    range_.windowToPixel(wx, wy, &px, &py);
  }

  double pixelToDeviceWidth(double x) override {
    double px1, px2, py;
    pixelToWindow(0.0, 0.0, px1, py);
    pixelToWindow(x  , 0.0, px2, py);
    return std::abs(px2 - px1);
  }

  double pixelToDeviceHeight(double y) override {
    double px, py1, py2;
    pixelToWindow(0.0, 0.0, px, py1);
    pixelToWindow(0.0, y  , px, py2);
    return std::abs(py2 - py1);
  }

  void pixelToWindow(double px, double py, double &wx, double &wy) {
    range_.pixelToWindow(px, py, &wx, &wy);
  }

 private:
  CQGraph*        graph_ { nullptr };
  CDisplayRange2D range_;
};

class GraphPlacerGraph : public CGraphPlacerGraph {
 public:
  GraphPlacerGraph(CQGraph *graph, const std::string &name="");

 ~GraphPlacerGraph() { }
};

class GraphPlacerDotNode : public CDotParse::Node {
 public:
  GraphPlacerDotNode(CDotParse::Graph *graph, const std::string &name="");

 ~GraphPlacerDotNode() { }
};

class GraphPlacerNode : public CGraphPlacerNode {
 public:
  GraphPlacerNode(CQGraph *graph, const std::string &name="");

 ~GraphPlacerNode() { }

  const std::string &color() const { return color_; }
  void setColor(const std::string &s) { color_ = s; std::cout << "Color: " << s << "\n"; }

  const std::string &label() const { return label_; }
  void setLabel(const std::string &s) { label_ = s; std::cout << "Label: " << s << "\n"; }

 private:
  std::string color_;
  std::string label_;
};

class GraphPlacerDotEdge : public CDotParse::Edge {
 public:
  GraphPlacerDotEdge(CDotParse::Node *srcMode, CDotParse::Node *destMode);

 ~GraphPlacerDotEdge() { }
};

class GraphPlacerEdge : public CGraphPlacerEdge {
 public:
  GraphPlacerEdge(CQGraph *graph, CGraphPlacer::OptReal value,
                  GraphPlacerNode *srcMode, GraphPlacerNode *destMode);

 ~GraphPlacerEdge() { }
};

//---

class CQGraphDotParse : public CDotParse::Parse {
 public:
  CQGraphDotParse(CQGraph *graph, const std::string &filename) :
   CDotParse::Parse(filename), graph_(graph) {
  }

  virtual ~CQGraphDotParse() {
  }

  void init() {
    if      (graph_->packType() == CQGraph::PackType::FORCE_DIRECTED) {
    }
    else if (graph_->packType() == CQGraph::PackType::CIRCLE_PACK) {
    }
    else if (graph_->packType() == CQGraph::PackType::GRAPH_PLACER) {
    }
  }

  // make dot graph
  CDotParse::Graph *makeGraph(const std::string &name) const override {
    if      (graph_->packType() == CQGraph::PackType::FORCE_DIRECTED) {
      return CDotParse::Parse::makeGraph(name);
    }
    else if (graph_->packType() == CQGraph::PackType::CIRCLE_PACK) {
      return CDotParse::Parse::makeGraph(name);
    }
    else if (graph_->packType() == CQGraph::PackType::GRAPH_PLACER) {
      return CDotParse::Parse::makeGraph(name);
    }
    else
      return CDotParse::Parse::makeGraph(name);
  }

  // make dot node
  CDotParse::Node *makeNode(CDotParse::Graph *graph, const std::string &name) const override {
    if      (graph_->packType() == CQGraph::PackType::FORCE_DIRECTED) {
      auto *node = new CForceDirectedDotNode(graph->parse(), name);

      return node;
    }
    else if (graph_->packType() == CQGraph::PackType::CIRCLE_PACK) {
      auto *node = new CirclePackNode(graph_->parse(), name);

      return dynamic_cast<CDotParse::Node *>(node);
    }
    else if (graph_->packType() == CQGraph::PackType::GRAPH_PLACER) {
      auto *node = new GraphPlacerDotNode(graph, name);

      return node;
    }
    else
      return CDotParse::Parse::makeNode(graph, name);
  }

  // make dot edge
  CDotParse::Edge *makeEdge(CDotParse::Node *node1, CDotParse::Node *node2) const override {
    if      (graph_->packType() == CQGraph::PackType::FORCE_DIRECTED) {
      auto *edge = new CForceDirectedDotEdge(node1, node2);

      return edge;
    }
    else if (graph_->packType() == CQGraph::PackType::CIRCLE_PACK) {
      return CDotParse::Parse::makeEdge(node1, node2);
    }
    else if (graph_->packType() == CQGraph::PackType::GRAPH_PLACER) {
      auto *edge = new GraphPlacerDotEdge(node1, node2);

      return edge;
    }
    else
      return CDotParse::Parse::makeEdge(node1, node2);
  }

 private:
  CQGraph* graph_ { nullptr };
};

//---

GraphPlacerGraph::
GraphPlacerGraph(CQGraph *graph, const std::string &name) :
 CGraphPlacerGraph(graph->graphPlacer(), name)
{
}

GraphPlacerDotNode::
GraphPlacerDotNode(CDotParse::Graph *graph, const std::string &name) :
 CDotParse::Node(graph, name)
{
}

GraphPlacerNode::
GraphPlacerNode(CQGraph *graph, const std::string &name) :
 CGraphPlacerNode(graph->graphPlacer(), name)
{
}

GraphPlacerDotEdge::
GraphPlacerDotEdge(CDotParse::Node *srcNode, CDotParse::Node *destNode) :
 CDotParse::Edge(srcNode, destNode)
{
}

GraphPlacerEdge::
GraphPlacerEdge(CQGraph *graph, CGraphPlacer::OptReal value, GraphPlacerNode *srcNode,
                GraphPlacerNode *destNode) :
 CGraphPlacerEdge(graph->graphPlacer(), value, srcNode, destNode)
{
}

//---

int
main(int argc, char **argv)
{
  QApplication app(argc, argv);

  std::vector<std::string> args;
  CQGraph::PackType        packType = CQGraph::PackType::FORCE_DIRECTED;
  int                      debug    = false;

  for (int i = 1; i < argc; ++i) {
    if   (argv[i][0] == '-') {
      auto arg = std::string(&argv[i][1]);

      if      (arg == "h")
        std::cerr << "Usage: CQGraph [-debug] [-h]\n";
      else if (arg == "force_directed")
        packType = CQGraph::PackType::FORCE_DIRECTED;
      else if (arg == "circle" || arg == "circle_pack")
        packType = CQGraph::PackType::CIRCLE_PACK;
      else if (arg == "graph" || arg == "graph_placer")
        packType = CQGraph::PackType::GRAPH_PLACER;
      else if (arg == "debug")
        debug = true;
    }
    else
      args.push_back(argv[i]);
  }

  auto graph = std::make_unique<CQGraph>();

  graph->setDebug(debug);

  graph->setPackType(packType);

  //---

  for (const auto &arg : args)
    graph->loadFile(arg);

  graph->init();

  //---

  graph->show();

  app.exec();
}

//-------

CQGraph::
CQGraph()
{
  auto *layout = new QVBoxLayout(this);

  canvas_ = new CQGraphCanvas(this);
  status_ = new CQGraphStatus(this);

  layout->addWidget(canvas_);
  layout->addWidget(status_);
}

CQGraph::
~CQGraph()
{
  delete parse_;

  delete forceDirected_;
  delete static_cast<CirclePack *>(circlePack_);
  delete graphPlacer_;
  //delete graphPlacerGraph_;
}

void
CQGraph::
loadFile(const std::string &filename)
{
  delete parse_;

  parse_ = new CQGraphDotParse(this, filename);

  parse_->init();

  parse_->setDebug(isDebug());

  if (! parse_->parse()) {
    std::cerr << "Parse failed\n";
    return;
  }

  if (isDebug()) {
    for (const auto &ng : parse_->graphs()) {
      const auto &gname = ng.first;
      auto        graph = ng.second;

      std::cerr << "Graph: " << gname << "\n";

      for (const auto &nn : graph->nodes()) {
        auto node1 = nn.second;

        for (const auto &edge : node1->edges()) {
          auto node2 = edge->toNode();

          std::cerr << "  Node: " << node2->name() << " -> " << node2->name() << "\n";
        }
      }
    }
  }
}

void
CQGraph::
init()
{
  if (! parse_)
    return;

  if      (packType() == PackType::FORCE_DIRECTED) {
    forceDirected_ = new CForceDirectedMgr(parse());

    initForceDirected();
  }
  else if (packType() == PackType::CIRCLE_PACK) {
    circlePack_ = new CirclePack;

    initCirclePack();
  }
  else if (packType() == PackType::GRAPH_PLACER) {
    createGraphPlacer();

    initGraphPlacer();
  }

  if (packType() == PackType::GRAPH_PLACER) {
    minGraph_ = parse_->currentGraph()->minimumSpaningTree();

    //minGraph_->print(std::cerr);

    if (minGraph_->nodes().size() > 1) {
      std::cerr << "Shortest Path\n";

      CDotParse::NodeP minNode, maxNode;

      CGraphPlacerNode *minPNode = nullptr;
      CGraphPlacerNode *maxPNode = nullptr;

      for (auto nnode : minGraph_->nodes()) {
        auto node = nnode.second;

        auto *pnode = dynamic_cast<CGraphPlacerGraph *>(graphPlacerGraph_)->findNode(node->name());

        if (! minPNode || pnode->pos() < minPNode->pos()) {
          minNode  = node;
          minPNode = pnode;
        }

        if (! maxPNode || pnode->pos() > maxPNode->pos()) {
          maxNode  = node;
          maxPNode = pnode;
        }
      }

      if (minNode != maxNode) {
        shortestPath_ = parse_->currentGraph()->shortestPath(minNode, maxNode);

#if 0
        for (const auto &node : shortestPath_) {
          node->print(std::cerr);

          std::cerr << "\n";
        }
#endif
      }
    }
  }
}

void
CQGraph::
initForceDirected()
{
  auto *forceDirected = forceDirected_;
  if (! forceDirected) return;

  int edgeId { 0 };

  for (const auto &ng : parse_->graphs()) {
    auto pgraph = ng.second;

    for (const auto &nn : pgraph->nodes()) {
      auto node1 = nn.second;

      auto *dnode1 = dynamic_cast<CForceDirectedDotNode *>(node1.get());

      auto fnode1 = forceDirected->getNode(dnode1->id());

      if (! fnode1) {
        fnode1 = Springy::NodeP(
          new CForceDirectedSpringNode(dnode1->id(), dnode1->parse(), dnode1->name()));

        forceDirected->addNode(fnode1);
      }

      for (const auto &edge : node1->edges()) {
        auto *node2 = edge->toNode();

        auto *dnode2 = dynamic_cast<CForceDirectedDotNode *>(node2);

        auto fnode2 = forceDirected->getNode(dnode2->id());

        if (! fnode2) {
          fnode2 = Springy::NodeP(
            new CForceDirectedSpringNode(dnode2->id(), dnode2->parse(), dnode2->name()));

          forceDirected->addNode(fnode2);
        }

        auto fedge = Springy::EdgeP(new CForceDirectedSpringEdge(++edgeId, fnode1, fnode2));

        forceDirected->addEdge(fedge);
      }
    }
  }

  int    initSteps { 1000 };
  double stepSize  { 0.01 };

  for (int i = 0; i < initSteps; ++i)
    forceDirected->step(stepSize);

  timer_ = new QTimer(this);

  connect(timer_, SIGNAL(timeout()), this, SLOT(animate()));

  timer_->start(250);
}

void
CQGraph::
initCirclePack()
{
  auto *pack = static_cast<CirclePack *>(circlePack_);

  for (const auto &ng : parse_->graphs()) {
    auto pgraph = ng.second;

    for (const auto &nn : pgraph->nodes()) {
      auto node = nn.second;

      pack->addNode(dynamic_cast<CirclePackNode *>(node.get()));
    }
  }
}

void
CQGraph::
createGraphPlacer()
{
  delete graphPlacer_;

  graphPlacer_      = new GraphPlacer(this);
  graphPlacerGraph_ = dynamic_cast<GraphPlacerGraph *>(graphPlacer_->getOrCreateGraph(0, -1));

  graphPlacer_->setOrientation(CGraphPlacer::Orientation::VERTICAL);
  graphPlacer_->setAlign(CGraphPlacer::Align::SRC);
  graphPlacer_->setAlignFirstLast(true);
  graphPlacer_->setNodeScaled(true);
  graphPlacer_->setNodePerpScaled(true);
}

void
CQGraph::
initGraphPlacer()
{
  auto *placer = graphPlacer_;

  for (const auto &ng : parse_->graphs()) {
    auto pgraph = ng.second;

    for (const auto &nn : pgraph->nodes()) {
      auto node1 = nn.second;

      auto *pnode1 = placer->findNode(node1->name());

      if (! pnode1) {
        pnode1 = placer->addNode(node1->name());

        pnode1->setName(node1->name());

        dynamic_cast<GraphPlacerNode *>(pnode1)->setColor(node1->color());
        dynamic_cast<GraphPlacerNode *>(pnode1)->setLabel(node1->label());
      }

      for (const auto &edge : node1->edges()) {
        auto node2 = edge->toNode();

        auto *pnode2 = placer->findNode(node2->name());

        if (! pnode2) {
          pnode2 = placer->addNode(node2->name());

          pnode2->setName(node2->name());

          dynamic_cast<GraphPlacerNode *>(pnode2)->setColor(node2->color());
          dynamic_cast<GraphPlacerNode *>(pnode2)->setLabel(node2->label());
        }

        auto *pedge = placer->addEdge(CGraphPlacer::OptReal(0.0), pnode1, pnode2);

        pnode1->addDestEdge(pedge);
        pnode2->addSrcEdge (pedge);
      }
    }
  }

  auto *pgraph = dynamic_cast<CGraphPlacerGraph *>(graphPlacerGraph_);

  for (const auto &nn : placer->namedNodes()) {
    auto node = nn.second;

    pgraph->addNode(node);
  }

  pgraph->place(CBBox2D(-1.0, -1.0, 1.0, 1.0));

//pgraph->print(std::cerr);
}

void
CQGraph::
animate()
{
  if (! parse_)
    return;

  assert(packType() == PackType::FORCE_DIRECTED);

  auto *forceDirected = forceDirected_;
  if (! forceDirected) return;

  int    animateteps { 100 };
  double stepSize    { 0.01 };

  for (int i = 0; i < animateteps; ++i)
    forceDirected->step(stepSize);

  update();
}

//------

CQGraphCanvas::
CQGraphCanvas(CQGraph *graph) :
 graph_(graph)
{
  setFocusPolicy(Qt::StrongFocus);

  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

  setMouseTracking(true);
}

CQGraphCanvas::
~CQGraphCanvas()
{
}

void
CQGraphCanvas::
mouseMoveEvent(QMouseEvent *me)
{
  mousePos_ = me->pos();

  graph_->status()->update();
}

void
CQGraphCanvas::
resizeEvent(QResizeEvent *)
{
  if      (graph_->packType() == CQGraph::PackType::FORCE_DIRECTED) {
  }
  else if (graph_->packType() == CQGraph::PackType::CIRCLE_PACK) {
  }
  else if (graph_->packType() == CQGraph::PackType::GRAPH_PLACER) {
    graph_->createGraphPlacer();

    auto *graphPlacer = graph_->graphPlacer();

    graphPlacer->setSize(width(), height());

    graph_->initGraphPlacer();
  }
}

void
CQGraphCanvas::
paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  if      (graph_->packType() == CQGraph::PackType::FORCE_DIRECTED) {
    drawForceDirected(&painter);
  }
  else if (graph_->packType() == CQGraph::PackType::CIRCLE_PACK) {
    drawCirclePack(&painter);
  }
  else if (graph_->packType() == CQGraph::PackType::GRAPH_PLACER) {
    drawGraphPlacer(&painter);
  }
}

void
CQGraphCanvas::
drawForceDirected(QPainter *painter)
{
  auto *forceDirected = graph_->forceDirected();
  if (! forceDirected) return;

  double xmin, ymin, xmax, ymax;

  forceDirected->calcRange(xmin, ymin, xmax, ymax);

  double m = graph_->margin();

  CDisplayRange2D range(m, m, width() - m, height() - m, xmin, ymin, xmax, ymax);

  //std::cerr << xmin << " " << ymin << " " << xmax << " " << ymax << "\n";

  double r = 32;

  auto drawNode = [&](CForceDirectedSpringNode *node, const Springy::Vector &p) {
    double px, py;

    range.windowToPixel(p.x(), p.y(), &px, &py);

    QRectF rect(px - r, py - r, 2*r, 2*r);

    painter->setPen  (Qt::NoPen);
    painter->setBrush(QColor(200, 200, 200));

    painter->drawEllipse(rect);

    painter->setPen(Qt::black);

    painter->drawText(rect, Qt::AlignCenter, QString::fromStdString(node->name()));
  };

  auto movePointOnLine = [](const QPointF &p1, const QPointF &p2, double d) {
    double a = atan2(p2.y() - p1.y(), p2.x() - p1.x());

    double c = std::cos(a);
    double s = std::sin(a);

    return QPointF(p1.x() + d*c, p1.y() + d*s);
  };

  auto drawEdge = [&](const QPointF &p1, const QPointF &p2) {
    auto pp1 = movePointOnLine(p1, p2, r);
    auto pp2 = movePointOnLine(p2, p1, r);

    painter->setPen(QColor(20, 20, 20));

    painter->drawLine(pp1, pp2);
  };

#if 0
  for (const auto &ng : parse_->graphs()) {
    auto graph = ng.second;

    for (const auto &nn : graph->nodes()) {
      auto *node1 = dynamic_cast<CForceDirectedSpringNode *>(nn.second.get());

      auto p1 = forceDirected->point(dynamic_cast<Springy::Node *>(node1))->p();

      drawNode(node1, p1);

      for (const auto &edge : node1->edges()) {
        auto *node2 = dynamic_cast<CForceDirectedSpringNode *>(edge->toNode());

        auto p2 = forceDirected->point(dynamic_cast<Springy::Node *>(node2))->p();

        drawNode(node2, p2);

        //std::cerr << p1.x() << " " << p1.y() << " " << p2.x() << " " << p2.y() << "\n";

        double px1, py1, px2, py2;

        range.windowToPixel(p1.x(), p1.y(), &px1, &py1);
        range.windowToPixel(p2.x(), p2.y(), &px2, &py2);

        drawEdge(QPointF(px1, py1), QPointF(px2, py2));

        //std::cerr << x1 << " " << y1 << " " << x2 << " " << y2 << "\n";
      }
    }
  }
#else
  for (auto &node : forceDirected->nodes()) {
    auto point = forceDirected->point(node);

    const auto &p1 = point->p();

    drawNode(dynamic_cast<CForceDirectedSpringNode *>(node.get()), p1);
  }

  for (auto &edge : forceDirected->edges()) {
    bool isTemp = false;

    auto spring = forceDirected->spring(edge, isTemp);

    const auto &p1 = spring->point1()->p();
    const auto &p2 = spring->point2()->p();

    double px1, py1, px2, py2;

    range.windowToPixel(p1.x(), p1.y(), &px1, &py1);
    range.windowToPixel(p2.x(), p2.y(), &px2, &py2);

    drawEdge(QPointF(px1, py1), QPointF(px2, py2));
  }
#endif
}

void
CQGraphCanvas::
drawCirclePack(QPainter *painter)
{
  auto *circlePack = static_cast<CirclePack *>(graph_->circlePack());
  if (! circlePack) return;

  double xmin, ymin, xmax, ymax;

  circlePack->boundingBox(xmin, ymin, xmax, ymax);

  double m = graph_->margin();

  CDisplayRange2D range(m, m, width() - m, height() - m, xmin, ymin, xmax, ymax);

  //std::cerr << xmin << " " << ymin << " " << xmax << " " << ymax << "\n";

  auto drawNode = [&](CirclePackNode *node) {
    auto r = node->radius();

    double px1, py1, px2, py2;

    range.windowToPixel(node->x() - r, node->y() - r, &px1, &py1);
    range.windowToPixel(node->x() + r, node->y() + r, &px2, &py2);

    QRectF rect(px1, py1, px2 - px1, py2 - py1);

    painter->setPen  (Qt::NoPen);
    painter->setBrush(QColor(200, 200, 200));

    painter->drawEllipse(rect);

    painter->setPen(Qt::black);

    painter->drawText(rect, Qt::AlignCenter, QString::fromStdString(node->name()));
  };

  for (const auto &node : circlePack->nodes()) {
    drawNode(node);
  }
}

void
CQGraphCanvas::
drawGraphPlacer(QPainter *painter)
{
  auto *graph = graph_->graphPlacerGraph();
  if (! graph) return;

  auto rect = graph->nodesBBox();
  if (! rect.isSet()) return;

  double m = graph_->margin();

  //double xmin { 0 }, ymin { 0 }, xmax { 1.0 }, ymax { 1.0 };
  double xmin = rect.getXMin(), ymin = rect.getYMin();
  double xmax = rect.getXMax(), ymax = rect.getYMax();

//CDisplayRange2D range(m, height() - m - 1, width() - m - 1, m, xmin, ymin, xmax, ymax);
  CDisplayRange2D range(m, m, width() - m - 1, height() - m - 1, xmin, ymin, xmax, ymax);

  auto rectToPixel = [&](const CBBox2D &rect) {
    double px1, py1, px2, py2;

    range.windowToPixel(rect.getXMin(), rect.getYMin(), &px1, &py2);
    range.windowToPixel(rect.getXMax(), rect.getYMax(), &px2, &py1);

    return QRectF(px1, py1, px2 - px1, py2 - py1);
  };

  auto edgeColor = QColor(20, 20, 20);
  auto edgeWidth = 1;

  auto qrect = rectToPixel(rect);

  painter->setPen  (Qt::red);
  painter->setBrush(QColor(240, 240, 240));

  painter->drawRect(qrect);

  auto movePointOnLine = [](const QPointF &p1, const QPointF &p2, double d) {
    double a = atan2(p2.y() - p1.y(), p2.x() - p1.x());

    double c = std::cos(a);
    double s = std::sin(a);

    return QPointF(p1.x() + d*c, p1.y() + d*s);
  };

  auto movePointPerpLine = [](const QPointF &p1, const QPointF &p2, double d) {
    double a = atan2(p2.y() - p1.y(), p2.x() - p1.x());

    double c = std::cos(a);
    double s = std::sin(a);

    return QPointF(p1.x() + d*s, p1.y() - d*c);
  };

  auto drawNode = [&](CGraphPlacerNode *node) {
    auto nrect = node->rect();
    if (! nrect.isSet()) return;

    auto rect = rectToPixel(nrect);

    const auto &color = dynamic_cast<GraphPlacerNode *>(node)->color();

    auto fc = (color != "" ? QColor(QString::fromStdString(color)) : QColor(200, 200, 240));
    auto pc = (qGray(fc.rgb()) > 127 ? QColor(40, 40, 40) : QColor(220, 220, 220));

    painter->setPen  (pc);
    painter->setBrush(fc);

    painter->drawRect(rect);

    auto tc = (qGray(fc.rgb()) > 127 ? QColor(0, 0, 0) : QColor(255, 255, 255));

    painter->setPen(tc);

    auto label = dynamic_cast<GraphPlacerNode *>(node)->label();

    if (label == "")
      label = node->name();

    painter->drawText(rect, Qt::AlignCenter, QString::fromStdString(label));
  };

  auto drawEdge = [&](CGraphPlacerNode *node1, CGraphPlacerNode *node2, bool center=false) {
    auto nrect1 = node1->rect();
    auto nrect2 = node2->rect();
    if (! nrect1.isSet() || ! nrect2.isSet()) return;

    double px1, py1, px2, py2;

    if (! center) {
      range.windowToPixel(nrect1.getXMax(), nrect1.getYMid(), &px1, &py1);
      range.windowToPixel(nrect2.getXMin(), nrect2.getYMid(), &px2, &py2);
    }
    else {
      range.windowToPixel(nrect1.getXMid(), nrect1.getYMid(), &px1, &py1);
      range.windowToPixel(nrect2.getXMid(), nrect2.getYMid(), &px2, &py2);
    }

    auto xm = (px1 + px2)/2.0;
    auto ym = (py1 + py2)/2.0;

    auto pam = QPointF(xm, ym);
    auto pa1 = movePointOnLine  (pam, QPointF(px2, py2), -8);
    auto pa2 = movePointPerpLine(pa1, QPointF(px2, py2), -8);
    auto pa3 = movePointPerpLine(pa1, QPointF(px2, py2),  8);

    QPen pen;

    pen.setColor(edgeColor);
    pen.setWidth(edgeWidth);

    painter->setPen(pen);

    painter->drawLine(QPointF(px1, py1), QPointF(px2, py2));

    painter->drawLine(pam, pa2);
    painter->drawLine(pam, pa3);
  };

  auto drawGraph = [&](CGraphPlacerGraph *graph) {
    for (auto node : graph->nodes()) {
      drawNode(node.get());

      for (auto *edge : node->srcEdges()) {
        drawEdge(edge->srcNode(), node.get(), /*center*/true);
      }

      for (auto *edge : node->destEdges()) {
        drawEdge(node.get(), edge->destNode(), /*center*/true);
      }
    }
  };

  //---

  edgeColor = Qt::blue;
  edgeWidth = 2;

  drawGraph(graph);

  //---

  edgeColor = Qt::red;
  edgeWidth = 4;

  for (auto edge : graph_->minGraph()->edges()) {
    auto *edge1 = dynamic_cast<GraphPlacerDotEdge *>(edge.get());

    auto *node1 = edge1->fromNode();
    auto *node2 = edge1->toNode  ();

    auto *node1o = dynamic_cast<CGraphPlacerGraph *>(graph)->findNode(node1->name());
    auto *node2o = dynamic_cast<CGraphPlacerGraph *>(graph)->findNode(node2->name());

    drawEdge(dynamic_cast<CGraphPlacerNode *>(node1o),
             dynamic_cast<CGraphPlacerNode *>(node2o), /*center*/true);
  }

  //drawGraph(dynamic_cast<CGraphPlacerGraph *>(minGraph_));

  //---

  edgeColor = Qt::green;
  edgeWidth = 8;

  CDotParse::Node *lastNode = nullptr;

  for (auto *node : graph_->shortestPath()) {
    if (lastNode) {
      auto *node1o = dynamic_cast<CGraphPlacerGraph *>(graph)->findNode(lastNode->name());
      auto *node2o = dynamic_cast<CGraphPlacerGraph *>(graph)->findNode(node    ->name());

      drawEdge(dynamic_cast<CGraphPlacerNode *>(node1o),
               dynamic_cast<CGraphPlacerNode *>(node2o), /*center*/true);
    }

    lastNode = node;
  }
}

GraphPlacer::Graph *
GraphPlacer::
makeGraph(const std::string &name) const
{
  auto *graph = new GraphPlacerGraph(graph_, name);

  return dynamic_cast<GraphPlacer::Graph *>(graph);
}

GraphPlacer::Node *
GraphPlacer::
makeNode(const std::string &name) const
{
  auto *node = new GraphPlacerNode(graph_, name);

  return dynamic_cast<GraphPlacer::Node *>(node);
}

GraphPlacer::Edge *
GraphPlacer::
makeEdge(const OptReal &value, Node *srcNode, Node *destNode) const
{
  auto *edge = new GraphPlacerEdge(graph_, value, dynamic_cast<GraphPlacerNode *>(srcNode),
                                   dynamic_cast<GraphPlacerNode *>(destNode));

  return dynamic_cast<GraphPlacer::Edge *>(edge);
}

//------

CQGraphStatus::
CQGraphStatus(CQGraph *graph) :
 graph_(graph)
{
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

CQGraphStatus::
~CQGraphStatus()
{
}

void
CQGraphStatus::
paintEvent(QPaintEvent *)
{
  auto *placer = graph_->graphPlacer();
  if (! placer) return;

  //auto *graph = graph_->graphPlacerGraph();

  auto pos = graph_->canvas()->mousePos();

  double wx, wy;

  placer->pixelToWindow(pos.x(), pos.y(), wx, wy);

  auto str = QString("(%1, %2)").arg(wx).arg(wy);

  QPainter painter(this);

  QFontMetrics fm(font());

  painter.drawText(2, fm.ascent() + 2, str);
}

QSize
CQGraphStatus::
sizeHint() const
{
  QFontMetrics fm(font());

  return QSize(100, fm.height() + 4);
}
