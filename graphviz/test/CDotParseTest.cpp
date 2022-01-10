#include <CDotParse.h>
#include <iostream>

int
main(int argc, char **argv)
{
  std::string filename;
  bool        debug      = false;
  bool        print      = false;
  bool        csv        = false;
  bool        mst        = false;
  bool        sub_graphs = false;

  for (auto i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      std::string arg(&argv[i][1]);

      if      (arg == "debug")
        debug = true;
      else if (arg == "print")
        print = true;
      else if (arg == "csv")
        csv = true;
      else if (arg == "mst")
        mst = true;
      else if (arg == "sub_graphs")
        sub_graphs = true;
      else if (arg == "h") {
        std::cerr << "CDotParseTest [-debug] [-print] [-csv] [-mst] [-sub_graphs] <file>\n";
        exit(1);
      }
      else
        std::cerr << "Unhandled option: " << arg << "\n";
    }
    else
      filename = argv[i];
  }

  if (filename == "") {
    std::cerr << "Missing filename\n";
    exit(1);
  }

  CDotParse::Parse parse(filename);

  parse.setDebug(debug);
  parse.setPrint(print);
  parse.setCSV  (csv);

  if (! parse.parse()) {
    std::cerr << "Parse failed\n";
    exit(1);
  }

  if (mst) {
    std::cerr << "Minimum Spaning Tree\n";

    for (const auto &ng : parse.graphs()) {
      auto graph = ng.second->minimumSpaningTree();

      std::cout << *graph << "\n";
    }
  }

  if (sub_graphs) {
    std::cerr << "Sub Graphs\n";

    for (const auto &ng : parse.graphs()) {
      auto subGraphs = ng.second->subGraphs();

      for (const auto &subGraph : subGraphs) {
        std::cerr << "Sub Graph " << subGraph->name() << "\n";

        for (const auto &edge : subGraph->edges())
          std::cout << " " << *edge << "\n";
      }
    }
  }

  exit(0);
}
