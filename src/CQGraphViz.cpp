#include <CQGraphViz.h>
#include <CJson.h>
#include <CDotParse.h>
//#include <CStrParse.h>

#include <QPainterPath>
#include <QRectF>

//---

namespace CQGraphViz {

App::
App()
{
  root_ = std::make_shared<Object>();
}

bool
App::
processJson(const std::string &filename)
{
  auto *json = new CJson;

  CJson::ValueP value;

  if (! json->loadFile(filename.c_str(), value)) {
    errorMsg("Parse failed");
    return false;
  }

  auto decodePoints = [](CJson::Array *array) {
    std::vector<double> coords;

    for (const auto &point : array->values()) {
      auto *pointArray = point->cast<CJson::Array>();

      for (const auto &xy : pointArray->values()) {
        auto p = xy->cast<CJson::Number>()->value();

        coords.push_back(p);
      }
    }

    Points points;

    int np = coords.size()/2;

    for (int ip = 0; ip < np; ++ip) {
      double x = coords[ip*2 + 0];
      double y = coords[ip*2 + 1];

      QPointF p(x, y);

      points.push_back(p);
    }

    return points;
  };

  auto pointsToPath = [](const Points &points) {
    QPainterPath path;

    int np = points.size();

    for (int ip = 0; ip < np; ++ip) {
      auto &p = points[ip];

      if (ip == 0)
        path.moveTo(p);
      else
        path.lineTo(p);
    }

    return path;
  };

  auto pointsToBSpline = [](const Points &points) {
    QPainterPath path;

    int np = points.size();

    for (int ip = 0; ip < np; ++ip) {
      auto &p = points[ip];

      if      (ip == 0)
        path.moveTo(p);
      else if (ip < np - 2) {
        path.cubicTo(p, points[ip + 1], points[ip + 2]);

        ip += 2;
      }
      else
        path.lineTo(p);
    }

    return path;
  };

  auto decodeRect = [](CJson::Array *array) {
    std::vector<double> coords;

    for (const auto &point : array->values()) {
      auto p = point->cast<CJson::Number>()->value();

      coords.push_back(p);
    }

    assert(coords.size() == 4);

    return QRectF(coords[0] - coords[2], coords[1] - coords[3], 2*coords[2], 2*coords[3]);
  };

  auto rectToEllipse = [](const QRectF &rect) {
    QPainterPath path;

    path.addEllipse(rect);

    return path;
  };

  auto decodePoint = [](CJson::Array *array) {
    std::vector<double> coords;

    for (const auto &point : array->values()) {
      auto p = point->cast<CJson::Number>()->value();

      coords.push_back(p);
    }

    assert(coords.size() == 2);

    QPointF p(coords[0], coords[1]);

    return p;
  };

  auto decodePos = [&](const std::string &s) {
    auto qstr = QString::fromStdString(s);

    auto strs = qstr.split(",");

    if (strs.size() == 2) {
      bool ok;

      auto x = strs[0].toDouble(&ok); assert(ok);
      auto y = strs[1].toDouble(&ok); assert(ok);

      return QPointF(x, y);
    }
    else {
      errorMsg("Unhandled pos: " + s);
      return QPointF();
    }
  };

  auto decodeRealString = [](const std::string &s) {
    auto qstr = QString::fromStdString(s);

    bool ok;
    double r = qstr.toDouble(&ok); assert(ok);

    return r;
  };

  auto getHexValue = [](const std::string &str) {
    static std::string xchars = "0123456789abcdef";

    int len = str.size();

    int hvalue = 0;

    for (int i = 0; i < len; ++i) {
      char c1 = std::tolower(str[i]);

      auto p = xchars.find(c1);

      hvalue = hvalue*16 + int(p);
    }

    return hvalue;
  };

  auto decodeColor = [&](const std::string &str) {
    QColor color;

    if (str.size() == 9) {
      color = QString::fromStdString(str.substr(0, 7));

      auto alpha = getHexValue(str.substr(7));

      color.setAlpha(alpha);
    }
    else
      color = QString::fromStdString(str);

    return color;
  };

  auto decodeStyle = [&](const std::string &str, StyleData &style) {
    if      (str == "solid") {
      style.lineStyle = LineStyle::SOLID;
    }
    else if (str == "dotted") {
      style.lineStyle = LineStyle::DOTTED;
    }
    else if (str == "dashed") {
      style.lineStyle = LineStyle::DASHED;
    }
    else if (str.substr(0, 13) == "setlinewidth(") {
      int n = str.size();

      std::string sstr;

      for (int i = 13; i < n && std::isdigit(str[i]); ++i)
        sstr += str[i];

      style.lineWidth = std::stoi(sstr);
    }
    else {
      errorMsg("Invalid style: " + str);
    }
  };

  auto decodeAlign = [&](const std::string &str) {
    if      (str == "l")
      return Qt::AlignLeft;
    else if (str == "c")
      return Qt::AlignHCenter;
    else if (str == "r")
      return Qt::AlignRight;
    else {
      errorMsg("Invalid align: " + str);
      return Qt::AlignHCenter;
    }
  };

  auto objToQString = [](const CJson::ValueP &value) {
    return QString::fromStdString(value->cast<CJson::String>()->value());
  };

#if 0
  auto stringToBool = [](const QString &str) {
    if (str == "true" ) return true;
    if (str == "false") return false;
    errorMsg("Unhandled bool : " + str.toStdString());
    return false;
  };
#endif

  auto stringToRect = [](const QString &str) {
    auto strs = str.split(",");
    assert(strs.size() == 4);

    bool ok;
    auto x1 = strs[0].toDouble(&ok); assert(ok);
    auto y1 = strs[1].toDouble(&ok); assert(ok);
    auto x2 = strs[2].toDouble(&ok); assert(ok);
    auto y2 = strs[3].toDouble(&ok); assert(ok);

    return QRectF(x1, y1, x2 - x1, y2 - y1);
  };

  // ops:
  // "S"  : style
  // "t"  : font_style
  // "pP" : polygon
  // "T"  : text
  // "eE" : ellipse
  // "bB" : bspline
  // "L"  : polyline
  // "F"  : font

  if (value->isObject()) {
    auto *obj = value->cast<CJson::Object>();

    for (const auto &nv : obj->nameValueArray()) {
      if      (nv.first == "bb") {
        setBBox(stringToRect(objToQString(nv.second)));
      }
      else if (nv.first == "bgcolor") {
      }
      else if (nv.first == "center") {
      }
      else if (nv.first == "charset") {
      }
      else if (nv.first == "color") {
      }
      else if (nv.first == "directed") {
        directed_ = nv.second->toBool();
      }
      else if (nv.first == "fontcolor") {
      }
      else if (nv.first == "fontname") {
      }
      else if (nv.first == "fontsize") {
      }
      else if (nv.first == "gradientangle") {
      }
      else if (nv.first == "label") {
        root_->setLabel(objToQString(nv.second));
      }
      else if (nv.first == "labeljust") {
      }
      else if (nv.first == "lwidth") {
      }
      else if (nv.first == "lheight") {
      }
      else if (nv.first == "lp") {
      }
      else if (nv.first == "margin") {
      }
      else if (nv.first == "name") {
        root_->setName(objToQString(nv.second));
      }
      else if (nv.first == "nodesep") {
      }
      else if (nv.first == "ordering") {
      }
      else if (nv.first == "orientation") {
      }
      else if (nv.first == "overlap") {
      }
      else if (nv.first == "page") {
      }
      else if (nv.first == "rankdir") {
      }
      else if (nv.first == "ranksep") {
      }
      else if (nv.first == "ratio") {
      }
      else if (nv.first == "root") {
      }
      else if (nv.first == "size") {
      }
      else if (nv.first == "splines") {
      }
      else if (nv.first == "ssize") {
      }
      else if (nv.first == "strict") {
      }
      else if (nv.first == "style") {
      }
      else if (nv.first == "truecolor") {
      }
      else if (nv.first == "xdotversion") {
      }
      else if (nv.first == "_subgraph_cnt") {
      }
      // top level draw
      else if (nv.first == "_draw_") {
        std::string op;
        ColorData   colorData;
        StyleData   styleData;

        auto *drawArray = nv.second->cast<CJson::Array>();

        for (const auto &objValue : drawArray->values()) {
          auto *drawObj = objValue->cast<CJson::Object>();

          for (const auto &nv1 : drawObj->nameValueArray()) {
            //debugMsg("_draw_ : " + nv1.first + " " + *nv1.second);

            if      (nv1.first == "op") {
              op = nv1.second->cast<CJson::String>()->value();
            }
            else if (nv1.first == "grad") {
              auto grad = nv1.second->cast<CJson::String>()->value();

              if (op == "c" || op == "C")
                colorData.grad = grad;
              else
                errorMsg(" _draw_ unhandled grad");
            }
            else if (nv1.first == "color") {
              auto color = decodeColor(nv1.second->cast<CJson::String>()->value());

              if      (op == "C")
                colorData.bg = color;
              else if (op == "c")
                colorData.fg = color;
              else
                errorMsg(" _draw_ unhandled color");
            }
            else if (nv1.first == "p0") {
            }
            else if (nv1.first == "p1") {
            }
            else if (nv1.first == "stops") {
            }
            else if (nv1.first == "style") {
              auto style = nv1.second->cast<CJson::String>()->value();

              if (op == "S") {
                if (style != "")
                  decodeStyle(style, styleData);
              }
              else
                errorMsg(" _draw_ unhandled style");
            }
            else if (nv1.first == "points") {
              auto points = decodePoints(nv1.second->cast<CJson::Array>());

              if      (op == "P" || op == "p") {
                PathData pathData;

                pathData.bg    = colorData.bg;
                pathData.fg    = colorData.fg;
                pathData.style = styleData;
                pathData.path  = pointsToPath(points);

                root_->addPath(pathData);

                colorData.reset();
              }
              else if (op == "b" || op == "B") {
                PathData bspline;

                bspline.fg    = colorData.fg;
                bspline.style = styleData;
                bspline.path  = pointsToBSpline(points);

                root_->addLine(bspline);

                colorData.reset();
              }
              else if (op == "L") {
                PathData bspline;

                bspline.fg    = colorData.fg;
                bspline.style = styleData;
                bspline.path  = pointsToBSpline(points);

                root_->addLine(bspline);

                root_->setRect(bspline.path.boundingRect());

                colorData.reset();
              }
              else
                errorMsg(" _draw_ unhandled points for op " + op);
            }
            else if (nv1.first == "rect") {
              auto rect = decodeRect(nv1.second->cast<CJson::Array>());

              if (op == "E" || op == "e") {
                PathData ellipse;

                ellipse.bg    = colorData.bg;
                ellipse.fg    = colorData.fg;
                ellipse.style = styleData;
                ellipse.path  = rectToEllipse(rect);

                root_->addPath(ellipse);

                colorData.reset();

                root_->setRect(rect);
              }
              else
                errorMsg(" _draw_ unhandled rect");
            }
            else {
              errorMsg(" _draw_ unhandled: " + nv1.first);
            }
          }
        }
      }
      else if (nv.first == "_ldraw_") {
        std::string op;
        ColorData   colorData;
        TextData    textData;
        StyleData   styleData;

        auto *drawArray = nv.second->cast<CJson::Array>();

        for (const auto &objValue1 : drawArray->values()) {
          auto *drawObj1 = objValue1->cast<CJson::Object>();

          for (const auto &nv1 : drawObj1->nameValueArray()) {
            //debugMsg("_ldraw_ : " + nv1.first + " " + *nv1.second);

            if      (nv1.first == "op") {
              op = nv1.second->cast<CJson::String>()->value();
            }
            else if (nv1.first == "grad") {
              auto grad = nv1.second->cast<CJson::String>()->value();

              if (op == "c" || op == "C")
                colorData.grad = grad;
              else
                errorMsg(" objects/_ldraw_ unhandled grad");
            }
            else if (nv1.first == "color") {
              auto color = decodeColor(nv1.second->cast<CJson::String>()->value());

              if      (op == "C")
                colorData.bg = color;
              else if (op == "c")
                colorData.fg = color;
              else
                errorMsg(" objects/_ldraw_ unhandled color");
            }
            else if (nv1.first == "p0") {
            }
            else if (nv1.first == "p1") {
            }
            else if (nv1.first == "stops") {
            }
            else if (nv1.first == "style") {
              auto style = nv1.second->cast<CJson::String>()->value();

              if (op == "S") {
                if (style != "")
                  decodeStyle(style, styleData);
              }
              else
                errorMsg(" objects/_ldraw_ unhandled style");
            }
            else if (nv1.first == "size") {
              auto size = nv1.second->cast<CJson::Number>()->value();

              if (op == "F")
                textData.size = size;
              else
                errorMsg(" objects/_ldraw_ unhandled size");
            }
            else if (nv1.first == "face") {
              auto face = objToQString(nv1.second);

              if (op == "F")
                textData.face = face;
            }
            else if (nv1.first == "pt") {
              auto pt = decodePoint(nv1.second->cast<CJson::Array>());

              if (op == "t" || op == "T")
                textData.pos = pt;
              else
                errorMsg(" objects/_ldraw_ unhandled pt");
            }
            else if (nv1.first == "align") {
              auto align = decodeAlign(nv1.second->cast<CJson::String>()->value());

              if (op == "t" || op == "T")
                textData.align = align;
              else
                errorMsg(" objects/_ldraw_ unhandled align");
            }
            else if (nv1.first == "width") {
              auto width = nv1.second->cast<CJson::Number>()->value();

              if (op == "t" || op == "T")
                textData.width = width;
              else
                errorMsg(" objects/_ldraw_ unhandled width");
            }
            else if (nv1.first == "text") {
              auto text = nv1.second->cast<CJson::String>()->value();

              if (op == "t" || op == "T") {
                textData.fg   = colorData.fg;
                textData.text = QString::fromStdString(text);

                root_->addText(textData);

                colorData.reset();
              }
              else
                errorMsg(" objects/_ldraw_ unhandled text");
            }
            else if (nv1.first == "points") {
              auto points = decodePoints(nv1.second->cast<CJson::Array>());

              if      (op == "P" || op == "p") {
                PathData pathData;

                pathData.bg    = colorData.bg;
                pathData.fg    = colorData.fg;
                pathData.style = styleData;
                pathData.path  = pointsToPath(points);

                root_->addPath(pathData);

                colorData.reset();
              }
              else if (op == "b" || op == "B") {
                PathData bspline;

                bspline.fg    = colorData.fg;
                bspline.style = styleData;
                bspline.path  = pointsToBSpline(points);

                root_->addLine(bspline);

                colorData.reset();
              }
              else if (op == "L") {
                PathData bspline;

                bspline.fg    = colorData.fg;
                bspline.style = styleData;
                bspline.path  = pointsToBSpline(points);

                root_->addLine(bspline);

                root_->setRect(bspline.path.boundingRect());

                colorData.reset();
              }
              else
                errorMsg(" objects/_ldraw_ unhandled points for op " + op);
            }
            else {
              errorMsg(" objects/_ldraw_ unhandled: " + nv1.first);
            }
          }
        }
      }
      else if (nv.first == "objects") {
        //debugMsg("Objects");

        auto *objArray = nv.second->cast<CJson::Array>();
        assert(objArray);

        for (const auto &objValue : objArray->values()) {
          auto *obj1 = objValue->cast<CJson::Object>();
          assert(obj1);

          auto object = std::make_shared<Object>();

          object->setType(Object::Type::OBJECT);

          for (const auto &nv1 : obj1->nameValueArray()) {
            if      (nv1.first == "_gvid") {
              object->setId(int(nv1.second->cast<CJson::Number>()->value()));
            }
            else if (nv1.first == "bb") {
            }
            else if (nv1.first == "bgcolor") {
            }
            else if (nv1.first == "center") {
            }
            else if (nv1.first == "color") {
            }
            else if (nv1.first == "colorscheme") {
            }
            else if (nv1.first == "distortion") {
            }
            else if (nv1.first == "edges") {
            }
            else if (nv1.first == "fillcolor") {
            }
            else if (nv1.first == "fixedsize") {
            }
            else if (nv1.first == "fontcolor") {
            }
            else if (nv1.first == "fname") {
            }
            else if (nv1.first == "fontname") {
            }
            else if (nv1.first == "fontsize") {
            }
            else if (nv1.first == "gradientangle") {
            }
            else if (nv1.first == "height") {
              object->setHeight(decodeRealString(nv1.second->cast<CJson::String>()->value()));
            }
            else if (nv1.first == "kind") {
            }
            else if (nv1.first == "label") {
              object->setLabel(objToQString(nv1.second));
            }
            else if (nv1.first == "lheight") {
            }
            else if (nv1.first == "lp") {
            }
            else if (nv1.first == "lwidth") {
            }
            else if (nv1.first == "margin") {
            }
            else if (nv1.first == "name") {
              object->setName(objToQString(nv1.second));
            }
            else if (nv1.first == "nodes") {
            }
            else if (nv1.first == "nodesep") {
            }
            else if (nv1.first == "ordering") {
            }
            else if (nv1.first == "orientation") {
            }
            else if (nv1.first == "outline") {
            }
            else if (nv1.first == "overlap") {
            }
            else if (nv1.first == "page") {
            }
            else if (nv1.first == "peripheries") {
            }
            else if (nv1.first == "pname") {
            }
            else if (nv1.first == "pos") {
              object->setPos(decodePos(nv1.second->cast<CJson::String>()->value()));
            }
            else if (nv1.first == "rank") {
            }
            else if (nv1.first == "ranksep") {
            }
            else if (nv1.first == "rankdir") {
            }
            else if (nv1.first == "ratio") {
            }
            else if (nv1.first == "rects") {
            }
            else if (nv1.first == "regular") {
            }
            else if (nv1.first == "shape") {
            }
            else if (nv1.first == "sides") {
            }
            else if (nv1.first == "size") {
            }
            else if (nv1.first == "subkind") {
            }
            else if (nv1.first == "skew") {
            }
            else if (nv1.first == "splines") {
            }
            else if (nv1.first == "style") {
            }
            else if (nv1.first == "subgraphs") {
            }
            else if (nv1.first == "tooltip") {
            }
            else if (nv1.first == "URL") {
            }
            else if (nv1.first == "width") {
              object->setWidth(decodeRealString(nv1.second->cast<CJson::String>()->value()));
            }
            else if (nv1.first == "_draw_") {
              std::string op;
              ColorData   colorData;
              StyleData   styleData;

              auto *drawArray1 = nv1.second->cast<CJson::Array>();

              for (const auto &objValue1 : drawArray1->values()) {
                auto *drawObj1 = objValue1->cast<CJson::Object>();

                for (const auto &nv2 : drawObj1->nameValueArray()) {
                  //debugMsg("objects/_draw_ : " + nv2.first + " " + *nv2.second);

                  if      (nv2.first == "op") {
                    op = nv2.second->cast<CJson::String>()->value();
                  }
                  else if (nv2.first == "grad") {
                    auto grad = nv2.second->cast<CJson::String>()->value();

                    if (op == "c" || op == "C")
                      colorData.grad = grad;
                    else
                      errorMsg(" objects/_draw_ unhandled grad");
                  }
                  else if (nv2.first == "color") {
                    auto color = decodeColor(nv2.second->cast<CJson::String>()->value());

                    if      (op == "C")
                      colorData.bg = color;
                    else if (op == "c")
                      colorData.fg = color;
                    else
                      errorMsg(" objects/_draw_ unhandled color");
                  }
                  else if (nv2.first == "p0") {
                  }
                  else if (nv2.first == "p1") {
                  }
                  else if (nv2.first == "stops") {
                  }
                  else if (nv2.first == "style") {
                    auto style = nv2.second->cast<CJson::String>()->value();

                    if (op == "S") {
                      if (style != "")
                        decodeStyle(style, styleData);
                    }
                    else
                      errorMsg(" objects/_draw_ unhandled style");
                  }
                  else if (nv2.first == "points") {
                    auto points = decodePoints(nv2.second->cast<CJson::Array>());

                    if      (op == "P" || op == "p") {
                      PathData pathData;

                      pathData.bg    = colorData.bg;
                      pathData.fg    = colorData.fg;
                      pathData.style = styleData;
                      pathData.path  = pointsToPath(points);

                      object->addPath(pathData);

                      object->setRect(pathData.path.boundingRect());

                      colorData.reset();
                    }
                    else if (op == "b" || op == "B") {
                      PathData bspline;

                      bspline.fg    = colorData.fg;
                      bspline.style = styleData;
                      bspline.path  = pointsToBSpline(points);

                      object->addLine(bspline);

                      object->setRect(bspline.path.boundingRect());

                      colorData.reset();
                    }
                    else if (op == "L") {
                      PathData bspline;

                      bspline.fg    = colorData.fg;
                      bspline.style = styleData;
                      bspline.path  = pointsToBSpline(points);

                      object->addLine(bspline);

                      object->setRect(bspline.path.boundingRect());

                      colorData.reset();
                    }
                    else
                      errorMsg(" objects/_draw_ unhandled points for op " + op);
                  }
                  else if (nv2.first == "rect") {
                    auto rect = decodeRect(nv2.second->cast<CJson::Array>());

                    if (op == "E" || op == "e") {
                      PathData ellipse;

                      ellipse.bg    = colorData.bg;
                      ellipse.fg    = colorData.fg;
                      ellipse.style = styleData;
                      ellipse.path  = rectToEllipse(rect);

                      object->addPath(ellipse);

                      object->setRect(rect);

                      colorData.reset();
                    }
                    else
                      errorMsg(" objects/_draw_ unhandled rect");
                  }
                  else {
                    errorMsg(" objects/_draw_ unhandled: " + nv2.first);
                  }
                }
              }
            }
            else if (nv1.first == "_ldraw_") {
              std::string op;
              ColorData   colorData;
              TextData    textData;
              StyleData   styleData;

              auto *drawArray1 = nv1.second->cast<CJson::Array>();

              for (const auto &objValue1 : drawArray1->values()) {
                auto *drawObj1 = objValue1->cast<CJson::Object>();

                for (const auto &nv2 : drawObj1->nameValueArray()) {
                  //debugMsg("_ldraw_ : " + nv2.first + " " + *nv2.second);

                  if      (nv2.first == "op") {
                    op = nv2.second->cast<CJson::String>()->value();
                  }
                  else if (nv2.first == "grad") {
                    auto grad = nv2.second->cast<CJson::String>()->value();

                    if (op == "c" || op == "C")
                      colorData.grad = grad;
                    else
                      errorMsg(" objects/_ldraw_ unhandled grad");
                  }
                  else if (nv2.first == "color") {
                    auto color = decodeColor(nv2.second->cast<CJson::String>()->value());

                    if      (op == "C")
                      colorData.bg = color;
                    else if (op == "c")
                      colorData.fg = color;
                    else
                      errorMsg(" objects/_ldraw_ unhandled color");
                  }
                  else if (nv2.first == "p0") {
                  }
                  else if (nv2.first == "p1") {
                  }
                  else if (nv2.first == "stops") {
                  }
                  else if (nv2.first == "style") {
                    auto style = nv2.second->cast<CJson::String>()->value();

                    if (op == "S") {
                      if (style != "")
                        decodeStyle(style, styleData);
                    }
                    else
                      errorMsg(" objects/_ldraw_ unhandled style");
                  }
                  else if (nv2.first == "size") {
                    auto size = nv2.second->cast<CJson::Number>()->value();

                    if (op == "F")
                      textData.size = size;
                    else
                      errorMsg(" objects/_ldraw_ unhandled size");
                  }
                  else if (nv2.first == "face") {
                    auto face = nv2.second->cast<CJson::String>()->value();

                    if (op == "F")
                      textData.face = QString::fromStdString(face);
                  }
                  else if (nv2.first == "pt") {
                    auto pt = decodePoint(nv2.second->cast<CJson::Array>());

                    if (op == "t" || op == "T")
                      textData.pos = pt;
                    else
                      errorMsg(" objects/_ldraw_ unhandled pt");
                  }
                  else if (nv2.first == "align") {
                    auto align = decodeAlign(nv2.second->cast<CJson::String>()->value());

                    if (op == "t" || op == "T")
                      textData.align = align;
                    else
                      errorMsg(" objects/_ldraw_ unhandled align");
                  }
                  else if (nv2.first == "width") {
                    auto width = nv2.second->cast<CJson::Number>()->value();

                    if (op == "t" || op == "T")
                      textData.width = width;
                    else
                      errorMsg(" objects/_ldraw_ unhandled width");
                  }
                  else if (nv2.first == "text") {
                    auto text = nv2.second->cast<CJson::String>()->value();

                    if (op == "t" || op == "T") {
                      textData.fg   = colorData.fg;
                      textData.text = QString::fromStdString(text);

                      object->addText(textData);

                      colorData.reset();
                    }
                    else
                      errorMsg(" objects/_ldraw_ unhandled text");
                  }
                  else if (nv2.first == "points") {
                    auto points = decodePoints(nv2.second->cast<CJson::Array>());

                    if      (op == "P" || op == "p") {
                      PathData pathData;

                      pathData.bg    = colorData.bg;
                      pathData.fg    = colorData.fg;
                      pathData.style = styleData;
                      pathData.path  = pointsToPath(points);

                      object->addPath(pathData);

                      object->setRect(pathData.path.boundingRect());

                      colorData.reset();
                    }
                    else if (op == "b" || op == "B") {
                      PathData bspline;

                      bspline.fg    = colorData.fg;
                      bspline.style = styleData;
                      bspline.path  = pointsToBSpline(points);

                      object->addLine(bspline);

                      object->setRect(bspline.path.boundingRect());

                      colorData.reset();
                    }
                    else if (op == "L") {
                      PathData bspline;

                      bspline.fg    = colorData.fg;
                      bspline.style = styleData;
                      bspline.path  = pointsToBSpline(points);

                      object->addLine(bspline);

                      object->setRect(bspline.path.boundingRect());

                      colorData.reset();
                    }
                    else
                      errorMsg(" objects/_ldraw_ unhandled points for op " + op);
                  }
                  else {
                    errorMsg(" objects/_ldraw_ unhandled: " + nv2.first);
                  }
                }
              }
            }
            else {
              errorMsg(" objects unhandled: " + nv1.first);
            }
          }

          objects_.push_back(object);
        }
      }
      else if (nv.first == "edges") {
        //debugMsg("Edges");

        auto *edgeArray = nv.second->cast<CJson::Array>();
        assert(edgeArray);

        for (const auto &edgeValue : edgeArray->values()) {
          auto *obj1 = edgeValue->cast<CJson::Object>();
          assert(obj1);

          auto edge = std::make_shared<Object>();

          edge->setType(Object::Type::EDGE);

          for (const auto &nv1 : obj1->nameValueArray()) {
            if      (nv1.first == "_gvid") {
              edge->setId(int(nv1.second->cast<CJson::Number>()->value()));
            }
            else if (nv1.first == "arrowhead") {
            }
            else if (nv1.first == "arrowsize") {
            }
            else if (nv1.first == "arrowtail") {
            }
            else if (nv1.first == "color") {
            }
            else if (nv1.first == "dir") {
            }
            else if (nv1.first == "edgeURL") {
            }
            else if (nv1.first == "f") {
            }
            else if (nv1.first == "fillcolor") {
            }
            else if (nv1.first == "fontcolor") {
            }
            else if (nv1.first == "fontname") {
            }
            else if (nv1.first == "fontsize") {
            }
            else if (nv1.first == "head") { // to
              int id = int(nv1.second->cast<CJson::Number>()->value());

              auto *obj = findObject(id);

              if (obj)
                obj->addDestEdge(edge.get());

              edge->setHeadId(id);
            }
            else if (nv1.first == "headclip") {
            }
            else if (nv1.first == "headlabel") {
            }
            else if (nv1.first == "head_lp") {
            }
            else if (nv1.first == "headport") {
            }
            else if (nv1.first == "id") {
            }
            else if (nv1.first == "label") {
              edge->setLabel(objToQString(nv1.second));
            }
            else if (nv1.first == "labelangle") {
            }
            else if (nv1.first == "labeldistance") {
            }
            else if (nv1.first == "labelfontsize") {
            }
            else if (nv1.first == "lp") {
            }
            else if (nv1.first == "minlen") {
            }
            else if (nv1.first == "pos") {
              //edge->setPos(decodePos(nv1.second->cast<CJson::String>()->value()));
            }
            else if (nv1.first == "samearrowhead") {
            }
            else if (nv1.first == "samearrowtail") {
            }
            else if (nv1.first == "samehead") {
            }
            else if (nv1.first == "sametail") {
            }
            else if (nv1.first == "style") {
            }
            else if (nv1.first == "tail") { // from
              int id = int(nv1.second->cast<CJson::Number>()->value());

              auto *obj = findObject(id);

              if (obj)
                obj->addSrcEdge(edge.get());

              edge->setTailId(id);
            }
            else if (nv1.first == "tailclip") {
            }
            else if (nv1.first == "taillabel") {
            }
            else if (nv1.first == "tailport") {
            }
            else if (nv1.first == "tail_lp") {
            }
            else if (nv1.first == "weight") {
            }
            else if (nv1.first == "wt") {
            }
            else if (nv1.first == "_draw_") {
              std::string op;
              ColorData   colorData;
              StyleData   styleData;

              auto *drawArray1 = nv1.second->cast<CJson::Array>();

              for (const auto &objValue1 : drawArray1->values()) {
                auto *drawObj1 = objValue1->cast<CJson::Object>();

                for (const auto &nv2 : drawObj1->nameValueArray()) {
                  //debugMsg("edges/_draw_: " + nv2.first + *nv2.second);

                  if      (nv2.first == "op") {
                    op = nv2.second->cast<CJson::String>()->value();
                  }
                  else if (nv2.first == "grad") {
                    auto grad = nv2.second->cast<CJson::String>()->value();

                    if (op == "c" || op == "C")
                      colorData.grad = grad;
                    else
                      errorMsg(" edges/_draw_ unhandled grad");
                  }
                  else if (nv2.first == "color") {
                    auto color = decodeColor(nv2.second->cast<CJson::String>()->value());

                    if      (op == "C")
                      colorData.bg = color;
                    else if (op == "c")
                      colorData.fg = color;
                    else
                      errorMsg(" edges/_draw_ unhandled color");
                  }
                  else if (nv2.first == "p0") {
                  }
                  else if (nv2.first == "p1") {
                  }
                  else if (nv2.first == "stops") {
                  }
                  else if (nv2.first == "style") {
                    auto style = nv2.second->cast<CJson::String>()->value();

                    if (op == "S") {
                      if (style != "")
                        decodeStyle(style, styleData);
                    }
                    else
                      errorMsg(" edges/_draw_ unhandled style");
                  }
                  else if (nv2.first == "points") {
                    auto points = decodePoints(nv2.second->cast<CJson::Array>());

                    if      (op == "P" || op == "p") {
                      PathData pathData;

                      pathData.bg    = colorData.bg;
                      pathData.fg    = colorData.fg;
                      pathData.style = styleData;
                      pathData.path  = pointsToPath(points);

                      edge->addPath(pathData);

                      edge->setRect(pathData.path.boundingRect());

                      colorData.reset();
                    }
                    else if (op == "b" || op == "B") {
                      PathData bspline;

                      bspline.fg    = colorData.fg;
                      bspline.style = styleData;
                      bspline.path  = pointsToBSpline(points);

                      edge->addLine(bspline);

                      edge->setRect(bspline.path.boundingRect());

                      colorData.reset();
                    }
                    else if (op == "L") {
                      PathData bspline;

                      bspline.fg    = colorData.fg;
                      bspline.style = styleData;
                      bspline.path  = pointsToBSpline(points);

                      edge->addLine(bspline);

                      edge->setRect(bspline.path.boundingRect());

                      colorData.reset();
                    }
                    else
                      errorMsg(" edges/_draw_ unhandled points for op " + op);
                  }
                  else if (nv2.first == "rect") {
                    auto rect = decodeRect(nv2.second->cast<CJson::Array>());

                    if (op == "E" || op == "e") {
                      PathData ellipse;

                      ellipse.bg    = colorData.bg;
                      ellipse.fg    = colorData.fg;
                      ellipse.style = styleData;
                      ellipse.path  = rectToEllipse(rect);

                      edge->addPath(ellipse);

                      edge->setRect(rect);

                      colorData.reset();
                    }
                    else
                      errorMsg(" edges/_draw_ unhandled rect");
                  }
                  else {
                    errorMsg(" edges/_draw_ unhandled: " + nv2.first);
                  }
                }
              }
            }
            else if (nv1.first == "_hdraw_") {
              std::string op;
              ColorData   colorData;
              StyleData   styleData;

              auto *drawArray1 = nv1.second->cast<CJson::Array>();

              for (const auto &objValue1 : drawArray1->values()) {
                auto *drawObj1 = objValue1->cast<CJson::Object>();

                for (const auto &nv2 : drawObj1->nameValueArray()) {
                  //debugMsg("edges/_hdraw_ : " + nv2.first + " " + *nv2.second);

                  if      (nv2.first == "op") {
                    op = nv2.second->cast<CJson::String>()->value();
                  }
                  else if (nv2.first == "grad") {
                    auto grad = nv2.second->cast<CJson::String>()->value();

                    if (op == "c" || op == "C")
                      colorData.grad = grad;
                    else
                      errorMsg(" edges/_hdraw_ unhandled grad");
                  }
                  else if (nv2.first == "color") {
                    auto color = decodeColor(nv2.second->cast<CJson::String>()->value());

                    if      (op == "C")
                      colorData.bg = color;
                    else if (op == "c")
                      colorData.fg = color;
                    else
                      errorMsg(" edges/_hdraw_ unhandled color");
                  }
                  else if (nv2.first == "p0") {
                  }
                  else if (nv2.first == "p1") {
                  }
                  else if (nv2.first == "stops") {
                  }
                  else if (nv2.first == "style") {
                    auto style = nv2.second->cast<CJson::String>()->value();

                    if (op == "S") {
                      if (style != "")
                        decodeStyle(style, styleData);
                    }
                    else
                      errorMsg(" edges/_hdraw_ unhandled style");
                  }
                  else if (nv2.first == "points") {
                    auto points = decodePoints(nv2.second->cast<CJson::Array>());

                    if      (op == "P" || op == "p") {
                      PathData pathData;

                      pathData.bg    = colorData.bg;
                      pathData.fg    = colorData.fg;
                      pathData.style = styleData;
                      pathData.path  = pointsToPath(points);

                      edge->addPath(pathData);

                      edge->setRect(pathData.path.boundingRect());

                      colorData.reset();
                    }
                    else if (op == "b" || op == "B") {
                      PathData bspline;

                      bspline.fg    = colorData.fg;
                      bspline.style = styleData;
                      bspline.path  = pointsToBSpline(points);

                      edge->addLine(bspline);

                      edge->setRect(bspline.path.boundingRect());

                      colorData.reset();
                    }
                    else if (op == "L") {
                      PathData bspline;

                      bspline.fg    = colorData.fg;
                      bspline.style = styleData;
                      bspline.path  = pointsToBSpline(points);

                      edge->addLine(bspline);

                      edge->setRect(bspline.path.boundingRect());

                      colorData.reset();
                    }
                    else
                      errorMsg(" edges/_hdraw_ unhandled points for op " + op);
                  }
                  else if (nv2.first == "rect") {
                    auto rect = decodeRect(nv2.second->cast<CJson::Array>());

                    if (op == "E" || op == "e") {
                      PathData ellipse;

                      ellipse.bg    = colorData.bg;
                      ellipse.fg    = colorData.fg;
                      ellipse.style = styleData;
                      ellipse.path  = rectToEllipse(rect);

                      edge->addPath(ellipse);

                      edge->setRect(rect);

                      colorData.reset();
                    }
                    else
                      errorMsg(" edges/_draw_ unhandled rect");
                  }
                  else {
                    errorMsg(" edges/_hdraw_ unhandled: " + nv2.first);
                  }
                }
              }
            }
            else if (nv1.first == "_ldraw_") {
              std::string op;
              ColorData   colorData;
              TextData    textData;
              StyleData   styleData;

              auto *drawArray1 = nv1.second->cast<CJson::Array>();

              for (const auto &objValue1 : drawArray1->values()) {
                auto *drawObj1 = objValue1->cast<CJson::Object>();

                for (const auto &nv2 : drawObj1->nameValueArray()) {
                  //debugMsg("objects/_ldraw_ : " + nv2.first + " " + *nv2.second);

                  if      (nv2.first == "op") {
                    op = nv2.second->cast<CJson::String>()->value();
                  }
                  else if (nv2.first == "grad") {
                    auto grad = nv2.second->cast<CJson::String>()->value();

                    if (op == "c" || op == "C")
                      colorData.grad = grad;
                    else
                      errorMsg(" edges/_ldraw_ unhandled grad");
                  }
                  else if (nv2.first == "color") {
                    auto color = decodeColor(nv2.second->cast<CJson::String>()->value());

                    if      (op == "C")
                      colorData.bg = color;
                    else if (op == "c")
                      colorData.fg = color;
                    else
                      errorMsg(" edges/_ldraw_ unhandled color");
                  }
                  else if (nv2.first == "p0") {
                  }
                  else if (nv2.first == "p1") {
                  }
                  else if (nv2.first == "stops") {
                  }
                  else if (nv2.first == "style") {
                    auto style = nv2.second->cast<CJson::String>()->value();

                    if (op == "S") {
                      if (style != "")
                        decodeStyle(style, styleData);
                    }
                    else
                      errorMsg(" edges/_ldraw_ unhandled style");
                  }
                  else if (nv2.first == "size") {
                    auto size = nv2.second->cast<CJson::Number>()->value();

                    if (op == "F")
                      textData.size = size;
                    else
                      errorMsg(" edges/_ldraw_ unhandled size");
                  }
                  else if (nv2.first == "face") {
                    auto face = nv2.second->cast<CJson::String>()->value();

                    if (op == "F")
                      textData.face = QString::fromStdString(face);
                  }
                  else if (nv2.first == "pt") {
                    auto pt = decodePoint(nv2.second->cast<CJson::Array>());

                    if (op == "t" || op == "T")
                      textData.pos = pt;
                    else
                      errorMsg(" edges/_ldraw_ unhandled pt");
                  }
                  else if (nv2.first == "align") {
                    auto align = decodeAlign(nv2.second->cast<CJson::String>()->value());

                    if (op == "t" || op == "T")
                      textData.align = align;
                    else
                      errorMsg(" edges/_ldraw_ unhandled align");
                  }
                  else if (nv2.first == "width") {
                    auto width = nv2.second->cast<CJson::Number>()->value();

                    if (op == "t" || op == "T")
                      textData.width = width;
                    else
                      errorMsg(" edges/_ldraw_ unhandled width");
                  }
                  else if (nv2.first == "text") {
                    auto text = nv2.second->cast<CJson::String>()->value();

                    if (op == "t" || op == "T") {
                      textData.fg   = colorData.fg;
                      textData.text = QString::fromStdString(text);

                      edge->addText(textData);

                      colorData.reset();
                    }
                    else
                      errorMsg(" edges/_ldraw_ unhandled text");
                  }
                  else if (nv2.first == "points") {
                    auto points = decodePoints(nv2.second->cast<CJson::Array>());

                    if      (op == "P" || op == "p") {
                      PathData pathData;

                      pathData.bg    = colorData.bg;
                      pathData.fg    = colorData.fg;
                      pathData.style = styleData;
                      pathData.path  = pointsToPath(points);

                      edge->setRect(pathData.path.boundingRect());

                      edge->addPath(pathData);

                      colorData.reset();
                    }
                    else if (op == "b" || op == "B") {
                      PathData bspline;

                      bspline.fg    = colorData.fg;
                      bspline.style = styleData;
                      bspline.path  = pointsToBSpline(points);

                      edge->addLine(bspline);

                      edge->setRect(bspline.path.boundingRect());

                      colorData.reset();
                    }
                    else if (op == "L") {
                      PathData bspline;

                      bspline.fg    = colorData.fg;
                      bspline.style = styleData;
                      bspline.path  = pointsToBSpline(points);

                      edge->addLine(bspline);

                      edge->setRect(bspline.path.boundingRect());

                      colorData.reset();
                    }
                    else
                      errorMsg(" edges/_ldraw_ unhandled points for op " + op);
                  }
                  else {
                    errorMsg(" edges/_ldraw_ unhandled: " + nv2.first);
                  }
                }
              }
            }
            else if (nv1.first == "_hldraw_") {
              std::string op;
              ColorData   colorData;
              TextData    textData;
              StyleData   styleData;

              auto *drawArray1 = nv1.second->cast<CJson::Array>();

              for (const auto &objValue1 : drawArray1->values()) {
                auto *drawObj1 = objValue1->cast<CJson::Object>();

                for (const auto &nv2 : drawObj1->nameValueArray()) {
                  //debugMsg("objects/_hldraw_ : " + nv2.first + " " + *nv2.second);

                  if      (nv2.first == "op") {
                    op = nv2.second->cast<CJson::String>()->value();
                  }
                  else if (nv2.first == "grad") {
                    auto grad = nv2.second->cast<CJson::String>()->value();

                    if (op == "c" || op == "C")
                      colorData.grad = grad;
                    else
                      errorMsg(" edges/_hldraw_ unhandled grad");
                  }
                  else if (nv2.first == "color") {
                    auto color = decodeColor(nv2.second->cast<CJson::String>()->value());

                    if      (op == "C")
                      colorData.bg = color;
                    else if (op == "c")
                      colorData.fg = color;
                    else
                      errorMsg(" edges/_hldraw_ unhandled color");
                  }
                  else if (nv2.first == "p0") {
                  }
                  else if (nv2.first == "p1") {
                  }
                  else if (nv2.first == "stops") {
                  }
                  else if (nv2.first == "style") {
                    auto style = nv2.second->cast<CJson::String>()->value();

                    if (op == "S") {
                      if (style != "")
                        decodeStyle(style, styleData);
                    }
                    else
                      errorMsg(" edges/_hldraw_ unhandled style");
                  }
                  else if (nv2.first == "size") {
                    auto size = nv2.second->cast<CJson::Number>()->value();

                    if (op == "F")
                      textData.size = size;
                    else
                      errorMsg(" edges/_hldraw_ unhandled size");
                  }
                  else if (nv2.first == "face") {
                    auto face = nv2.second->cast<CJson::String>()->value();

                    if (op == "F")
                      textData.face = QString::fromStdString(face);
                  }
                  else if (nv2.first == "pt") {
                    auto pt = decodePoint(nv2.second->cast<CJson::Array>());

                    if (op == "t" || op == "T")
                      textData.pos = pt;
                    else
                      errorMsg(" edges/_hldraw_ unhandled pt");
                  }
                  else if (nv2.first == "align") {
                    auto align = decodeAlign(nv2.second->cast<CJson::String>()->value());

                    if (op == "t" || op == "T")
                      textData.align = align;
                    else
                      errorMsg(" edges/_hldraw_ unhandled align");
                  }
                  else if (nv2.first == "width") {
                    auto width = nv2.second->cast<CJson::Number>()->value();

                    if (op == "t" || op == "T")
                      textData.width = width;
                    else
                      errorMsg(" edges/_hldraw_ unhandled width");
                  }
                  else if (nv2.first == "text") {
                    auto text = nv2.second->cast<CJson::String>()->value();

                    if (op == "t" || op == "T") {
                      textData.fg   = colorData.fg;
                      textData.text = QString::fromStdString(text);

                      edge->addText(textData);

                      colorData.reset();
                    }
                    else
                      errorMsg(" edges/_hldraw_ unhandled text");
                  }
                  else if (nv2.first == "points") {
                    auto points = decodePoints(nv2.second->cast<CJson::Array>());

                    if      (op == "P" || op == "p") {
                      PathData pathData;

                      pathData.bg    = colorData.bg;
                      pathData.fg    = colorData.fg;
                      pathData.style = styleData;
                      pathData.path  = pointsToPath(points);

                      edge->setRect(pathData.path.boundingRect());

                      edge->addPath(pathData);

                      colorData.reset();
                    }
                    else if (op == "b" || op == "B") {
                      PathData bspline;

                      bspline.fg    = colorData.fg;
                      bspline.style = styleData;
                      bspline.path  = pointsToBSpline(points);

                      edge->addLine(bspline);

                      edge->setRect(bspline.path.boundingRect());

                      colorData.reset();
                    }
                    else if (op == "L") {
                      PathData bspline;

                      bspline.fg    = colorData.fg;
                      bspline.style = styleData;
                      bspline.path  = pointsToBSpline(points);

                      edge->addLine(bspline);

                      edge->setRect(bspline.path.boundingRect());

                      colorData.reset();
                    }
                    else
                      errorMsg(" edges/_hldraw_ unhandled points for op " + op);
                  }
                  else {
                    errorMsg(" edges/_hldraw_ unhandled: " + nv2.first);
                  }
                }
              }
            }
            else if (nv1.first == "_tdraw_") {
              std::string op;
              ColorData   colorData;
              TextData    textData;
              StyleData   styleData;

              auto *drawArray1 = nv1.second->cast<CJson::Array>();

              for (const auto &objValue1 : drawArray1->values()) {
                auto *drawObj1 = objValue1->cast<CJson::Object>();

                for (const auto &nv2 : drawObj1->nameValueArray()) {
                  //debugMsg("objects/_tdraw_ : " + nv2.first + " " << *nv2.second);

                  if      (nv2.first == "op") {
                    op = nv2.second->cast<CJson::String>()->value();
                  }
                  else if (nv2.first == "grad") {
                    auto grad = nv2.second->cast<CJson::String>()->value();

                    if (op == "c" || op == "C")
                      colorData.grad = grad;
                    else
                      errorMsg(" edges/_tdraw_ unhandled grad");
                  }
                  else if (nv2.first == "color") {
                    auto color = decodeColor(nv2.second->cast<CJson::String>()->value());

                    if      (op == "C")
                      colorData.bg = color;
                    else if (op == "c")
                      colorData.fg = color;
                    else
                      errorMsg(" edges/_tdraw_ unhandled color");
                  }
                  else if (nv2.first == "p0") {
                  }
                  else if (nv2.first == "p1") {
                  }
                  else if (nv2.first == "stops") {
                  }
                  else if (nv2.first == "style") {
                    auto style = nv2.second->cast<CJson::String>()->value();

                    if (op == "S") {
                      if (style != "")
                        decodeStyle(style, styleData);
                    }
                    else
                      errorMsg(" edges/_tdraw_ unhandled style");
                  }
                  else if (nv2.first == "size") {
                    auto size = nv2.second->cast<CJson::Number>()->value();

                    if (op == "F")
                      textData.size = size;
                    else
                      errorMsg(" edges/_tdraw_ unhandled size");
                  }
                  else if (nv2.first == "face") {
                    auto face = nv2.second->cast<CJson::String>()->value();

                    if (op == "F")
                      textData.face = QString::fromStdString(face);
                  }
                  else if (nv2.first == "pt") {
                    auto pt = decodePoint(nv2.second->cast<CJson::Array>());

                    if (op == "t" || op == "T")
                      textData.pos = pt;
                    else
                      errorMsg(" edges/_tdraw_ unhandled pt");
                  }
                  else if (nv2.first == "align") {
                    auto align = decodeAlign(nv2.second->cast<CJson::String>()->value());

                    if (op == "t" || op == "T")
                      textData.align = align;
                    else
                      errorMsg(" edges/_tdraw_ unhandled align");
                  }
                  else if (nv2.first == "width") {
                    auto width = nv2.second->cast<CJson::Number>()->value();

                    if (op == "t" || op == "T")
                      textData.width = width;
                    else
                      errorMsg(" edges/_tdraw_ unhandled width");
                  }
                  else if (nv2.first == "text") {
                    auto text = nv2.second->cast<CJson::String>()->value();

                    if (op == "t" || op == "T") {
                      textData.fg   = colorData.fg;
                      textData.text = QString::fromStdString(text);

                      edge->addText(textData);

                      colorData.reset();
                    }
                    else
                      errorMsg(" edges/_tdraw_ unhandled text");
                  }
                  else if (nv2.first == "points") {
                    auto points = decodePoints(nv2.second->cast<CJson::Array>());

                    if      (op == "P" || op == "p") {
                      PathData pathData;

                      pathData.bg    = colorData.bg;
                      pathData.fg    = colorData.fg;
                      pathData.style = styleData;
                      pathData.path  = pointsToPath(points);

                      edge->setRect(pathData.path.boundingRect());

                      edge->addPath(pathData);

                      colorData.reset();
                    }
                    else if (op == "b" || op == "B") {
                      PathData bspline;

                      bspline.fg    = colorData.fg;
                      bspline.style = styleData;
                      bspline.path  = pointsToBSpline(points);

                      edge->addLine(bspline);

                      edge->setRect(bspline.path.boundingRect());

                      colorData.reset();
                    }
                    else if (op == "L") {
                      PathData bspline;

                      bspline.fg    = colorData.fg;
                      bspline.style = styleData;
                      bspline.path  = pointsToBSpline(points);

                      edge->addLine(bspline);

                      edge->setRect(bspline.path.boundingRect());

                      colorData.reset();
                    }
                    else
                      errorMsg(" edges/_tdraw_ unhandled points for op " + op);
                  }
                  else if (nv2.first == "rect") {
                    auto rect = decodeRect(nv2.second->cast<CJson::Array>());

                    if (op == "E" || op == "e") {
                      PathData ellipse;

                      ellipse.bg    = colorData.bg;
                      ellipse.fg    = colorData.fg;
                      ellipse.style = styleData;
                      ellipse.path  = rectToEllipse(rect);

                      edge->addPath(ellipse);

                      edge->setRect(rect);

                      colorData.reset();
                    }
                    else
                      errorMsg(" edges/_tdraw_ unhandled rect");
                  }
                  else {
                    errorMsg(" edges/_tdraw_ unhandled: " + nv2.first);
                  }
                }
              }
            }
            else if (nv1.first == "_tldraw_") {
              std::string op;
              ColorData   colorData;
              TextData    textData;
              StyleData   styleData;

              auto *drawArray1 = nv1.second->cast<CJson::Array>();

              for (const auto &objValue1 : drawArray1->values()) {
                auto *drawObj1 = objValue1->cast<CJson::Object>();

                for (const auto &nv2 : drawObj1->nameValueArray()) {
                  //debugMsg("objects/_tldraw_ : " + nv2.first + " " + *nv2.second);

                  if      (nv2.first == "op") {
                    op = nv2.second->cast<CJson::String>()->value();
                  }
                  else if (nv2.first == "grad") {
                    auto grad = nv2.second->cast<CJson::String>()->value();

                    if (op == "c" || op == "C")
                      colorData.grad = grad;
                    else
                      errorMsg(" edges/_tldraw_ unhandled grad");
                  }
                  else if (nv2.first == "color") {
                    auto color = decodeColor(nv2.second->cast<CJson::String>()->value());

                    if      (op == "C")
                      colorData.bg = color;
                    else if (op == "c")
                      colorData.fg = color;
                    else
                      errorMsg(" edges/_tldraw_ unhandled color");
                  }
                  else if (nv2.first == "p0") {
                  }
                  else if (nv2.first == "p1") {
                  }
                  else if (nv2.first == "stops") {
                  }
                  else if (nv2.first == "style") {
                    auto style = nv2.second->cast<CJson::String>()->value();

                    if (op == "S") {
                      if (style != "")
                        decodeStyle(style, styleData);
                    }
                    else
                      errorMsg(" edges/_tldraw_ unhandled style");
                  }
                  else if (nv2.first == "size") {
                    auto size = nv2.second->cast<CJson::Number>()->value();

                    if (op == "F")
                      textData.size = size;
                    else
                      errorMsg(" edges/_tldraw_ unhandled size");
                  }
                  else if (nv2.first == "face") {
                    auto face = nv2.second->cast<CJson::String>()->value();

                    if (op == "F")
                      textData.face = QString::fromStdString(face);
                  }
                  else if (nv2.first == "pt") {
                    auto pt = decodePoint(nv2.second->cast<CJson::Array>());

                    if (op == "t" || op == "T")
                      textData.pos = pt;
                    else
                      errorMsg(" edges/_tldraw_ unhandled pt");
                  }
                  else if (nv2.first == "align") {
                    auto align = decodeAlign(nv2.second->cast<CJson::String>()->value());

                    if (op == "t" || op == "T")
                      textData.align = align;
                    else
                      errorMsg(" edges/_tldraw_ unhandled align");
                  }
                  else if (nv2.first == "width") {
                    auto width = nv2.second->cast<CJson::Number>()->value();

                    if (op == "t" || op == "T")
                      textData.width = width;
                    else
                      errorMsg(" edges/_tldraw_ unhandled width");
                  }
                  else if (nv2.first == "text") {
                    auto text = nv2.second->cast<CJson::String>()->value();

                    if (op == "t" || op == "T") {
                      textData.fg   = colorData.fg;
                      textData.text = QString::fromStdString(text);

                      edge->addText(textData);

                      colorData.reset();
                    }
                    else
                      errorMsg(" edges/_tldraw_ unhandled text");
                  }
                  else if (nv2.first == "points") {
                    auto points = decodePoints(nv2.second->cast<CJson::Array>());

                    if      (op == "P" || op == "p") {
                      PathData pathData;

                      pathData.bg    = colorData.bg;
                      pathData.fg    = colorData.fg;
                      pathData.style = styleData;
                      pathData.path  = pointsToPath(points);

                      edge->setRect(pathData.path.boundingRect());

                      edge->addPath(pathData);

                      colorData.reset();
                    }
                    else if (op == "b" || op == "B") {
                      PathData bspline;

                      bspline.fg    = colorData.fg;
                      bspline.style = styleData;
                      bspline.path  = pointsToBSpline(points);

                      edge->addLine(bspline);

                      edge->setRect(bspline.path.boundingRect());

                      colorData.reset();
                    }
                    else if (op == "L") {
                      PathData bspline;

                      bspline.fg    = colorData.fg;
                      bspline.style = styleData;
                      bspline.path  = pointsToBSpline(points);

                      edge->addLine(bspline);

                      edge->setRect(bspline.path.boundingRect());

                      colorData.reset();
                    }
                    else
                      errorMsg(" edges/_tldraw_ unhandled points for op " + op);
                  }
                  else {
                    errorMsg(" edges/_tldraw_ unhandled: " + nv2.first);
                  }
                }
              }
            }
            else {
              errorMsg(" edges unhandled: " + nv1.first);
            }
          }

          edges_.push_back(edge);
        }
      }
      else {
        errorMsg(" unhandled: " + nv.first);
      }
    }
  }
  else {
    //debugMsg(*value);
  }

  return true;
}

bool
App::
processDot(const std::string &filename)
{
  CDotParse::Parse parse(filename);

  if (! parse.parse()) {
    std::cerr << "Parse failed\n";
    return false;
  }

  //---

  int objId = 0;

  for (const auto &ng : parse.graphs()) {
    auto graph = ng.second;

    auto subGraphs = graph->subGraphs();

    auto &attributes = graph->attributes();
    //std::cerr << "Graph: "; attributes.print(std::cerr);

    //---

    if (! graph->parent()) {
      bool ok;
      auto bbReals = attributes.getReals("bb", ok);
      if (! ok) { std::cerr << "No bb\n"; continue; }
      if (bbReals.size() != 4) { std::cerr << "Invalid bb\n"; continue; }
      auto bbox = QRectF(bbReals[0], bbReals[1], bbReals[2], bbReals[3]);
      setBBox(bbox);
    }

    bool ok;
    auto fontSize = attributes.getReal("fontsize", ok); // font size in points
    if (! ok) fontSize = -1;

    //---

    auto addNode = [&](const CDotParse::NodeP &node) {
      auto &attributes = node->attributes();

      bool ok;

      auto w = 72*attributes.getReal("width", ok); // width in inches
      if (! ok) { std::cerr << "No width\n"; return; }
      auto h = 72*attributes.getReal("height", ok); // height in inches
      if (! ok) { std::cerr << "No height\n"; return; }

      auto posReals = attributes.getReals("pos", ok); // center in points
      if (! ok) { std::cerr << "No pos\n"; return; }
      if (posReals.size() != 2) { std::cerr << "Invalid pos\n"; return; }
      auto pos = QPointF(posReals[0], posReals[1]);

      //std::cerr << "node " << w << " " << h << " " << pos.x() << " " << pos.y() << "\n";

      auto object = std::make_shared<Object>();

      object->setType(Object::Type::OBJECT);

      object->setId(++objId);
      object->setName(QString::fromStdString(node->name()));

      object->setPos(pos);
      object->setWidth(w);
      object->setHeight(h);

      object->setRect(QRectF(pos.x(), pos.y(), w, h));

      objects_.push_back(object);
    };

    //---

    for (const auto &node : graph->nodes()) {
      addNode(node.second);
    }

    //---

    for (const auto &subGraph : subGraphs) {
      //auto &attributes = subGraph->attributes();
      //std::cerr << "Subgraph: "; attributes.print(std::cerr);

      if (fontSize > 0)
        setFontSize(fontSize);

      //---

      for (const auto &node : subGraph->nodes()) {
        addNode(node.second);
      }
    }
  }

  for (const auto &ng : parse.graphs()) {
    auto graph = ng.second;

    auto subGraphs = graph->subGraphs();

    for (const auto &subGraph : subGraphs) {
      for (const auto &edge : subGraph->edges()) {
        auto *fromNode = edge->fromNode();
        auto *toNode   = edge->toNode();

        auto *from = findObject(QString::fromStdString(fromNode->name()));
        auto *to   = findObject(QString::fromStdString(toNode  ->name()));

        if (! from || ! to) {
          std::cerr << "No from/to\n";
          continue;
        }

        auto edgeObj = std::make_shared<Object>();

        edgeObj->setType(Object::Type::EDGE);

        edgeObj->setId(++objId);

        from->addDestEdge(edgeObj.get());
        to  ->addSrcEdge (edgeObj.get());

        edgeObj->setHeadId(from->id());
        edgeObj->setTailId(to  ->id());

        edges_.push_back(edgeObj);
      }
    }
  }

  return true;
}

#if 0
bool
App::
processDot1(const std::string &filename)
{
  FILE *fp = fopen(filename.c_str(), "r");
  if (! fp) return false;

  auto getc = [&](int &c) { c = fgetc(fp); return true; };

  auto readLine = [&](std::string &line) {
    int c;

    if (! getc(c))
      return false;

    if (c == EOF)
      return false;

    line = "";

    while (c != EOF && c != '\n') {
      if (c == '\\') {
        if (getc(c)) {
          if (c != '\n') {
            line += '\\';

            line += static_cast<char>(c);
          }
        }
        else
          line += '\\';
      }
      else
        line += static_cast<char>(c);

      if (! getc(c))
        return false;
    }

    return true;
  };

  enum class State {
    NONE,
    DIGRAPH,
    GRAPH,
    NODE,
    DATA,
    ATTRIBUTES
  };

  using NameValues = std::map<QString, QString>;

  //---

  struct ObjectData {
    void printAttributes(const NameValues &attributes) const {
      for (const auto &nv : attributes)
        std::cerr << " " << nv.first.toStdString() << "=" << nv.second.toStdString();
    };

    double getReal(const QString &name, bool &ok) const {
      ok = true;
      double r = 0.0;
      auto p = attributes.find(name);
      if (p == attributes.end()) { ok = false; return 0.0; }
      r = (*p).second.toDouble(&ok);
      return r;
    }

    QPointF getPoint(const QString &name, bool &ok) const {
      ok = true;
      QPointF point;
      auto p = attributes.find(name);
      if (p == attributes.end()) { ok = false; return point; }
      auto strs = (*p).second.split(",");
      if (strs.length() != 2) { ok = false; return point; }
      bool ok1, ok2;
      auto x = strs[0].toDouble(&ok1);
      auto y = strs[1].toDouble(&ok2);
      if (! ok1 || ! ok2) return point;
      point = QPointF(x, y);
      return point;
    }

    QRectF getRect(const QString &name, bool &ok) const {
      ok = true;
      QRectF rect;
      auto p = attributes.find(name);
      if (p == attributes.end()) { ok = false; return rect; }
      auto strs = (*p).second.split(",");
      if (strs.length() != 4) { ok = false; return rect; }
      bool ok1, ok2, ok3, ok4;
      auto x = strs[0].toDouble(&ok1);
      auto y = strs[1].toDouble(&ok2);
      auto w = strs[2].toDouble(&ok3);
      auto h = strs[3].toDouble(&ok4);
      if (! ok1 || ! ok2 || ! ok3 || ! ok4) return rect;
      rect = QRectF(x, y, w, h);
      return rect;
    }

    NameValues attributes;
  };

  struct Graph : public ObjectData {
    void print() const {
      std::cerr << "graph attrs["; printAttributes(attributes); std::cerr << "]\n";
    }
  };

  struct Node : public ObjectData {
    void print() const {
      std::cerr << "node attrs["; printAttributes(attributes); std::cerr << "]\n";
    }
  };

  struct NodeData : public ObjectData {
    std::string name;

    void print() const {
      std::cerr << "nodeData name=" << name;
      std::cerr << " attrs["; printAttributes(attributes); std::cerr << "]\n";
    }
  };

  using Nodes = std::vector<NodeData>;

  struct EdgeData : public ObjectData {
    std::string from;
    std::string to;

    void print() const {
      std::cerr << "edgeData from=" << from << " to=" << to;
      std::cerr << " attrs["; printAttributes(attributes); std::cerr << "]\n";
    }
  };

  using Edges = std::vector<EdgeData>;

  struct Digraph {
    std::string name;
    Graph       graph;
    Node        node;
    Nodes       nodes;
    Edges       edges;

    void print() const {
      std::cerr << "digraph=" << name << "\n";

      graph.print();
      node .print();

      for (const auto &node : nodes)
        node.print();

      for (const auto &edge : edges)
        edge.print();
    }
  };

  Digraph digraph;

  Graph    *currentGraph    = nullptr;
  Node     *currentNode     = nullptr;
  NodeData *currentNodeData = nullptr;
  EdgeData *currentEdgeData = nullptr;

  auto resetCurrent = [&]() {
    currentGraph    = nullptr;
    currentNode     = nullptr;
    currentEdgeData = nullptr;
    currentNodeData = nullptr;
  };

  //---

  State state { State::NONE };

  std::vector<State> stack;

#if 0
  auto printState = [&](State s) {
    std::cerr << "state=";
    switch (s) {
      case State::NONE      : std::cerr << "none"; break;
      case State::DIGRAPH   : std::cerr << "digraph"; break;
      case State::GRAPH     : std::cerr << "graph"; break;
      case State::NODE      : std::cerr << "node"; break;
      case State::DATA      : std::cerr << "data"; break;
      case State::ATTRIBUTES: std::cerr << "attributes"; break;
    }
    std::cerr << "\n";
  };
#endif

  auto pushState = [&](State s) {
    //printState(s);

    stack.push_back(s);

    state = s;
  };

  auto popState = [&]() {
    if (! stack.empty()) {
      stack.pop_back();

      state = stack.back();

      //printState(state);
    }
    else
      std::cerr << "No state\n";
  };

  auto endCurrent = [&]() {
    popState();
    popState();

    resetCurrent();
  };

  std::string line;

  while (readLine(line)) {
    //std::cerr << "line=" << line << "\n";

    CStrParse parse(line);

    parse.skipSpace();

    //---

    auto readAttribute = [&](std::string &name, std::string &value) {
      parse.skipSpace();

      if (parse.eof())
        return false;

      if (parse.isChar(']'))
        return false;

      parse.readIdentifier(name);

      parse.skipSpace();

      if (parse.isChar('=')) {
        parse.skipChar();
        parse.skipSpace();

        if (parse.isChar('"'))
          parse.readString(value, /*strip_quotes*/true);
        else {
          while (! parse.eof() && ! parse.isSpace() && ! parse.isChar(',') && ! parse.isChar(']'))
            value += parse.readChar();
        }

        parse.skipSpace();

        if (parse.isChar(','))
          parse.skipChar();
      }

      return true;
    };

    auto readAttributes = [&](NameValues &attributes) {
      parse.skipSpace();

      while (! parse.eof()) {
        std::string name, value;

        if (! readAttribute(name, value))
          return false;

        attributes[QString::fromStdString(name)] = QString::fromStdString(value);
      }

      return true;
    };

    //---

    if      (state == State::NONE) {
      std::string ident;

      parse.readIdentifier(ident);

      if (ident == "digraph") {
        pushState(State::DIGRAPH);

        parse.skipSpace();

        parse.readIdentifier(digraph.name);

        parse.skipSpace();

        if (! parse.isChar('{'))
          std::cerr << "Syntax error '" << line << ";\n";

        //digraph.print();
      }
      else {
        std::cerr << "Unhandled '" << line << ";\n";
      }
    }
    else if (state == State::DIGRAPH) {
      if (parse.isChar('}')) {
        popState();

        resetCurrent();

        continue;
      }

      std::string ident;

      parse.readIdentifier(ident);

      // graph attributes
      if      (ident == "graph") {
        pushState(State::GRAPH);

        parse.skipSpace();

        if (parse.isChar('[')) {
          parse.skipChar();

          pushState(State::ATTRIBUTES);

          readAttributes(digraph.graph.attributes);

          parse.skipSpace();

          if (parse.isString("];"))
            endCurrent();
        }

        //digraph.graph.print();

        resetCurrent();

        currentGraph = &digraph.graph;
      }
      // node attributes
      else if (ident == "node") {
        pushState(State::NODE);

        parse.skipSpace();

        if (parse.isChar('[')) {
          parse.skipChar();

          pushState(State::ATTRIBUTES);

          readAttributes(digraph.node.attributes);

          parse.skipSpace();

          if (parse.isString("];"))
            endCurrent();
        }

        //digraph.node.print();

        resetCurrent();

        currentNode = &digraph.node;
      }
      else {
        pushState(State::DATA);

        parse.skipSpace();

        EdgeData edgeData;
        NodeData nodeData;
        bool     isEdge = false;

        if (parse.isString("->")) {
          edgeData.from = ident;

          parse.skipChars("->");

          parse.skipSpace();

          std::string ident1;

          parse.readIdentifier(edgeData.to);

          isEdge = true;
        }
        else {
          nodeData.name = ident;
        }

        parse.skipSpace();

        if (parse.isChar('[')) {
          parse.skipChar();

          pushState(State::ATTRIBUTES);

          if (isEdge)
            readAttributes(edgeData.attributes);
          else
            readAttributes(nodeData.attributes);

          parse.skipSpace();

          if (parse.isString("];"))
            endCurrent();
        }

        if (isEdge) {
          digraph.edges.push_back(edgeData);

          //edgeData.print();

          resetCurrent();

          currentEdgeData = &digraph.edges[digraph.edges.size() - 1];
        }
        else {
          digraph.nodes.push_back(nodeData);

          //nodeData.print();

          resetCurrent();

          currentNodeData = &digraph.nodes[digraph.nodes.size() - 1];
        }
      }
    }
    else if (state == State::ATTRIBUTES) {
      if (parse.isString("];")) {
        endCurrent();
      }
      else {
        if      (currentGraph)
          readAttributes(currentGraph->attributes);
        else if (currentNode)
          readAttributes(currentNode->attributes);
        else if (currentEdgeData)
          readAttributes(currentEdgeData->attributes);
        else if (currentNodeData)
          readAttributes(currentNodeData->attributes);
        else {
          std::cerr << "No current object\n";
          assert(false);
        }

        parse.skipSpace();

        if (parse.isString("];")) {
          endCurrent();
        }
      }
    }
    else {
      std::cerr << "Unhandled '" << line << ";\n";
    }
  }

  fclose(fp);

  if (isDebug())
    digraph.print();

  //---

  bool ok;
  auto bbox = digraph.graph.getRect("bb", ok);
  if (ok) setBBox(bbox);

  auto fontSize = digraph.graph.getReal("fontsize", ok); // font size in points
  if (ok) setFontSize(fontSize);

  int objId = 0;

  for (const auto &node : digraph.nodes) {
    bool ok;

    auto w = 72*node.getReal("width", ok); // width in inches
    if (! ok) { std::cerr << "No width\n"; continue; }
    auto h = 72*node.getReal("height", ok); // height in inches
    if (! ok) { std::cerr << "No height\n"; continue; }

    auto pos = node.getPoint("pos", ok); // center in points
    if (! ok) { std::cerr << "No pos\n"; continue; }

    //std::cerr << "node " << w << " " << h << " " << pos.x() << " " << pos.y() << "\n";

    auto object = std::make_shared<Object>();

    object->setType(Object::Type::OBJECT);

    object->setId(++objId);
    object->setName(QString::fromStdString(node.name));

    object->setPos(pos);
    object->setWidth(w);
    object->setHeight(h);

    objects_.push_back(object);
  }

  for (const auto &edge : digraph.edges) {
    //std::cerr << "edge\n";

    auto *from = findObject(QString::fromStdString(edge.from));
    auto *to   = findObject(QString::fromStdString(edge.to));

    if (! from || ! to) {
      std::cerr << "No from/to\n";
      continue;
    }

    auto edgeObj = std::make_shared<Object>();

    edgeObj->setType(Object::Type::EDGE);

    edgeObj->setId(++objId);

    from->addDestEdge(edgeObj.get());
    to  ->addSrcEdge (edgeObj.get());

    edgeObj->setHeadId(from->id());
    edgeObj->setTailId(to  ->id());

    edges_.push_back(edgeObj);
  }

  return true;
}
#endif

Object *
App::
findObject(int id)
{
  for (auto &object : objects_)
    if (object->id() == id)
      return object.get();

  return nullptr;
}

Object *
App::
findObject(const QString &name)
{
  for (auto &object : objects_)
    if (object->name() == name)
      return object.get();

  return nullptr;
}

void
App::
errorMsg(const std::string &str) const
{
  std::cerr << str << "\n";
}

#if 0
void
App::
debugMsg(const std::string &str) const
{
  std::cerr << str << "\n";
}
#endif

}
